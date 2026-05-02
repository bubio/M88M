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
#include <sys/stat.h>
#include <cctype>
#include <ctime>
#include <cstdio>

namespace {
struct StateSlotInfo {
    bool exists = false;
    std::string modified;
};

static std::string FormatTime(time_t t) {
    char buf[64] = {};
    struct tm tmv;
    if (localtime_s(&tmv, &t) == 0) {
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    }
    return buf[0] ? std::string(buf) : std::string("Unknown");
}

static StateSlotInfo ReadStateSlotInfo(const std::string& path) {
    StateSlotInfo info;
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return info;

    info.exists = true;
    info.modified = FormatTime(st.st_mtime);
    return info;
}
}

static bool ContainsJapanese(const std::string& s) {
    for (size_t i = 0; i < s.length(); i++) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 0x80) return true; // Any non-ASCII is likely Japanese in our context
    }
    return false;
}

static const char* GetFileNameOnly(const char* path) {
    const char* f1 = strrchr(path, '/');
    const char* f2 = strrchr(path, '\\');
    const char* f = (f1 > f2) ? f1 : f2;
    return f ? f + 1 : path;
}

UIManager::UIManager() :
    showMenu(false), showSettings(false), showStateDialog(false), selectingDiskForDrive(-1), activeTab(0),
    currentStateSlot(0),
    windowScale(0), isFullscreen(false),
    basicModeEdit(false), windowScaleEdit(false), resetPending(false)
{
    lastOpenedPath[0] = "";
    lastOpenedPath[1] = "";
    statePreviewTexture = {0};
    NFD_Init();
}

UIManager::~UIManager() {
    if (IsTextureValid(statePreviewTexture)) UnloadTexture(statePreviewTexture);
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
        else if (showStateDialog) showStateDialog = false;
        else if (showSettings) showSettings = false;
        else ToggleMenu(coreRunner);
    }
}

void UIManager::Draw(DiskManager* diskmgr, PC8801::Config& cfg, PC88* pc88, CoreRunner* coreRunner, bool& shouldExit) {
    DrawStatusBar(diskmgr);

    if (showMenu) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.4f));
        if (selectingDiskForDrive != -1) DrawDiskSelector(diskmgr);
        else if (showStateDialog) DrawStateDialog(diskmgr, coreRunner);
        else if (showSettings) DrawSettings(cfg, pc88, coreRunner);
        else DrawMainMenu(diskmgr, pc88, shouldExit, coreRunner);
    }
}

void UIManager::ToggleMenu(CoreRunner* coreRunner) {
    showMenu = !showMenu;
    if (!showMenu) {
        showSettings = false;
        showStateDialog = false;
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
    if (!lastOpenedPath[0].empty()) groupTitle = GetFileNameOnly(lastOpenedPath[0].c_str());
    else if (!lastOpenedPath[1].empty()) groupTitle = GetFileNameOnly(lastOpenedPath[1].c_str());

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
            const char* title = diskmgr->GetImageTitle(i, diskIdx);
            label = Paths::SJIStoUTF8(title ? std::string(title, 16) : "");
            size_t last = label.find_last_not_of(" \0", label.length());
            if (last != std::string::npos) label = label.substr(0, last + 1);
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
    btnY += 44;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "State Save / Load")) {
        showStateDialog = true;
        stateMessage.clear();
    }
    btnY += 34;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "Settings")) showSettings = true;

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
        std::string dLabel = dTitle ? Paths::SJIStoUTF8(std::string(dTitle, 16)) : "(No Title)";
        size_t last = dLabel.find_last_not_of(" \0", dLabel.length());
        if (last != std::string::npos) dLabel = dLabel.substr(0, last + 1);

        bool isJp = ContainsJapanese(dLabel);
        if (isJp && IsFontValid(fontJp)) {
            GuiSetFont(fontJp);
            GuiSetStyle(DEFAULT, TEXT_SIZE, 15);
        }

        std::string label = std::to_string(i + 1) + ": " + dLabel;

        if (GuiButton({ x + 10, btnY, width - 20, btnH }, label.c_str())) {
            diskmgr->Mount(selectingDiskForDrive, lastOpenedPath[selectingDiskForDrive].c_str(), false, i, false);
            selectingDiskForDrive = -1;
        }

        if (isJp) {
            GuiSetFont(GetFontDefault());
            GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
        }
        btnY += 30;
    }

    if (GuiButton({ x + width / 2 - 50, y + height - 40, 100, 28 }, "Back")) selectingDiskForDrive = -1;
}

