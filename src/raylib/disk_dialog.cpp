#include "disk_dialog.h"
#include "raylib.h"
#include "raygui.h"
#include "nfd.h"
#include "config.h"
#include "pc88.h"
#include "status.h"
#include "core_runner.h"
#include <string>
#include <vector>

UIManager::UIManager() :
    showMenu(false), showSettings(false), selectingDiskForDrive(-1), activeTab(0),
    windowScale(0), isFullscreen(false),
    basicModeEdit(false), windowScaleEdit(false)
{
    lastOpenedPath[0] = "";
    lastOpenedPath[1] = "";
    NFD_Init();
}

UIManager::~UIManager() {
    NFD_Quit();
}

void UIManager::Init() {}

void UIManager::Update(bool& shouldExit, PC88* pc88, CoreRunner* coreRunner) {
    if (IsKeyPressed(KEY_F12)) ToggleMenu();
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showMenu) {
        if (GetMouseY() < GetScreenHeight() - 24) ToggleMenu();
    }
    if (showMenu && IsKeyPressed(KEY_ESCAPE)) {
        if (selectingDiskForDrive != -1) selectingDiskForDrive = -1;
        else if (showSettings) showSettings = false;
        else ToggleMenu();
    }
}

void UIManager::Draw(DiskManager* diskmgr, PC8801::Config& cfg, PC88* pc88, CoreRunner* coreRunner, bool& shouldExit) {
    DrawStatusBar(diskmgr);

    if (showMenu) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.4f));
        if (selectingDiskForDrive != -1) DrawDiskSelector(diskmgr);
        else if (showSettings) DrawSettings(cfg, pc88, coreRunner);
        else DrawMainMenu(diskmgr, pc88, shouldExit);
    }
}

void UIManager::DrawMainMenu(DiskManager* diskmgr, PC88* pc88, bool& shouldExit) {
    float width = 280;
    float height = 350; // Increased height for new buttons
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    if (GuiWindowBox({ x, y, width, height }, "M88M Main Menu")) ToggleMenu();

    float btnY = y + 40;
    float btnH = 26;

    // Drive 1&2 button
    if (GuiButton({ x + 10, btnY, width - 60, btnH }, "Drive 1&2 (Dual Mount)...")) OpenBothDrives(diskmgr);
    if (GuiButton({ x + width - 40, btnY, 30, btnH }, GuiIconText(ICON_FILE_DELETE, NULL))) {
        diskmgr->Unmount(0); diskmgr->Unmount(1);
        lastOpenedPath[0] = lastOpenedPath[1] = "";
    }
    btnY += 36;

    for (int i = 0; i < 2; i++) {
        int diskIdx = diskmgr->GetCurrentDisk(i);
        std::string label = (diskIdx >= 0) ? diskmgr->GetImageTitle(i, diskIdx) : (std::string("Drive ") + std::to_string(i + 1) + ": Empty");

        // Drive label / open dialog
        if (GuiButton({ x + 10, btnY, width - 90, btnH }, label.c_str())) OpenNativeDialog(diskmgr, i);

        // Disk selection button (only if multiple disks)
        bool hasMultiple = diskmgr->GetNumDisks(i) > 1;
        if (GuiButton({ x + width - 75, btnY, 30, btnH }, hasMultiple ? GuiIconText(ICON_FILE_COPY, NULL) : GuiIconText(ICON_FILE_OPEN, NULL))) {
            if (hasMultiple) selectingDiskForDrive = i;
            else OpenNativeDialog(diskmgr, i);
        }

        // Eject button
        if (GuiButton({ x + width - 40, btnY, 30, btnH }, GuiIconText(ICON_FILE_DELETE, NULL))) {
            diskmgr->Unmount(i);
            lastOpenedPath[i] = "";
        }
        btnY += 34;
    }

    btnY += 10;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "Reset PC-8801")) { pc88->Reset(); ToggleMenu(); }
    btnY += 34;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "Settings")) showSettings = true;
    btnY += 34;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "Resume")) ToggleMenu();

    // Move Quit to the bottom and set shouldExit flag instead of calling CloseWindow directly
    if (GuiButton({ x + 10, y + height - 40, width - 20, btnH }, "Quit M88M")) shouldExit = true;
    }void UIManager::OpenBothDrives(DiskManager* diskmgr) {
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "Disk Image", "d88,d77,88i,dim,dx9,784,dsk" } };
    if (NFD_OpenDialog(&outPath, filterItem, 1, NULL) == NFD_OKAY) {
        lastOpenedPath[0] = outPath;
        lastOpenedPath[1] = outPath;
        if (diskmgr->Mount(0, outPath, false, 0, false)) {
            if (diskmgr->GetNumDisks(0) > 1) {
                diskmgr->Mount(1, outPath, false, 1, false);
            }
        }
        NFD_FreePath(outPath);
    }
}

