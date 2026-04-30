#include "core_runner.h"
#include "paths.h"
#include "config.h"
#include "opnif.h"
#include "beep.h"
#include "devices/Z80.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <unistd.h>

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
    uint soundBuffer = Config::Get().soundbuffer;
    if (soundBuffer < 1024) soundBuffer = 4096;
    if (!coreSound.Init(this, 44100, soundBuffer)) return false;
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
            int actualTicks = Proceed(ticksToRun, clockParam, speedParam);
            UpdateScreen(true);

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