static std::string SanitizeStateName(const std::string& in) {
    std::string out;
    for (unsigned char c : in) {
        if (std::isalnum(c) || c == '-' || c == '_') out.push_back((char)c);
        else if (c >= 0x80) out.push_back((char)c);
    }
    if (out.empty()) out = "snapshot";
    if (out.size() > 64) out.resize(64);
    return out;
}

std::string UIManager::GetStatePath(DiskManager* diskmgr, int slot) const {
    std::string dir = Paths::GetConfigDir() + "/states";
    struct stat st;
    if (stat(dir.c_str(), &st) != 0) mkdir(dir.c_str(), 0755);

    std::string title = "snapshot";
    int diskIdx = diskmgr->GetCurrentDisk(0);
    if (diskIdx >= 0) {
        title = Paths::SJIStoUTF8(diskmgr->GetImageTitle(0, diskIdx));
    } else if (!lastOpenedPath[0].empty()) {
        title = GetFileNameOnly(lastOpenedPath[0].c_str());
    }
    return dir + "/" + SanitizeStateName(title) + "_" + std::to_string(slot) + ".s88";
}

std::string UIManager::GetStateScreenshotPath(DiskManager* diskmgr, int slot) const {
    std::string path = GetStatePath(diskmgr, slot);
    size_t dot = path.find_last_of('.');
    if (dot != std::string::npos) path.resize(dot);
    return path + ".png";
}

bool UIManager::StateSlotExists(DiskManager* diskmgr, int slot) const {
    std::string path = GetStatePath(diskmgr, slot);
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

void UIManager::LoadStatePreview(const std::string& path) {
    if (statePreviewPath == path) return;
    if (IsTextureValid(statePreviewTexture)) {
        UnloadTexture(statePreviewTexture);
        statePreviewTexture = {0};
    }
    statePreviewPath = path;
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        statePreviewTexture = LoadTexture(path.c_str());
        if (IsTextureValid(statePreviewTexture)) {
            SetTextureFilter(statePreviewTexture, TEXTURE_FILTER_POINT);
        }
    }
}

static std::string GetCurrentDiskDisplayName(DiskManager* diskmgr, const std::string& fallbackPath) {
    int diskIdx = diskmgr->GetCurrentDisk(0);
    if (diskIdx >= 0) {
        return Paths::SJIStoUTF8(diskmgr->GetImageTitle(0, diskIdx));
    }
    if (!fallbackPath.empty()) {
        return GetFileNameOnly(fallbackPath.c_str());
    }
    return "snapshot";
}