void UIManager::DrawDiskSelector(DiskManager* diskmgr) {
    float width = 320;
    float height = 340;
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    std::string title = "Select Disk for Drive " + std::to_string(selectingDiskForDrive + 1);
    if (GuiWindowBox({ x, y, width, height }, title.c_str())) selectingDiskForDrive = -1;

    int numDisks = diskmgr->GetNumDisks(selectingDiskForDrive);
    float btnY = y + 40;
    float btnH = 26;

    for (int i = 0; i < numDisks && i < 8; i++) {
        const char* dTitle = diskmgr->GetImageTitle(selectingDiskForDrive, i);
        std::string label = std::to_string(i + 1) + ": " + (dTitle ? dTitle : "(No Title)");

        if (GuiButton({ x + 10, btnY, width - 20, btnH }, label.c_str())) {
            diskmgr->Mount(selectingDiskForDrive, lastOpenedPath[selectingDiskForDrive].c_str(), false, i, false);
            selectingDiskForDrive = -1;
        }
        btnY += 30;
    }

    if (GuiButton({ x + width / 2 - 50, y + height - 40, 100, 28 }, "Back")) selectingDiskForDrive = -1;
}

void UIManager::DrawSettings(PC8801::Config& cfg, PC88* pc88, CoreRunner* coreRunner) {
    float width = 520;
    float height = 340;
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    if (GuiWindowBox({ x, y, width, height }, "Settings")) ToggleMenu();

    float tabW = (width - 20) / 5;
    GuiToggleGroup({ x + 10, y + 35, tabW, 26 }, "System;Audio;Video;Input;About", &activeTab);

    float pY = y + 75;
    float rowH = 32;
    bool changed = false;

    if (activeTab == 0) { // System
        GuiLabel({ x + 20, pY, 120, 20 }, "CPU Clock:");

        bool is4 = (cfg.clock == 4);
        bool is8 = (cfg.clock == 8);
        bool new_is4 = is4, new_is8 = is8;

        GuiToggle({ x + 150, pY, 70, 24 }, "4MHz", &new_is4);
        GuiToggle({ x + 225, pY, 70, 24 }, "8MHz", &new_is8);

        if (new_is4 && !is4) { 
            cfg.clock = 4; cfg.dipsw |= (1 << 5); cfg.mainsubratio = 1; changed = true; 
            coreRunner->RequestConfigApply(cfg, true); // Reset required
        } else if (new_is8 && !is8) {
            cfg.clock = 8; cfg.dipsw &= ~(1 << 5); cfg.mainsubratio = 2; changed = true;
            coreRunner->RequestConfigApply(cfg, true); // Reset required
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, 120, 20 }, "BASIC Mode:");
        int modeIndex = 0;
        if (cfg.basicmode == PC8801::Config::N88V1) modeIndex = 0;
        else if (cfg.basicmode == PC8801::Config::N88V2) modeIndex = 1;
        else if (cfg.basicmode == PC8801::Config::N80) modeIndex = 2;
        else if (cfg.basicmode == PC8801::Config::N80V2) modeIndex = 3;

        if (GuiDropdownBox({ x + 150, pY, 200, 24 }, "N88 V1;N88 V2;N80 V1;N80 V2", &modeIndex, basicModeEdit)) {
            basicModeEdit = !basicModeEdit;
            if (!basicModeEdit) {
                if (modeIndex == 0) cfg.basicmode = PC8801::Config::N88V1;
                else if (modeIndex == 1) cfg.basicmode = PC8801::Config::N88V2;
                else if (modeIndex == 2) cfg.basicmode = PC8801::Config::N80;
                else if (modeIndex == 3) cfg.basicmode = PC8801::Config::N80V2;
                changed = true;
                coreRunner->RequestConfigApply(cfg, true); // Reset required
            }
        }
        pY += rowH + 4;
        bool wait = (cfg.flags & PC8801::Config::enablewait) != 0;
        bool oldWait = wait;
        GuiCheckBox({ x + 150, pY, 20, 20 }, "Enable CPU Wait", &wait);
        if (wait != oldWait) {
            if (wait) cfg.flags |= PC8801::Config::enablewait;
            else cfg.flags &= ~PC8801::Config::enablewait;
            changed = true;
        }
    }
    else if (activeTab == 1) { // Audio
        // Master Volume at the top
        GuiLabel({ x + 20, pY, 80, 20 }, "MASTER");
        float mVal = (float)cfg.mastervol;
        GuiSliderBar({ x + 100, pY, 240, 16 }, "0", "128", &mVal, 0, 128);
        if ((int)mVal != cfg.mastervol) { cfg.mastervol = (int)mVal; changed = true; }
        pY += 35;

        // Individual sound sources (removed BEEP)
        const char* names[] = { "FM", "SSG", "ADPCM", "Rhythm" };
        int* vPtrs[] = { &cfg.volfm, &cfg.volssg, &cfg.voladpcm, &cfg.volrhythm };
        for (int i = 0; i < 4; i++) {
            GuiLabel({ x + 20, pY, 80, 20 }, names[i]);
            float val = (float)*vPtrs[i];
            GuiSliderBar({ x + 100, pY, 240, 16 }, "0", "128", &val, 0, 128);
            if ((int)val != *vPtrs[i]) { *vPtrs[i] = (int)val; changed = true; }
            pY += 28;
        }

        pY += 10;
        if (GuiButton({ x + 100, pY, 150, 24 }, "Reset to Defaults")) {
            cfg.volfm = 64; cfg.volssg = 64; cfg.voladpcm = 64; cfg.volrhythm = 64;
            // mastervol is excluded from reset as requested
            changed = true;
        }

        pY += 34;
        bool lpf = (cfg.lpforder > 0);
        bool oldLpf = lpf;
        GuiCheckBox({ x + 100, pY, 20, 20 }, "Enable LPF (Low Pass Filter)", &lpf);
        if (lpf != oldLpf) { cfg.lpforder = lpf ? 8 : 0; changed = true; }
    }
    else if (activeTab == 2) { // Video
        GuiLabel({ x + 20, pY, 120, 20 }, "Window Scale:");
        if (GuiDropdownBox({ x + 150, pY, 200, 24 }, "1x (640x400);2x (1280x800);3x (1920x1200)", &windowScale, windowScaleEdit)) {
            windowScaleEdit = !windowScaleEdit;
            if (!windowScaleEdit) {
                int scale = windowScale + 1;
                SetWindowSize(640 * scale, (400 * scale) + 24);
            }
        }
        pY += rowH + 4;
        bool fs = isFullscreen, oldFs = fs;
        GuiCheckBox({ x + 150, pY, 20, 20 }, "Fullscreen", &fs);
        if (fs != oldFs) { isFullscreen = fs; ToggleFullscreen(); }
        pY += rowH;
        GuiLabel({ x + 20, pY, 300, 20 }, "CRT Filter / Scanlines: Coming Soon!");
    }
    else if (activeTab == 3) { // Input
        GuiLabel({ x + 20, pY, 350, 20 }, "Keyboard Layout: JIS (Standard)");
    }
    else if (activeTab == 4) { // About
        GuiLabel({ x + 20, pY, 400, 20 }, "M88M - PC-8801 Emulator for Modern Platforms");
        pY += 25;
        GuiLabel({ x + 20, pY, 400, 20 }, "Version: 0.1.0-alpha");
        pY += 35;
        GuiLabel({ x + 20, pY, 400, 20 }, "Original M88: Copyright (C) cisc 1998-2003");
        pY += 20;
        GuiLabel({ x + 20, pY, 400, 20 }, "OPNA Emulation: fmgen by cisc");
        pY += 35;
        GuiLabel({ x + 20, pY, 400, 20 }, "Ported with Raylib & Raygui");
        pY += 20;
        GuiLabel({ x + 20, pY, 400, 20 }, "Contributor: Gemini / Claude Opus 4.7");
    }

    if (changed) {
        // Only settings that haven't already requested a reset will fall through here.
        // We use the current changed flag to save the config and apply non-critical changes.
        coreRunner->RequestConfigApply(cfg, false); 
        Config::Save(cfg);
    }
    if (GuiButton({ x + width / 2 - 50, y + height - 40, 100, 28 }, "Back")) showSettings = false;
}
void UIManager::DrawStatusBar(DiskManager* diskmgr) {
    float sW = (float)GetScreenWidth();
    float sH = (float)GetScreenHeight();
    GuiStatusBar({ 0, sH - 24, sW, 24 }, "");
    int d0 = diskmgr->GetCurrentDisk(0);
    const char* t0 = (d0 >= 0) ? diskmgr->GetImageTitle(0, d0) : "Empty";
    Color l0 = (statusdisplay.GetFDState(0) & 1) ? RED : DARKGRAY;
    DrawCircle(15, (int)sH - 12, 4, l0);
    DrawText(TextFormat("FDD1: %s", t0), 25, (int)sH - 18, 10, DARKGRAY);

    int d1 = diskmgr->GetCurrentDisk(1);
    const char* t1 = (d1 >= 0) ? diskmgr->GetImageTitle(1, d1) : "Empty";
    Color l1 = (statusdisplay.GetFDState(1) & 1) ? RED : DARKGRAY;
    DrawCircle(215, (int)sH - 12, 4, l1);
    DrawText(TextFormat("FDD2: %s", t1), 225, (int)sH - 18, 10, DARKGRAY);

    DrawText(TextFormat("%d FPS", GetFPS()), (int)sW - 80, (int)sH - 18, 10, DARKGRAY);
}

void UIManager::OpenNativeDialog(DiskManager* diskmgr, int drive) {
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "Disk Image", "d88,d77,88i,dim,dx9,784,dsk" } };
    if (NFD_OpenDialog(&outPath, filterItem, 1, NULL) == NFD_OKAY) {
        lastOpenedPath[drive] = outPath;
        if (diskmgr->Mount(drive, outPath, false, 0, false)) {
            if (diskmgr->GetNumDisks(drive) > 1) {
                selectingDiskForDrive = drive;
            }
        }
        NFD_FreePath(outPath);
    }
}
