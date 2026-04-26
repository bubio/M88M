#include "core_runner.h"
#include "opnif.h"
#include "beep.h"
#include <chrono>

CoreRunner::CoreRunner() : running(false), paused(false) {
}

CoreRunner::~CoreRunner() {
    Stop();
}

bool CoreRunner::Init(Draw* draw) {
    if (!diskmgr.Init()) return false;
    // TapeManager doesn't have Init()?
    
    if (!pc88.Init(draw, &diskmgr, &tapemgr)) {
        return false;
    }

    sound.Init();
    sound.Connect(pc88.GetOPN1());
    sound.Connect(pc88.GetOPN2());
    sound.Connect(pc88.GetBEEP());

    keyInput.Init(&pc88);

    return true;
}

void CoreRunner::UpdateInput() {
    keyInput.Update();
}

void CoreRunner::Start() {
    if (running) return;
    running = true;
    sound.Start();
    thread = std::thread(&CoreRunner::Run, this);
}

void CoreRunner::Stop() {
    running = false;
    if (thread.joinable()) {
        thread.join();
    }
}

void CoreRunner::Pause(bool p) {
    paused = p;
}

void CoreRunner::Run() {
    while (running) {
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Proceed for a small slice of time (e.g., 1ms)
        pc88.Proceed(1000, 4000000, 100);

        // Trigger screen update
        pc88.UpdateScreen();

        // Basic throttle to avoid 100% CPU if not needed
        // In a real implementation, we would sync with real time
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}
