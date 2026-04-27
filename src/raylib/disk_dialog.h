#pragma once

#include "diskmgr.h"
#include "pc88/config.h"
#include <string>

class UIManager {
public:
    UIManager();
    ~UIManager();

    void Init();
    void Update(bool& shouldExit, class PC88* pc88, class CoreRunner* coreRunner);
    void Draw(DiskManager* diskmgr, PC8801::Config& cfg, class PC88* pc88, class CoreRunner* coreRunner);
    void OpenNativeDialog(DiskManager* diskmgr, int drive);

    bool IsMenuOpen() const { return showMenu; }
    void ToggleMenu() { showMenu = !showMenu; if (!showMenu) showSettings = false; }

private:
    void DrawMainMenu(DiskManager* diskmgr, class PC88* pc88);
    void DrawSettings(PC8801::Config& cfg, class PC88* pc88, class CoreRunner* coreRunner);
    void DrawStatusBar(DiskManager* diskmgr);
    void DrawDriveStatus(DiskManager* diskmgr, int drive, float x, float y);

    bool showMenu;
    bool showSettings;
    int activeTab;
    
    // UI state
    int windowScale;
    bool isFullscreen;
    bool basicModeEdit;
    bool windowScaleEdit;
};
