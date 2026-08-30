#pragma once

#include "pc88.h"
#include "sound.h"
#include "diskmgr.h"
#include "tapemgr.h"
#include "audio_out.h"
#include "key_input.h"
#include "disk_dialog.h" // Actually UIManager now
#include <thread>
#include <atomic>
#include <string>
#include <mutex>

class CoreRunner : public PC88 {
public:
    CoreRunner();
    virtual ~CoreRunner();

    bool Init(Draw* draw);
    void Start();
    void Stop();
    void Pause(bool pause);
    void UpdateInput();
    void UpdateUI(bool& shouldExit);
    void DrawUI(bool& shouldExit);
    void RequestReset();
    bool SaveState(const std::string& path, const std::string& screenshotPath = "", std::string* message = nullptr);
    bool LoadState(const std::string& path, std::string* message = nullptr);
    
    // Thread-safe config update
    void RequestConfigApply(const PC8801::Config& cfg, bool requireReset = false);
    void StopAudio();
    void RestartAudio();

    PC88* GetPC88() { return this; }
    DiskManager* GetDiskManager() { return &diskmgr; }
    UIManager* GetUIManager() { return &uiManager; }
    bool HasRomError() const { return !romError.empty(); }
    const std::string& GetRomError() const { return romError; }
    void ClearRomError() { romError.clear(); }

private:
    void Run();
    std::string CheckMandatoryRoms(const std::string& romDir);

    DiskManager diskmgr;
    TapeManager tapemgr;
    PC8801::Sound coreSound;
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
    std::atomic<bool> resetPending;
    std::mutex stateMutex;

    // Run() 内でリセット時に音声デバイスの再初期化が必要かどうかを
    // 判定するための直近の設定値。関数ローカルの static だと 0 から
    // 始まってしまい、初回リセット時に必ず再初期化が走って
    // (別スレッドからの InitAudioDevice/CloseAudioDevice 呼び出しにより)
    // フリーズする不具合があったため、Init() で実際の初期値を設定する。
    uint32 lastAudioRate = 0;
    int lastAudioBufMs = 0;

    // ESC でメニュー/ダイアログを閉じた直後、同じキー押下が
    // そのままエミュ側の ESC (STOP相当) にも入力されてしまうのを防ぐ。
    // 物理キーが離されるまで、このキー押下はメニュー操作専用として扱う。
    bool escOwnedByMenu = false;
};
