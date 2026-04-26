#pragma once

#include "pc88.h"
#include "diskmgr.h"
#include "tapemgr.h"
#include "audio_out.h"
#include "key_input.h"
#include "disk_dialog.h"
#include <thread>
#include <atomic>
#include <string>

class CoreRunner {
public:
    CoreRunner();
    ~CoreRunner();

    bool Init(Draw* draw);
    void Start();
    void Stop();
    void Pause(bool pause);
    void UpdateInput();
    void DrawUI();
    void OpenDiskDialog(int drive);
    void ToggleSettings() { showSettings = !showSettings; }
    
    // ROMエラー関連
    bool HasRomError() const { return !romError.empty(); }
    const std::string& GetRomError() const { return romError; }
    void ClearRomError() { romError.clear(); }

    PC88* GetPC88() { return &pc88; }
    DiskManager* GetDiskManager() { return &diskmgr; }

private:
    void Run();
    std::string CheckMandatoryRoms(const std::string& romDir);

    PC88 pc88;
    DiskManager diskmgr;
    TapeManager tapemgr;
    RaylibSound sound;
    KeyInput keyInput;
    DiskDialog diskDialog;
    
    std::thread thread;
    std::atomic<bool> running;
    std::atomic<bool> paused;
    bool showSettings;
    std::string romError;
};
