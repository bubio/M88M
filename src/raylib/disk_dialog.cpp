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
    showMenu(false), showSettings(false), activeTab(0), 
    windowScale(0), isFullscreen(false),
    basicModeEdit(false), windowScaleEdit(false)
{
    NFD_Init();
}

UIManager::~UIManager() {
    NFD_Quit();
}

void UIManager::Init() {}

void UIManager::Update(bool& shouldExit, PC88* pc88, CoreRunner* coreRunner) {
    if (IsKeyPressed(KEY_F12)) ToggleMenu();
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !showMenu) {
        if (GetMouseY() < GetScreenHeight() - 24) ToggleMenu();
    }
    if (showMenu && IsKeyPressed(KEY_ESCAPE)) ToggleMenu();
}

void UIManager::Draw(DiskManager* diskmgr, PC8801::Config& cfg, PC88* pc88, CoreRunner* coreRunner) {
    DrawStatusBar(diskmgr);

    if (showMenu) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.4f));
        if (showSettings) DrawSettings(cfg, pc88, coreRunner);
        else DrawMainMenu(diskmgr, pc88);
    }
}

void UIManager::DrawMainMenu(DiskManager* diskmgr, PC88* pc88) {
    float width = 320;
    float height = 400;
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    if (GuiWindowBox({ x, y, width, height }, "M88M Main Menu")) ToggleMenu();

    float btnY = y + 45;
    for (int i = 0; i < 2; i++) {
        int diskIdx = diskmgr->GetCurrentDisk(i);
        std::string label = (diskIdx >= 0) ? diskmgr->GetImageTitle(i, diskIdx) : (std::string("Drive ") + std::to_string(i + 1) + ": Empty");
        if (GuiButton({ x + 15, btnY, width - 70, 32 }, label.c_str())) OpenNativeDialog(diskmgr, i);
        if (GuiButton({ x + width - 50, btnY, 35, 32 }, "#11#")) diskmgr->Mount(i, "", false, 0, false);
        btnY += 42;
    }

    btnY += 10;
    if (GuiButton({ x + 15, btnY, width - 30, 32 }, "Reset PC-8801")) { pc88->Reset(); ToggleMenu(); }
    btnY += 42;
    if (GuiButton({ x + 15, btnY, width - 30, 32 }, "Settings")) showSettings = true;
    btnY += 42;
    if (GuiButton({ x + 15, btnY, width - 30, 32 }, "Resume")) ToggleMenu();
    btnY += 52;
    if (GuiButton({ x + 15, btnY, width - 30, 32 }, "Quit M88M")) CloseWindow(); 
}

void UIManager::DrawSettings(PC8801::Config& cfg, PC88* pc88, CoreRunner* coreRunner) {
    float width = 560;
    float height = 440;
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    if (GuiWindowBox({ x, y, width, height }, "Settings")) showSettings = false;

    float tabW = (width - 20) / 4;
    GuiToggleGroup({ x + 10, y + 40, tabW, 30 }, "System;Audio;Video;Input", &activeTab);

    float pY = y + 90;
    bool changed = false;

    if (activeTab == 0) { // System
        GuiLabel({ x + 20, pY, 120, 20 }, "CPU Clock:");
        
        bool is4 = (cfg.clock == 4);
        bool is8 = (cfg.clock == 8);
        
        bool new_is4 = is4;
        bool new_is8 = is8;
        
        GuiToggle({ x + 150, pY, 80, 24 }, "4MHz", &new_is4);
        GuiToggle({ x + 240, pY, 80, 24 }, "8MHz", &new_is8);
        
        if (new_is4 && !is4) { // 4MHz was just toggled ON
            cfg.clock = 4;
            changed = true;
        } else if (new_is8 && !is8) { // 8MHz was just toggled ON
            cfg.clock = 8;
            changed = true;
        }
        
        pY += 40;

        GuiLabel({ x + 20, pY, 120, 20 }, "BASIC Mode:");
        int modeIndex = 0;
        if (cfg.basicmode == PC8801::Config::N88V1) modeIndex = 0;
        else if (cfg.basicmode == PC8801::Config::N88V2) modeIndex = 1;
        else if (cfg.basicmode == PC8801::Config::N80) modeIndex = 2;
        else if (cfg.basicmode == PC8801::Config::N80V2) modeIndex = 3;

        if (GuiDropdownBox({ x + 150, pY, 220, 24 }, "N88 V1;N88 V2;N80 V1;N80 V2", &modeIndex, basicModeEdit)) {
            basicModeEdit = !basicModeEdit;
            if (!basicModeEdit) { 
                if (modeIndex == 0) cfg.basicmode = PC8801::Config::N88V1;
                else if (modeIndex == 1) cfg.basicmode = PC8801::Config::N88V2;
                else if (modeIndex == 2) cfg.basicmode = PC8801::Config::N80;
                else if (modeIndex == 3) cfg.basicmode = PC8801::Config::N80V2;
                changed = true; 
            }
        }
        
        pY += 40;
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
        const char* names[] = { "FM", "SSG", "ADPCM", "Rhythm", "BEEP" };
        int* vPtrs[] = { &cfg.volfm, &cfg.volssg, &cfg.voladpcm, &cfg.volrhythm, &cfg.volbd };
        for (int i = 0; i < 5; i++) {
            GuiLabel({ x + 20, pY, 100, 20 }, names[i]);
            float val = (float)*vPtrs[i];
            GuiSliderBar({ x + 120, pY, 280, 16 }, "0", "128", &val, 0, 128);
            if ((int)val != *vPtrs[i]) {
                *vPtrs[i] = (int)val;
                changed = true;
            }
            pY += 32;
        }
        
        pY += 10;
        bool lpf = (cfg.lpforder > 0);
        bool oldLpf = lpf;
        GuiCheckBox({ x + 120, pY, 20, 20 }, "Enable LPF (Low Pass Filter)", &lpf);
        if (lpf != oldLpf) {
            cfg.lpforder = lpf ? 8 : 0;
            changed = true;
        }
    }
    else if (activeTab == 2) { // Video
        GuiLabel({ x + 20, pY, 120, 20 }, "Window Scale:");
        if (GuiDropdownBox({ x + 150, pY, 220, 24 }, "1x (640x400);2x (1280x800);3x (1920x1200)", &windowScale, windowScaleEdit)) {
            windowScaleEdit = !windowScaleEdit;
            if (!windowScaleEdit) {
                int scale = windowScale + 1;
                SetWindowSize(640 * scale, (400 * scale) + 24);
            }
        }
        pY += 40;
        
        bool fs = isFullscreen;
        bool oldFs = fs;
        GuiCheckBox({ x + 150, pY, 20, 20 }, "Fullscreen", &fs);
        if (fs != oldFs) {
            isFullscreen = fs;
            ToggleFullscreen();
        }
        
        pY += 40;
        GuiLabel({ x + 20, pY, 300, 20 }, "CRT Filter / Scanlines: Coming Soon!");
    }
    else if (activeTab == 3) { // Input
        GuiLabel({ x + 20, pY, 350, 20 }, "Keyboard Layout: JIS (Standard)");
    }

    if (changed) {
        coreRunner->RequestConfigApply(cfg);
        Config::Save(cfg);
    }

    if (GuiButton({ x + width / 2 - 50, y + height - 50, 100, 30 }, "Back")) showSettings = false;
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
        diskmgr->Mount(drive, outPath, false, 0, false);
        NFD_FreePath(outPath);
    }
}
