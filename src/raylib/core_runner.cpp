#include "core_runner.h"
#include "paths.h"
#include "config.h"
#include "opnif.h"
#include "beep.h"
#include "devices/Z80.h"
#include "screen_view.h"
#include "file.h"
#include "zlib/zlib.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <string.h>
#include <string>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace {
    // Snapshot constants and header (moved from pc88.cpp)
    #define SNAPSHOT_ID	"M88 SnapshotData"
    enum { ssmajor = 1, ssminor = 1 };

    struct SnapshotHeader {
        char id[16];
        uint8 major, minor;
        int8 disk[2];
        int datasize;
        PC8801::Config::BASICMode basicmode;
        int16 clock;
        uint16 erambanks;
        uint16 cpumode;
        uint16 mainsubratio;
        uint flags;
        uint flag2;
    };
}

CoreRunner::CoreRunner() : running(false), paused(false), configPending(false), configResetPending(false), resetPending(false) {}
CoreRunner::~CoreRunner() { Stop(); }

std::string CoreRunner::CheckMandatoryRoms(const std::string& romDir) {
    std::vector<std::string> mandatory = { "N88.ROM", "DISK.ROM", "FONT.ROM" };
    std::vector<std::string> missing;
    for (const auto& file : mandatory) {
        std::string path = romDir + "/" + file;
        if (access(path.c_str(), F_OK) != 0) missing.push_back(file);
    }
    if (missing.empty()) return "";
    std::string msg = "Mandatory ROM files are missing:\n";
    for (const auto& m : missing) msg += " - " + m + "\n";
    msg += "\nPlease place them in this folder:\n\n" + romDir;
    return msg;
}

bool CoreRunner::Init(Draw* draw) {
    std::string romDir = Paths::GetRomDir();
    romError = CheckMandatoryRoms(romDir);
    if (!romError.empty()) return false;
    chdir(romDir.c_str());
    if (!diskmgr.Init()) return false;
    if (!PC88::Init(draw, &diskmgr, &tapemgr, romDir.c_str())) return false;

    ApplyConfig(&Config::Get());
    Reset();
    const uint outrate = 48000;
    int bufsize = (int)Config::Get().soundbuffer;
    if (bufsize < 1024) bufsize = 4096;
    if (!coreSound.Init(this, outrate, bufsize)) return false;
    coreSound.ApplyConfig(&Config::Get());
    sound.Init();
    sound.SetVolume(&Config::Get());
    sound.SetSource(coreSound.GetSoundSource());
    GetOPN1()->Connect(&coreSound);
    GetOPN2()->Connect(&coreSound);
    GetBEEP()->Connect(&coreSound);
    keyInput.Init(&bus1);
    uiManager.Init();
    return true;
}

void CoreRunner::RequestConfigApply(const PC8801::Config& cfg, bool requireReset) {
    std::lock_guard<std::mutex> lock(configMutex);
    pendingConfig = cfg;
    configPending = true;
    configResetPending = requireReset;
}

void CoreRunner::RequestReset() {
    resetPending = true;
}

bool CoreRunner::SaveState(const std::string& path, const std::string& screenshotPath, std::string* message) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    bool ok = false;
    const PC8801::Config& config = Config::Get();
    const bool docomp = !!(config.flag2 & PC8801::Config::compresssnapshot);
    const uint size = devlist.GetStatusSize();
    const uLongf maxCompressed = compressBound(size);
    std::vector<uint8> state(size);
    std::vector<uint8> payload(docomp ? maxCompressed + 4 : size);

    if (devlist.SaveStatus(state.data())) {
        uLongf payloadSize = size;
        const uint8* payloadData = state.data();
        bool stepOk = true;

        if (docomp) {
            uLongf compressedSize = maxCompressed;
            if (compress(payload.data() + 4, &compressedSize, state.data(), size) == Z_OK) {
                *(int32*)payload.data() = -(int32)compressedSize;
                payloadSize = compressedSize + 4;
                payloadData = payload.data();
            } else {
                stepOk = false;
            }
        }

        if (stepOk) {
            SnapshotHeader ssh;
            memset(&ssh, 0, sizeof(ssh));
            memcpy(ssh.id, SNAPSHOT_ID, 16);
            ssh.major = ssmajor;
            ssh.minor = ssminor;
            ssh.datasize = (int)size;
            ssh.basicmode = config.basicmode;
            ssh.clock = (int16)config.clock;
            ssh.erambanks = (uint16)config.erambanks;
            ssh.cpumode = (uint16)config.cpumode;
            ssh.mainsubratio = (uint16)config.mainsubratio;
            ssh.flags = config.flags | (payloadSize < size ? 0x80000000u : 0);
            ssh.flag2 = config.flag2;
            for (uint i = 0; i < 2; i++)
                ssh.disk[i] = (int8)diskmgr.GetCurrentDisk(i);

            FileIO file;
            if (file.CreateNew(path.c_str())) {
                if (file.Write(&ssh, sizeof(ssh)) == sizeof(ssh) &&
                    file.Write(payloadData, (int32)payloadSize) == (int32)payloadSize) {
                    ok = true;
                }
            }
        }
    }

    if (ok && !screenshotPath.empty()) {
        if (RaylibDraw* rayDraw = dynamic_cast<RaylibDraw*>(draw)) {
            rayDraw->SavePNG(screenshotPath.c_str());
        }
    }
    if (message) *message = ok ? "State saved" : "Failed to save state";
    return ok;
}

