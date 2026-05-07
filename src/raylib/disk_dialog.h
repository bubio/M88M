#pragma once

#include "raylib.h"
#include "diskmgr.h"
#include "pc88/config.h"
#include <string>
#include <vector>

class UIManager {
public:
    UIManager();
    ~UIManager();

    void Init();
    void Update(bool& shouldExit, class PC88* pc88, class CoreRunner* coreRunner);
    void Draw(DiskManager* diskmgr, PC8801::Config& cfg, class PC88* pc88, class CoreRunner* coreRunner, bool& shouldExit);
    void OpenNativeDialog(DiskManager* diskmgr, int drive);
    void OpenBothDrives(DiskManager* diskmgr);
    void MountDisk(DiskManager* diskmgr, const char* path, int img1, int img2);
    void AddRecent(const std::string& path);
    void LoadRecent();
    void SaveRecent();
    void SetJPFont(Font font) { fontJp = font; }
    void SetENFont(Font font) { fontEn = font; }

    bool IsMenuOpen() const { return showMenu; }
    void ToggleMenu(class CoreRunner* coreRunner = nullptr);
    void RequestQuitConfirm() { modalState = MODAL_CONFIRM_QUIT; showMenu = true; }

    enum ModalState {
        MODAL_NONE,
        MODAL_CONFIRM_RESET,
        MODAL_CONFIRM_QUIT
    };

private:
    void DrawMainMenu(DiskManager* diskmgr, class PC88* pc88, bool& shouldExit, class CoreRunner* coreRunner);
    void DrawSettings(PC8801::Config& cfg, class PC88* pc88, class CoreRunner* coreRunner);
    void DrawDiskSelector(DiskManager* diskmgr);
    void DrawRecentDiskDialog(DiskManager* diskmgr);
    void DrawStateDialog(DiskManager* diskmgr, class CoreRunner* coreRunner);
    void DrawConfirmDialog(bool& shouldExit, class PC88* pc88, class CoreRunner* coreRunner);
    void DrawStatusBar(DiskManager* diskmgr);
    void DrawDriveStatus(DiskManager* diskmgr, int drive, float x, float y);
    std::string GetStatePath(DiskManager* diskmgr, int slot) const;
    std::string GetStateScreenshotPath(DiskManager* diskmgr, int slot) const;
    bool StateSlotExists(DiskManager* diskmgr, int slot) const;
    void LoadStatePreview(const std::string& path);
    void DrawEnText(const char* text, int x, int y, Color color) const;
    int MeasureEnText(const char* text) const;

    bool showMenu;
    ModalState modalState;
    bool showSettings;
    bool showStateDialog;
    bool showRecentDialog;
    int selectingDiskForDrive; // -1: none, 0: Drive 1, 1: Drive 2
    int recentDiskTargetDrive;
    int activeTab;
    int currentStateSlot;
    Vector2 diskScrollOffset;
    Vector2 recentScrollOffset;
    
    // UI state
    int windowScale;
    bool isFullscreen;
    bool basicModeEdit;
    bool windowScaleEdit;
    bool cpuModeEdit;
    bool port44Edit;
    bool portA8Edit;
    bool samplingEdit;
    bool keyboardEdit;
    bool speedEdit;
    bool eramEdit;
    bool bufferEdit;
    bool masterVolEdit;
    bool volFmEdit;
    bool volSsgEdit;
    bool volAdpcmEdit;
    bool volRhythmEdit;
    bool volRhythmDetailEdit[6];
    bool mouseSensEdit;
    
    // Persistent values for uint config members (GuiValueBox requires int*)
    int bufferVal;
    int mouseSensVal;
    Vector2 mixerScroll;
    Vector2 inputScroll;
    
    std::string lastOpenedPath[2];
    std::vector<std::string> recentDisks;
    std::string stateMessage;
    std::string statePreviewPath;
    Texture2D statePreviewTexture;
    Font fontJp;
    Font fontEn;
    bool resetPending;
};
