#include "disk_dialog.h"
#include "raylib.h"
#include "raygui.h"
#include "nfd.h"
#include "config.h"
#include "pc88.h"
#include "paths.h"
#include "status.h"
#include "core_runner.h"
#include <string>
#include <vector>

static bool ContainsJapanese(const std::string& s) {
    for (unsigned char c : s) if (c >= 0x80) return true;
    return false;
}

UIManager::UIManager() :
    showMenu(false), showSettings(false), selectingDiskForDrive(-1), activeTab(0),
    windowScale(0), isFullscreen(false),
    basicModeEdit(false), windowScaleEdit(false), resetPending(false)
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
    if (IsKeyPressed(KEY_F10)) ToggleMenu(coreRunner);
    if (IsKeyPressed(KEY_F12)) {
        if (coreRunner) coreRunner->RequestReset();
        else pc88->Reset();
    }
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showMenu) {
        if (GetMouseY() < GetScreenHeight() - 24) ToggleMenu(coreRunner);
    }
    if (showMenu && IsKeyPressed(KEY_ESCAPE)) {
        if (selectingDiskForDrive != -1) selectingDiskForDrive = -1;
        else if (showSettings) showSettings = false;
        else ToggleMenu(coreRunner);
    }
}

void UIManager::Draw(DiskManager* diskmgr, PC8801::Config& cfg, PC88* pc88, CoreRunner* coreRunner, bool& shouldExit) {
    DrawStatusBar(diskmgr);

    if (showMenu) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.4f));
        if (selectingDiskForDrive != -1) DrawDiskSelector(diskmgr);
        else if (showSettings) DrawSettings(cfg, pc88, coreRunner);
        else DrawMainMenu(diskmgr, pc88, shouldExit, coreRunner);
    }
}

void UIManager::ToggleMenu(CoreRunner* coreRunner) {
    showMenu = !showMenu;
    if (!showMenu) {
        showSettings = false;
        // If a critical setting was changed while the menu was open,
        // apply the config with a reset request now that the menu is closing.
        if (resetPending && coreRunner) {
            coreRunner->RequestConfigApply(Config::Get(), true);
            resetPending = false;
        }
    }
}

void UIManager::DrawMainMenu(DiskManager* diskmgr, PC88* pc88, bool& shouldExit, CoreRunner* coreRunner) {
    float width = 280;
    float height = 380;
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    if (GuiWindowBox({ x, y, width, height }, "M88M Main Menu")) ToggleMenu(coreRunner);

    float btnY = y + 45;
    float btnH = 26;

    // --- Disk Drives Group ---
    std::string groupTitle = "Disk Drives";
    if (!lastOpenedPath[0].empty()) groupTitle = GetFileName(lastOpenedPath[0].c_str());
    else if (!lastOpenedPath[1].empty()) groupTitle = GetFileName(lastOpenedPath[1].c_str());

    bool groupJp = ContainsJapanese(groupTitle);
    if (groupJp && IsFontValid(fontJp)) {
        GuiSetFont(fontJp);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 15);
    }
    GuiGroupBox({ x + 10, btnY - 5, width - 20, 115 }, groupTitle.c_str());
    if (groupJp) {
        GuiSetFont(GetFontDefault());
        GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
    }

    // Drive 1&2 button
    if (GuiButton({ x + 20, btnY + 10, width - 85, btnH }, "Drive 1&2 (Dual Mount)...")) OpenBothDrives(diskmgr);
    if (GuiButton({ x + width - 50, btnY + 10, 30, btnH }, GuiIconText(ICON_FILE_DELETE, NULL))) {
        diskmgr->Unmount(0); diskmgr->Unmount(1);
        lastOpenedPath[0] = lastOpenedPath[1] = "";
    }
    btnY += 36;

    for (int i = 0; i < 2; i++) {
        int diskIdx = diskmgr->GetCurrentDisk(i);
        std::string label;
        if (diskIdx >= 0) {
            label = Paths::SJIStoUTF8(diskmgr->GetImageTitle(i, diskIdx));
        } else {
            label = std::string("Drive ") + std::to_string(i + 1) + ": Empty";
        }

        bool isJp = ContainsJapanese(label);
        if (isJp && IsFontValid(fontJp)) {
            GuiSetFont(fontJp);
            GuiSetStyle(DEFAULT, TEXT_SIZE, 15);
        }

        if (GuiButton({ x + 20, btnY + 10, width - 110, btnH }, label.c_str())) OpenNativeDialog(diskmgr, i);

        if (isJp) {
            GuiSetFont(GetFontDefault());
            GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
        }

        bool hasMultiple = diskmgr->GetNumDisks(i) > 1;
        if (GuiButton({ x + width - 85, btnY + 10, 30, btnH }, hasMultiple ? GuiIconText(ICON_FILE_COPY, NULL) : GuiIconText(ICON_FILE_OPEN, NULL))) {
            if (hasMultiple) selectingDiskForDrive = i;
            else OpenNativeDialog(diskmgr, i);
        }

        if (GuiButton({ x + width - 50, btnY + 10, 30, btnH }, GuiIconText(ICON_FILE_DELETE, NULL))) {
            diskmgr->Unmount(i);
            lastOpenedPath[i] = "";
        }
        btnY += 34;
    }

    btnY += 35; // Space after group box
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "Reset PC-8801")) {
        if (coreRunner) coreRunner->RequestReset();
        else pc88->Reset();
        ToggleMenu(coreRunner);
    }
    btnY += 34;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "Settings")) showSettings = true;
    btnY += 34;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "Resume")) ToggleMenu(coreRunner);

    if (GuiButton({ x + 10, y + height - 40, width - 20, btnH }, "Quit M88M")) shouldExit = true;
}