bool CoreRunner::LoadState(const std::string& path, std::string* message) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    bool ok = false;
    PC8801::Config cfg = Config::Get();
    FileIO file;

    if (file.Open(path.c_str(), FileIO::readonly)) {
        SnapshotHeader ssh;
        if (file.Read(&ssh, sizeof(ssh)) == sizeof(ssh) &&
            memcmp(ssh.id, SNAPSHOT_ID, 16) == 0 &&
            ssh.major == ssmajor && ssh.minor <= ssminor) {

            const uint fl1a = PC8801::Config::subcpucontrol | PC8801::Config::fullspeed
                            | PC8801::Config::enableopna	| PC8801::Config::enablepcg
                            | PC8801::Config::fv15k 		| PC8801::Config::cpuburst
                            | PC8801::Config::cpuclockmode	| PC8801::Config::digitalpalette
                            | PC8801::Config::opnona8		| PC8801::Config::opnaona8
                            | PC8801::Config::enablewait;
            const uint fl2a = PC8801::Config::disableopn44;

            cfg.flags = (cfg.flags & ~fl1a) | (ssh.flags & fl1a);
            cfg.flag2 = (cfg.flag2 & ~fl2a) | (ssh.flag2 & fl2a);
            cfg.basicmode = ssh.basicmode;
            cfg.clock = ssh.clock;
            cfg.erambanks = ssh.erambanks;
            cfg.cpumode = ssh.cpumode;
            cfg.mainsubratio = ssh.mainsubratio;

            ApplyConfig(&cfg);
            Reset();

            std::vector<uint8> state((size_t)ssh.datasize);
            bool dataRead = false;
            if (ssh.flags & 0x80000000u) {
                int32 csize = 0;
                if (file.Read(&csize, 4) == 4 && csize < 0) {
                    csize = -csize;
                    std::vector<uint8> compressed((size_t)csize);
                    if (file.Read(compressed.data(), csize) == csize) {
                        uLongf destSize = (uLongf)ssh.datasize;
                        dataRead = uncompress(state.data(), &destSize, compressed.data(), (uLongf)csize) == Z_OK
                                && destSize == (uLongf)ssh.datasize;
                    }
                }
            } else {
                dataRead = file.Read(state.data(), ssh.datasize) == ssh.datasize;
            }

            if (dataRead) {
                if (devlist.LoadStatus(state.data())) {
                    ok = true;
                } else {
                    Reset();
                }
            }
        }
    }

    if (ok) {
        coreSound.ApplyConfig(&cfg);
        sound.SetVolume(&cfg);
        Config::Get() = cfg;
        Config::Save(cfg);
        UpdateScreen(true);
    }
    if (message) *message = ok ? "State loaded" : "Failed to load state";
    return ok;
}

void CoreRunner::UpdateInput() {
    if (!uiManager.IsMenuOpen()) {
        if (running) keyInput.Update();
    }
}

void CoreRunner::UpdateUI(bool& shouldExit) {
    uiManager.Update(shouldExit, this, this);
}

void CoreRunner::DrawUI(bool& shouldExit) {
    uiManager.Draw(&diskmgr, Config::Get(), this, this, shouldExit);
}

void CoreRunner::Start() {
    if (running || HasRomError()) return;
    running = true;
    sound.Start();
    thread = std::thread(&CoreRunner::Run, this);
}

void CoreRunner::Stop() {
    if (running) {
        running = false;
        if (thread.joinable()) thread.join();
    }
}

void CoreRunner::Pause(bool p) { paused = p; }

void CoreRunner::Run() {
    auto startTime = std::chrono::high_resolution_clock::now();
    uint64_t totalTicksEmulated = 0;
    bool audioPaused = false;

    while (running) {
        if (paused || uiManager.IsMenuOpen()) {
            if (!audioPaused) {
                sound.Pause(true);
                audioPaused = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            startTime = std::chrono::high_resolution_clock::now();
            totalTicksEmulated = 0;
            continue;
        }

        if (audioPaused) {
            sound.Pause(false);
            audioPaused = false;
        }

        int actualTicks = 0;
        {
            std::lock_guard<std::mutex> lock(stateMutex);

            if (resetPending.exchange(false)) {
                Reset();
            }

            if (configPending) {
                // Reset();
                ApplyConfig(&pendingConfig);
                coreSound.ApplyConfig(&pendingConfig);
                sound.SetVolume(&pendingConfig);
                if (configResetPending) {
                    Reset();
                }
                Config::Get() = pendingConfig;
                Config::Save(pendingConfig);
                configPending = false;
            }

            // Run one frame (1/60s)
            const auto& cfg = Config::Get();
            uint32_t clockParam = cfg.clock;
            uint32_t speedParam = clockParam * (cfg.speed > 0 ? cfg.speed : 100) / 100;
            uint32_t ticksToRun = GetFramePeriod();

            TimeSync();
            actualTicks = Proceed(ticksToRun, clockParam, speedParam);
            UpdateScreen(true);
        }

        totalTicksEmulated += actualTicks;

        // Synchronization
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count();
        uint64_t targetUs = totalTicksEmulated * 10;

        if (elapsedUs < targetUs) {
            uint64_t waitUs = targetUs - elapsedUs;
            if (waitUs > 1000) {
                std::this_thread::sleep_for(std::chrono::microseconds(waitUs - 500));
            }
            // Busy wait for precision
            while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime).count() < targetUs) {
                std::this_thread::yield();
            }
        }

        // Periodic reset to prevent drift
        if (totalTicksEmulated > 500000) { // 5 seconds
            startTime = std::chrono::high_resolution_clock::now();
            totalTicksEmulated = 0;
        }
    }
}