void UIManager::DrawStateDialog(DiskManager* diskmgr, CoreRunner* coreRunner) {
    float width = 500;
    float height = 360;
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    if (GuiWindowBox({ x, y, width, height }, "State Save / Load")) {
        showStateDialog = false;
        return;
    }

    float slotY = y + 45;
    float slotW = 42;
    for (int i = 0; i < 10; i++) {
        bool exists = StateSlotExists(diskmgr, i);
        const char* label = TextFormat("%d%s", i, exists ? "*" : "");
        Rectangle r = { x + 20 + i * (slotW + 4), slotY, slotW, 30 };
        if (GuiButton(r, label)) {
            currentStateSlot = i;
            stateMessage.clear();
        }
        if (currentStateSlot == i) {
            DrawRectangleLinesEx({ r.x - 2, r.y - 2, r.width + 4, r.height + 4 }, 2, SKYBLUE);
        }
    }

    std::string path = GetStatePath(diskmgr, currentStateSlot);
    std::string screenshotPath = GetStateScreenshotPath(diskmgr, currentStateSlot);
    StateSlotInfo info = ReadStateSlotInfo(path);
    bool exists = info.exists;
    std::string diskName = GetCurrentDiskDisplayName(diskmgr, lastOpenedPath[0]);
    std::string fileLabel = "Slot " + std::to_string(currentStateSlot) + ": " + (exists ? info.modified : "Empty");
    GuiLabel({ x + 20, y + 85, width - 40, 22 }, fileLabel.c_str());

    LoadStatePreview(screenshotPath);
    float previewWidth = 220;
    Rectangle preview = { x + ((width - previewWidth) / 2), y + 112, previewWidth, previewWidth * 0.625f };
    DrawRectangleRec(preview, Color{ 20, 20, 20, 255 });
    DrawRectangleLinesEx(preview, 1, DARKGRAY);
    if (IsTextureValid(statePreviewTexture)) {
        DrawTexturePro(
            statePreviewTexture,
            { 0, 0, (float)statePreviewTexture.width, (float)statePreviewTexture.height },
            preview,
            { 0, 0 },
            0,
            WHITE
        );
    } else {
        GuiLabel({ preview.x + 58, preview.y + 58, 120, 20 }, "No Screenshot");
    }

    if (GuiButton({ x + 30, y + height - 122 + 26, 195, 28 }, "Save State")) {
        if (coreRunner) {
            coreRunner->SaveState(path, screenshotPath, &stateMessage);
            statePreviewPath.clear();
            LoadStatePreview(screenshotPath);
        }
    }

    GuiState statePush = (GuiState)GuiGetState();
    if (!exists) GuiSetState(STATE_DISABLED);
    if (GuiButton({ x + 275, y + height - 122 + 26, 195, 28 }, exists ? "Load State" : "No State")) {
        if (coreRunner && exists) coreRunner->LoadState(path, &stateMessage);
        else stateMessage = "No saved state in this slot";
    }
    GuiSetState(statePush);

    if (!stateMessage.empty()) {
        GuiLabel({ x + 30, y + height - 72, width - 60, 24 }, stateMessage.c_str());
    }

    if (GuiButton({ x + width / 2 - 50, y + height - 42, 100, 28 }, "Back")) {
        showStateDialog = false;
    }
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

    int d1 = diskmgr->GetCurrentDisk(1);
    const char* t1_raw = (d1 >= 0) ? diskmgr->GetImageTitle(1, d1) : nullptr;
    std::string t1 = (t1_raw) ? Paths::SJIStoUTF8(std::string(t1_raw, 16)) : "Empty";
    if (t1_raw) {
        size_t last = t1.find_last_not_of(" \0", t1.length());
        if (last != std::string::npos) t1 = t1.substr(0, last + 1);
    }

    Color l1 = (statusdisplay.GetFDState(1) & 1) ? RED : Color{ 60, 20, 20, 255 };
    DrawCircle(15, (int)centerY, 4, l1);

    DrawText("FDD2:", 25, (int)textY, 10, DARKGRAY);
    if (ContainsJapanese(t1) && IsFontValid(fontJp)) {
        DrawTextEx(fontJp, t1.c_str(), { 60, sH - 18 }, 14, 1, DARKGRAY);
    } else {
        DrawText(t1.c_str(), 60, (int)textY, 10, DARKGRAY);
    }

    int d0 = diskmgr->GetCurrentDisk(0);
    const char* t0_raw = (d0 >= 0) ? diskmgr->GetImageTitle(0, d0) : nullptr;
    std::string t0 = (d0 >= 0) ? Paths::SJIStoUTF8(std::string(t0_raw, 16)) : "Empty";
    if (t0_raw) {
        size_t last = t0.find_last_not_of(" \0", t0.length());
        if (last != std::string::npos) t0 = t0.substr(0, last + 1);
    }

    Color l0 = (statusdisplay.GetFDState(0) & 1) ? RED : Color{ 60, 20, 20, 255 };
    DrawCircle(215, (int)centerY, 4, l0);

    DrawText("FDD1:", 225, (int)textY, 10, DARKGRAY);
    if (ContainsJapanese(t0) && IsFontValid(fontJp)) {
        DrawTextEx(fontJp, t0.c_str(), { 260, sH - 18 }, 14, 1, DARKGRAY);
    } else {
        DrawText(t0.c_str(), 260, (int)textY, 10, DARKGRAY);
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