void UIManager::OpenBothDrives(DiskManager* diskmgr) {
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
        std::string label = std::to_string(i + 1) + ": " + (dTitle ? Paths::SJIStoUTF8(dTitle) : "(No Title)");

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
    float height = 390;
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    if (GuiWindowBox({ x, y, width, height }, "Settings")) ToggleMenu(coreRunner);

    float tabW = (width - 20) / 5;
    GuiToggleGroup({ x + 10, y + 35, tabW, 26 }, "System;Audio;Video;Input;About", &activeTab);

    float pY = y + 75;
    float rowH = 32;
    bool changed = false;

    if (activeTab == 0) { // System
        GuiLabel({ x + 20, pY, 120, 20 }, "CPU Clock:");

        bool is4 = (cfg.clock < 60);
        bool is8 = (cfg.clock >= 60);
        bool new_is4 = is4, new_is8 = is8;

        GuiToggle({ x + 150, pY, 70, 24 }, "4MHz", &new_is4);
        GuiToggle({ x + 225, pY, 70, 24 }, "8MHz", &new_is8);

        if (new_is4 && !is4) {
            cfg.clock = 40; cfg.dipsw |= (1 << 5); cfg.mainsubratio = 1; changed = true;
            resetPending = true;
        } else if (new_is8 && !is8) {
            cfg.clock = 80; cfg.dipsw &= ~(1 << 5); cfg.mainsubratio = 2; changed = true;
            resetPending = true;
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
                resetPending = true;
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

        // Individual sound sources group
        GuiGroupBox({ x + 10, pY - 5, width - 20, 165 }, "Mixer (Internal Sound Sources)");

        const char* names[] = { "FM", "SSG", "ADPCM", "Rhythm" };
        int* vPtrs[] = { &cfg.volfm, &cfg.volssg, &cfg.voladpcm, &cfg.volrhythm };
        for (int i = 0; i < 4; i++) {
            GuiLabel({ x + 25, pY + 10, 80, 20 }, names[i]);
            float val = (float)*vPtrs[i];
            GuiSliderBar({ x + 100, pY + 10, 240, 16 }, "-100dB", "+40dB", &val, -100, 40);
            if ((int)val != *vPtrs[i]) { *vPtrs[i] = (int)val; changed = true; }
            pY += 28;
        }

        pY += 10;
        if (GuiButton({ x + 100, pY + 10, 150, 24 }, "Reset to Defaults")) {
            cfg.volfm = 0; cfg.volssg = 0; cfg.voladpcm = 0; cfg.volrhythm = 0;
            cfg.volbd = 0; cfg.volsd = 0; cfg.voltop = 0;
            cfg.volhh = 0; cfg.voltom = 0; cfg.volrim = 0;
            changed = true;
        }

        pY += 50;
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
        bool sl = (cfg.flag2 & PC8801::Config::scanline) != 0;
        bool oldSl = sl;
        GuiCheckBox({ x + 150, pY, 20, 20 }, "Scanlines", &sl);
        if (sl != oldSl) {
            if (sl) cfg.flag2 |= PC8801::Config::scanline;
            else cfg.flag2 &= ~PC8801::Config::scanline;
            changed = true;
        }

        pY += rowH;
        bool isDigi = (cfg.flags & PC8801::Config::digitalpalette) != 0;
        bool oldDigi = isDigi;
        GuiCheckBox({ x + 150, pY, 20, 20 }, "Digital Monitor (8 Colors)", &isDigi);
        if (isDigi != oldDigi) {
            if (isDigi) cfg.flags |= PC8801::Config::digitalpalette;
            else cfg.flags &= ~PC8801::Config::digitalpalette;
            changed = true;
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, 300, 20 }, "CRT Filter: Coming Soon!");
    }
    else if (activeTab == 3) { // Input
        bool useArrow = (cfg.flags & PC8801::Config::usearrowfor10) != 0;
        bool oldUse = useArrow;
        GuiCheckBox({ x + 20, pY, 20, 20 }, "Use Numpad as Arrow Keys", &useArrow);
        if (useArrow != oldUse) {
            if (useArrow) cfg.flags |= PC8801::Config::usearrowfor10;
            else cfg.flags &= ~PC8801::Config::usearrowfor10;
            changed = true;
        }
        pY += rowH;
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
        coreRunner->RequestConfigApply(cfg, false);
        Config::Save(cfg);
    }
    if (GuiButton({ x + width / 2 - 50, y + height - 40, 100, 28 }, "Back")) showSettings = false;
}

