#include "core_runner.h"
#include "paths.h"
#include "config.h"
#include "opnif.h"
#include "beep.h"
#include "devices/Z80.h"
#include <chrono>
#include <unistd.h>
#include <vector>
#include <cstdio>

CoreRunner::CoreRunner() : running(false), paused(false), configPending(false) {}
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
    if (!pc88.Init(draw, &diskmgr, &tapemgr)) return false;
    
    pc88.ApplyConfig(&Config::Get());
    pc88.Reset();
    sound.Init();
    sound.Connect(pc88.GetOPN1());
    sound.Connect(pc88.GetOPN2());
    sound.Connect(pc88.GetBEEP());
    keyInput.Init(&pc88);
    uiManager.Init();
    return true;
}

void CoreRunner::RequestConfigApply(const PC8801::Config& cfg) {
    std::lock_guard<std::mutex> lock(configMutex);
    pendingConfig = cfg;
    configPending = true;
}

void CoreRunner::UpdateInput() {
    if (!uiManager.IsMenuOpen()) {
        if (running) keyInput.Update(&pc88);
    }
}

void CoreRunner::UpdateUI(bool& shouldExit) {
    uiManager.Update(shouldExit, &pc88, this);
}

void CoreRunner::DrawUI() {
    uiManager.Draw(&diskmgr, Config::Get(), &pc88, this);
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

    while (running) {
        if (paused || uiManager.IsMenuOpen()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            startTime = std::chrono::high_resolution_clock::now();
            totalTicksEmulated = 0;
            continue;
        }

        if (configPending) {
            std::lock_guard<std::mutex> lock(configMutex);
            pc88.ApplyConfig(&pendingConfig);
            Config::Get() = pendingConfig;
            Config::Save(pendingConfig);
            configPending = false;
        }

        const auto& cfg = Config::Get();
        uint32_t clockParam = cfg.clock * 10;
        uint32_t speedParam = (cfg.speed > 0) ? cfg.speed : 100;

        // Run about 1/60th of a second (1667 ticks)
        uint32_t ticksToRun = 1667; 
        pc88.Proceed(ticksToRun, clockParam, speedParam);
        pc88.UpdateScreen(true);
        totalTicksEmulated += ticksToRun;

        // Synchronization
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count();
        
        // Target time for emulated ticks (1 tick = 10us)
        // If speed is 100%, 1667 ticks should take 16670us.
        uint64_t targetUs = totalTicksEmulated * 10;
        
        if (elapsed < targetUs) {
            uint64_t sleepUs = targetUs - elapsed;
            if (sleepUs > 2000) {
                std::this_thread::sleep_for(std::chrono::microseconds(sleepUs - 1000));
            }
            // Busy wait for the remaining time to get better precision
            while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime).count() < targetUs) {
                std::this_thread::yield();
            }
        }

        // Periodic reset to prevent drift
        if (totalTicksEmulated > 100000) {
            startTime = std::chrono::high_resolution_clock::now();
            totalTicksEmulated = 0;
        }
    }
}
