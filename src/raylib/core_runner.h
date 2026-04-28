#pragma once

#include "pc88.h"
#include "diskmgr.h"
#include "tapemgr.h"
#include "audio_out.h"
#include "key_input.h"
#include "disk_dialog.h" // Actually UIManager now
#include <thread>
#include <atomic>
#include <string>
#include <mutex>

class CoreRunner {
public:
    CoreRunner();
    ~CoreRunner();

    bool Init(Draw* draw);
    void Start();
    void Stop();
    void Pause(bool pause);
    void UpdateInput();
    void UpdateUI(bool& shouldExit);
    void DrawUI(bool& shouldExit);
    
    // Thread-safe config update
    void RequestConfigApply(const PC8801::Config& cfg, bool requireReset = false);

    PC88* GetPC88() { return &pc88; }
    DiskManager* GetDiskManager() { return &diskmgr; }
    bool HasRomError() const { return !romError.empty(); }
    const std::string& GetRomError() const { return romError; }
    void ClearRomError() { romError.clear(); }

private:
    void Run();
    std::string CheckMandatoryRoms(const std::string& romDir);

    PC88 pc88;
    DiskManager diskmgr;
    TapeManager tapemgr;
    RaylibSound sound;
    KeyInput keyInput;
    UIManager uiManager;
    
    std::thread thread;
    std::atomic<bool> running;
    std::atomic<bool> paused;
    std::string romError;

    // Config deferred application
    std::mutex configMutex;
    PC8801::Config pendingConfig;
    std::atomic<bool> configPending;
    std::atomic<bool> configResetPending;
};