void UIManager::DrawStatusBar(DiskManager* diskmgr) {
    float sW = (float)GetScreenWidth();
    float sH = (float)GetScreenHeight();
    GuiStatusBar({ 0, sH - 24, sW, 24 }, "");

    float centerY = sH - 12.0f;
    float textY = sH - 17.0f;

    int d0 = diskmgr->GetCurrentDisk(0);
    const char* t0_sjis = (d0 >= 0) ? diskmgr->GetImageTitle(0, d0) : "Empty";
    std::string t0 = (d0 >= 0) ? Paths::SJIStoUTF8(t0_sjis) : "Empty";
    Color l0 = (statusdisplay.GetFDState(0) & 1) ? RED : Color{ 60, 20, 20, 255 };
    DrawCircle(15, (int)centerY, 4, l0);

    DrawText("FDD1:", 25, (int)textY, 10, DARKGRAY);
    if (ContainsJapanese(t0) && IsFontValid(fontJp)) {
        DrawTextEx(fontJp, t0.c_str(), { 60, sH - 18 }, 14, 1, DARKGRAY);
    } else {
        DrawText(t0.c_str(), 60, (int)textY, 10, DARKGRAY);
    }

    int d1 = diskmgr->GetCurrentDisk(1);
    const char* t1_sjis = (d1 >= 0) ? diskmgr->GetImageTitle(1, d1) : "Empty";
    std::string t1 = (d1 >= 0) ? Paths::SJIStoUTF8(t1_sjis) : "Empty";
    Color l1 = (statusdisplay.GetFDState(1) & 1) ? RED : Color{ 60, 20, 20, 255 };
    DrawCircle(215, (int)centerY, 4, l1);

    DrawText("FDD2:", 225, (int)textY, 10, DARKGRAY);
    if (ContainsJapanese(t1) && IsFontValid(fontJp)) {
        DrawTextEx(fontJp, t1.c_str(), { 260, sH - 18 }, 14, 1, DARKGRAY);
    } else {
        DrawText(t1.c_str(), 260, (int)textY, 10, DARKGRAY);
    }

    const auto& cfg = Config::Get();
    std::string modeStr = "Unknown";
    switch (cfg.basicmode) {
        case PC8801::Config::N88V1:  modeStr = "N88 V1"; break;
        case PC8801::Config::N88V2:  modeStr = "N88 V2"; break;
        case PC8801::Config::N80:    modeStr = "N80 V1"; break;
        case PC8801::Config::N80V2:  modeStr = "N80 V2"; break;
        case PC8801::Config::N88V2CD: modeStr = "N88 V2CD"; break;
        default: break;
    }

    std::string infoStr = TextFormat("[%s] %dMHz", modeStr.c_str(), cfg.clock / 10);
    DrawText(infoStr.c_str(), (int)sW - 200, (int)textY, 10, DARKGRAY);

    DrawText(TextFormat("%d FPS", GetFPS()), (int)sW - 80, (int)textY, 10, DARKGRAY);
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
