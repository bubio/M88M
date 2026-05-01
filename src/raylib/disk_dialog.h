#pragma once

#include "raylib.h"
#include "diskmgr.h"
#include "pc88/config.h"
#include <string>

class UIManager {
public:
    UIManager();
    ~UIManager();

    void Init();
    void Update(bool& shouldExit, class PC88* pc88, class CoreRunner* coreRunner);
    void Draw(DiskManager* diskmgr, PC8801::Config& cfg, class PC88* pc88, class CoreRunner* coreRunner, bool& shouldExit);
    void OpenNativeDialog(DiskManager* diskmgr, int drive);
    void OpenBothDrives(DiskManager* diskmgr);
    void SetJPFont(Font font) { fontJp = font; }

    bool IsMenuOpen() const { return showMenu; }
    void ToggleMenu(class CoreRunner* coreRunner = nullptr);

private:
    void DrawMainMenu(DiskManager* diskmgr, class PC88* pc88, bool& shouldExit, class CoreRunner* coreRunner);
    void DrawSettings(PC8801::Config& cfg, class PC88* pc88, class CoreRunner* coreRunner);
    void DrawDiskSelector(DiskManager* diskmgr);
    void DrawStateDialog(DiskManager* diskmgr, class CoreRunner* coreRunner);
    void DrawStatusBar(DiskManager* diskmgr);
    void DrawDriveStatus(DiskManager* diskmgr, int drive, float x, float y);
    std::string GetStatePath(DiskManager* diskmgr, int slot) const;
    std::string GetStateScreenshotPath(DiskManager* diskmgr, int slot) const;
    bool StateSlotExists(DiskManager* diskmgr, int slot) const;
    void LoadStatePreview(const std::string& path);

    bool showMenu;
    bool showSettings;
    bool showStateDialog;
    int selectingDiskForDrive; // -1: none, 0: Drive 1, 1: Drive 2
    int activeTab;
    int currentStateSlot;
    
    // UI state
    int windowScale;
    bool isFullscreen;
    bool basicModeEdit;
    bool windowScaleEdit;
    
    std::string lastOpenedPath[2];
    std::string stateMessage;
    std::string statePreviewPath;
    Texture2D statePreviewTexture;
    Font fontJp;
    bool resetPending;
};
