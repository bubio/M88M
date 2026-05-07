#include "disk_dialog.h"
#include "raylib.h"
#define RAYGUI_MALLOC(sz) malloc(sz)
#include "raygui.h"
#include "style_cyber.h"
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
    if (localtime_r(&t, &tmv)) {
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
        if (c >= 0x80) return true;
    }
    return false;
}

static const char* GetFileNameOnly(const char* path) {
    const char* f1 = strrchr(path, '/');
    const char* f2 = strrchr(path, '\\');
    const char* f = (f1 > f2) ? f1 : f2;
    return f ? f + 1 : path;
}

static std::string GetDirFromPath(const std::string& path) {
    size_t last = path.find_last_of("/\\");
    if (last != std::string::npos) {
        return path.substr(0, last);
    }
    return "";
}

UIManager::UIManager() :
    showMenu(false), modalState(MODAL_NONE), showSettings(false), showStateDialog(false), selectingDiskForDrive(-1), activeTab(0),
    currentStateSlot(0),
    diskScrollOffset({ 0, 0 }),
    windowScale(0), isFullscreen(false),
    basicModeEdit(false), windowScaleEdit(false),
    cpuModeEdit(false), port44Edit(false), portA8Edit(false), samplingEdit(false), keyboardEdit(false),
    speedEdit(false), eramEdit(false), bufferEdit(false), masterVolEdit(false),
    volFmEdit(false), volSsgEdit(false), volAdpcmEdit(false), volRhythmEdit(false),
    mouseSensEdit(false),
    bufferVal(100), mouseSensVal(10),
    mixerScroll({ 0, 0 }),
    inputScroll({ 0, 0 }),
    resetPending(false)
{
    for (int i = 0; i < 6; i++) volRhythmDetailEdit[i] = false;
    lastOpenedPath[0] = "";
    lastOpenedPath[1] = "";
    statePreviewTexture = {0};
    fontJp = {0};
    fontEn = {0};
    NFD_Init();
}

UIManager::~UIManager() {
    if (IsTextureValid(statePreviewTexture)) UnloadTexture(statePreviewTexture);
    NFD_Quit();
}

void UIManager::DrawEnText(const char* text, int x, int y, Color color) const {
    if (IsFontValid(fontEn)) {
        DrawTextEx(fontEn, text, { (float)x, (float)y - 1 }, 16, 1, color);
    } else {
        DrawText(text, x, y, 10, color);
    }
}

static Color StyleColor(int control, int prop) {
    return GetColor((unsigned)GuiGetStyle(control, prop));
}

static Color StyleFade(int control, int prop, float alpha) {
    Color c = StyleColor(control, prop);
    c.a = (unsigned char)(alpha * 255);
    return c;
}

int UIManager::MeasureEnText(const char* text) const {
    if (IsFontValid(fontEn)) {
        return (int)MeasureTextEx(fontEn, text, 16, 1).x;
    }
    return MeasureText(text, 10);
}

void UIManager::Init() {
    GuiLoadStyleCyber();

    // "FD" DOS style theme overrides
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, 0x000044f0); // Deep Navy Blue
    GuiSetStyle(DEFAULT, LINE_COLOR, 0x00ffffff);       // Bright Cyan Lines

    // Normal state
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, 0x00afafff);
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0x000044ff);   // Navy base
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0xffffffff);   // White text

    // Focused (Hover) state - Magenta highlight like FD
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, 0x00ffffff);
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, 0xcc00ccff);   // Magenta background
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, 0xffffffff);

    // Pressed state
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, 0xffffffff);
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, 0x00aaaaff);   // Cyan-ish
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, 0x000000ff);   // Black text on cyan

    if (IsFontValid(fontEn)) {
        GuiSetFont(fontEn);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
        GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
    }

    int h = GetScreenHeight();
    if (h >= 1224) windowScale = 2;
    else if (h >= 824) windowScale = 1;
    else windowScale = 0;
}

void UIManager::Update(bool& shouldExit, PC88* pc88, CoreRunner* coreRunner) {
    if (IsKeyPressed(KEY_F10)) ToggleMenu(coreRunner);
    if (IsKeyPressed(KEY_F12)) {
        if (Config::Get().flags & PC8801::Config::askbeforereset) {
            modalState = MODAL_CONFIRM_RESET;
            showMenu = true;
        } else {
            if (coreRunner) coreRunner->RequestReset();
            else pc88->Reset();
        }
    }
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !showMenu) {
        if (GetMouseY() < GetScreenHeight() - 24) ToggleMenu(coreRunner);
    }
    if (showMenu && IsKeyPressed(KEY_ESCAPE)) {
        if (modalState != MODAL_NONE) modalState = MODAL_NONE;
        else if (selectingDiskForDrive != -1) selectingDiskForDrive = -1;
        else if (showStateDialog) showStateDialog = false;
        else if (showSettings) showSettings = false;
        else ToggleMenu(coreRunner);
    }
}

void UIManager::Draw(DiskManager* diskmgr, PC8801::Config& cfg, PC88* pc88, CoreRunner* coreRunner, bool& shouldExit) {
    DrawStatusBar(diskmgr);

    if (showMenu) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), StyleFade(DEFAULT, BACKGROUND_COLOR, 0.4f));
        if (modalState != MODAL_NONE) DrawConfirmDialog(shouldExit, pc88, coreRunner);
        else if (selectingDiskForDrive != -1) DrawDiskSelector(diskmgr);
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

    std::string groupTitle = "Disk Drives";
    if (!lastOpenedPath[0].empty()) groupTitle = GetFileNameOnly(lastOpenedPath[0].c_str());
    else if (!lastOpenedPath[1].empty()) groupTitle = GetFileNameOnly(lastOpenedPath[1].c_str());

    bool groupJp = ContainsJapanese(groupTitle);
    if (groupJp && IsFontValid(fontJp)) {
        GuiSetFont(fontJp);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 15);
    }
    GuiGroupBox({ x + 10, btnY - 5, width - 20, 125 }, groupTitle.c_str());
    if (groupJp) {
        if (IsFontValid(fontEn)) GuiSetFont(fontEn); else GuiSetFont(GetFontDefault());
        GuiSetStyle(DEFAULT, TEXT_SIZE, IsFontValid(fontEn) ? 16 : 10);
    }

    if (GuiButton({ x + 20, btnY + 10, width - 85, btnH }, "Drive 1&2...")) OpenBothDrives(diskmgr);
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
            if (IsFontValid(fontEn)) GuiSetFont(fontEn); else GuiSetFont(GetFontDefault());
            GuiSetStyle(DEFAULT, TEXT_SIZE, IsFontValid(fontEn) ? 16 : 10);
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

    btnY += 35;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "Reset")) {
        if (Config::Get().flags & PC8801::Config::askbeforereset) {
            modalState = MODAL_CONFIRM_RESET;
        } else {
            if (coreRunner) coreRunner->RequestReset();
            else pc88->Reset();
            ToggleMenu(coreRunner);
        }
    }
    btnY += 54;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "State Save / Load")) {
        showStateDialog = true;
        stateMessage.clear();
    }
    btnY += 44;
    if (GuiButton({ x + 10, btnY, width - 20, btnH }, "Settings")) showSettings = true;

    if (GuiButton({ x + 10, y + height - 40, width - 20, btnH }, "Quit")) {
        if (Config::Get().flags & PC8801::Config::askbeforereset) {
            modalState = MODAL_CONFIRM_QUIT;
        } else {
            shouldExit = true;
        }
    }
}

void UIManager::OpenBothDrives(DiskManager* diskmgr) {
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "Disk Image", "d88,d77,88i,dim,dx9,784,dsk" } };

    std::string defaultPathStr;
    const nfdchar_t* defaultPath = NULL;
    if (Config::Get().flags & PC8801::Config::savedirectory) {
        if (!lastOpenedPath[0].empty()) {
            defaultPathStr = GetDirFromPath(lastOpenedPath[0]);
            if (!defaultPathStr.empty()) defaultPath = defaultPathStr.c_str();
        } else if (!lastOpenedPath[1].empty()) {
            defaultPathStr = GetDirFromPath(lastOpenedPath[1]);
            if (!defaultPathStr.empty()) defaultPath = defaultPathStr.c_str();
        }
    }

    if (NFD_OpenDialog(&outPath, filterItem, 1, defaultPath) == NFD_OKAY) {
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

    Rectangle view = { x + 10, btnY, width - 20, height - 90 };
    Rectangle content = { 0, 0, width - 40, (float)numDisks * (btnH + 4) };

    GuiScrollPanel(view, NULL, { 0, 0, content.width, content.height }, &diskScrollOffset, &view);

    BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
    for (int i = 0; i < numDisks; i++) {
        float itemY = view.y + i * (btnH + 4) + diskScrollOffset.y;
        if (itemY + btnH < view.y || itemY > view.y + view.height) continue;

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

        if (GuiButton({ view.x, itemY, content.width, btnH }, label.c_str())) {
            diskmgr->Mount(selectingDiskForDrive, diskmgr->GetImageTitle(selectingDiskForDrive, i), true, i, true);
            selectingDiskForDrive = -1;
        }

        if (isJp && IsFontValid(fontEn)) {
            GuiSetFont(fontEn);
            GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
        }
    }
    EndScissorMode();

    if (GuiButton({ x + width - 120, y + height - 40, 100, 28 }, "Back")) selectingDiskForDrive = -1;
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
            DrawRectangleLinesEx({ r.x - 2, r.y - 2, r.width + 4, r.height + 4 }, 2, StyleColor(BUTTON, BORDER_COLOR_FOCUSED));
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
    DrawRectangleRec(preview, StyleColor(DEFAULT, BACKGROUND_COLOR));
    DrawRectangleLinesEx(preview, 1, StyleColor(DEFAULT, LINE_COLOR));
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

    if (GuiButton({ x + width - 120, y + height - 42, 100, 28 }, "Back")) {
        showStateDialog = false;
    }
}

void UIManager::DrawSettings(PC8801::Config& cfg, PC88* pc88, CoreRunner* coreRunner) {
    if (!bufferEdit) bufferVal = (int)cfg.soundbuffer;
    if (!mouseSensEdit) mouseSensVal = (int)cfg.mousesensibility;

    bool isDropdownOpen = basicModeEdit || cpuModeEdit || port44Edit || portA8Edit || samplingEdit || windowScaleEdit || keyboardEdit;
    bool isValueBoxActive = speedEdit || eramEdit || bufferEdit || masterVolEdit || volFmEdit || volSsgEdit || volAdpcmEdit || volRhythmEdit;
    for (int i = 0; i < 6; i++) if (volRhythmDetailEdit[i]) isValueBoxActive = true;
    bool anyEdit = isDropdownOpen || isValueBoxActive;

    float width = 540;
    float height = 400;
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    Rectangle ddRect = { 0 };
    const char* ddText = nullptr;
    int* ddIndexPtr = nullptr;
    bool* ddEditPtr = nullptr;

    if (GuiWindowBox({ x, y, width, height }, "Settings")) {
        ToggleMenu(coreRunner);
        return;
    }

    int prevTab = activeTab;
    float tabW = (width - 40) / 6;
    GuiToggleGroup({ x + 20, y + 35, tabW, 26 }, "System;Audio;Video;Mixer;Input;About", &activeTab);

    if (activeTab != prevTab) {
        basicModeEdit = cpuModeEdit = port44Edit = portA8Edit = samplingEdit = windowScaleEdit = keyboardEdit = false;
        speedEdit = eramEdit = bufferEdit = masterVolEdit = volFmEdit = volSsgEdit = volAdpcmEdit = volRhythmEdit = false;
        mouseSensEdit = false;
        for (int i = 0; i < 6; i++) volRhythmDetailEdit[i] = false;
        anyEdit = false;
        return; // Prevent click-through to new tab items in the same frame
    }

    if (anyEdit) {
        GuiLock();
        DrawRectangleRec({ x + 2, y + 65, width - 4, height - 67 }, Fade(BLACK, 0.3f));
    }

    float pY = y + 75;
    float rowH = 32;
    bool changed = false;

    if (activeTab == 0) { // System
        float labelW = 160;
        GuiLabel({ x + 20, pY, labelW, 20 }, "CPU Clock:");
        int clockMode = (cfg.clock < 60) ? 0 : 1;
        int newClock = clockMode;
        GuiToggleGroup({ x + 180, pY, 150, 24 }, "4MHz;8MHz", &newClock);
        if (newClock != clockMode) {
            if (newClock == 0) { cfg.clock = 40; cfg.dipsw |= (1 << 5); cfg.mainsubratio = 1; }
            else { cfg.clock = 80; cfg.dipsw &= ~(1 << 5); cfg.mainsubratio = 2; }
            changed = true; resetPending = true;
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, labelW, 20 }, "Speed:");
        float speedValF = (float)cfg.speed;
        if (GuiSlider({ x + 180, pY, 180, 16 }, NULL, NULL, &speedValF, 20, 200)) {
            cfg.speed = (int)speedValF; changed = true;
        }
        bool wasUnl = false; if (speedEdit) { GuiUnlock(); wasUnl = true; }
        if (GuiValueBox({ x + 370, pY, 50, 16 }, NULL, &cfg.speed, 20, 200, speedEdit)) {
            speedEdit = !speedEdit; if (!speedEdit) changed = true;
        }
        if (wasUnl) GuiLock();
        GuiLabel({ x + 425, pY, 40, 20 }, "%");

        pY += rowH;
        GuiLabel({ x + 20, pY, labelW, 20 }, "BASIC Mode:");
        static int modeIndex;
        if (!basicModeEdit) {
            if (cfg.basicmode == PC8801::Config::N88V1) modeIndex = 0;
            else if (cfg.basicmode == PC8801::Config::N88V2) modeIndex = 1;
            else if (cfg.basicmode == PC8801::Config::N80) modeIndex = 2;
            else if (cfg.basicmode == PC8801::Config::N80V2) modeIndex = 3;
        }
        Rectangle basicRect = { x + 180, pY, 200, 24 };
        if (basicModeEdit) { ddRect = basicRect; ddText = "N88 V1;N88 V2;N80 V1;N80 V2"; ddIndexPtr = &modeIndex; ddEditPtr = &basicModeEdit; }
        else if (GuiDropdownBox(basicRect, "N88 V1;N88 V2;N80 V1;N80 V2", &modeIndex, false)) basicModeEdit = true;

        pY += rowH + 4;
        GuiLabel({ x + 20, pY, labelW, 20 }, "CPU Mode:");
        static int cpumode;
        if (!cpuModeEdit) cpumode = cfg.cpumode;
        Rectangle cpuRect = { x + 180, pY, 200, 24 };
        if (cpuModeEdit) { ddRect = cpuRect; ddText = "Main 1 : Sub 1;Main 2 : Sub 1;Auto"; ddIndexPtr = &cpumode; ddEditPtr = &cpuModeEdit; }
        else if (GuiDropdownBox(cpuRect, "Main 1 : Sub 1;Main 2 : Sub 1;Auto", &cpumode, false)) cpuModeEdit = true;

        pY += rowH + 4;
        GuiLabel({ x + 20, pY, labelW, 20 }, "CPU Wait:");
        int waitVal = (cfg.flags & PC8801::Config::enablewait) ? 1 : 0;
        if (GuiToggleSlider({ x + 180, pY, 60, 20 }, "OFF;ON", &waitVal)) {
            if (waitVal) cfg.flags |= PC8801::Config::enablewait; else cfg.flags &= ~PC8801::Config::enablewait; changed = true;
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, labelW, 20 }, "FDD Wait:");
        int fddwaitVal = (cfg.flag2 & PC8801::Config::fddnowait) ? 0 : 1;
        if (GuiToggleSlider({ x + 180, pY, 60, 20 }, "OFF;ON", &fddwaitVal)) {
            if (fddwaitVal) cfg.flag2 &= ~PC8801::Config::fddnowait; else cfg.flag2 |= PC8801::Config::fddnowait; changed = true;
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, labelW, 20 }, "ERAM:");
        float eramValF = (float)cfg.erambanks;
        if (GuiSlider({ x + 180, pY, 180, 16 }, NULL, NULL, &eramValF, 0, 128)) {
            cfg.erambanks = (int)eramValF; changed = true; resetPending = true;
        }
        wasUnl = false; if (eramEdit) { GuiUnlock(); wasUnl = true; }
        if (GuiValueBox({ x + 370, pY, 50, 16 }, NULL, &cfg.erambanks, 0, 128, eramEdit)) {
            eramEdit = !eramEdit; if (!eramEdit) { changed = true; resetPending = true; }
        }
        if (wasUnl) GuiLock();
        GuiLabel({ x + 425, pY, 80, 20 }, "x 32KB");

        pY += rowH;
        GuiLabel({ x + 20, pY, labelW, 20 }, "Ask Before Reset:");
        int askVal = (cfg.flags & PC8801::Config::askbeforereset) ? 1 : 0;
        if (GuiToggleSlider({ x + 180, pY, 60, 20 }, "OFF;ON", &askVal)) {
            if (askVal) cfg.flags |= PC8801::Config::askbeforereset; else cfg.flags &= ~PC8801::Config::askbeforereset; changed = true;
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, labelW, 20 }, "Save Last Dir:");
        int saveDirVal = (cfg.flags & PC8801::Config::savedirectory) ? 1 : 0;
        if (GuiToggleSlider({ x + 180, pY, 60, 20 }, "OFF;ON", &saveDirVal)) {
            if (saveDirVal) cfg.flags |= PC8801::Config::savedirectory; else cfg.flags &= ~PC8801::Config::savedirectory; changed = true;
        }
    }
    else if (activeTab == 1) { // Audio
        GuiLabel({ x + 20, pY, 120, 20 }, "Port 44h:");
        Rectangle p44Rect = { x + 150, pY, 120, 24 };
        static int p44;
        if (!port44Edit) p44 = (cfg.flag2 & PC8801::Config::disableopn44) ? 0 : ((cfg.flags & PC8801::Config::enableopna) ? 2 : 1);
        if (port44Edit) { ddRect = p44Rect; ddText = "None;OPN;OPNA"; ddIndexPtr = &p44; ddEditPtr = &port44Edit; }
        else if (GuiDropdownBox(p44Rect, "None;OPN;OPNA", &p44, false)) port44Edit = true;

        pY += rowH + 4;
        GuiLabel({ x + 20, pY, 120, 20 }, "Port A8h:");
        Rectangle pa8Rect = { x + 150, pY, 120, 24 };
        static int pa8;
        if (!portA8Edit) pa8 = (cfg.flags & PC8801::Config::opnaona8) ? 2 : ((cfg.flags & PC8801::Config::opnona8) ? 1 : 0);
        if (portA8Edit) { ddRect = pa8Rect; ddText = "None;OPN;OPNA"; ddIndexPtr = &pa8; ddEditPtr = &portA8Edit; }
        else if (GuiDropdownBox(pa8Rect, "None;OPN;OPNA", &pa8, false)) portA8Edit = true;

        pY += rowH + 4;
        GuiLabel({ x + 20, pY, 120, 20 }, "Sampling:");
        Rectangle sIdxRect = { x + 150, pY, 120, 24 };
        static int sIdx;
        if (!samplingEdit) { if (cfg.sound == 44100) sIdx = 0; else if (cfg.sound == 48000) sIdx = 1; else if (cfg.sound == 55467) sIdx = 2; else if (cfg.sound == 96000) sIdx = 3; }
        if (samplingEdit) { ddRect = sIdxRect; ddText = "44k;48k;55k;96k"; ddIndexPtr = &sIdx; ddEditPtr = &samplingEdit; }
        else if (GuiDropdownBox(sIdxRect, "44k;48k;55k;96k", &sIdx, false)) samplingEdit = true;

        pY += rowH + 4;
        GuiLabel({ x + 20, pY, 120, 20 }, "Buffer (ms):");
        float bufValF = (float)cfg.soundbuffer;
        if (GuiSlider({ x + 150, pY, 180, 16 }, NULL, NULL, &bufValF, 50, 500)) {
            cfg.soundbuffer = (uint)bufValF; changed = true; resetPending = true;
        }
        bool wasUnl = false; if (bufferEdit) { GuiUnlock(); wasUnl = true; }
        if (GuiValueBox({ x + 340, pY, 50, 16 }, NULL, &bufferVal, 50, 500, bufferEdit)) {
            bufferEdit = !bufferEdit; if (!bufferEdit) { cfg.soundbuffer = (uint)bufferVal; changed = true; resetPending = true; }
        }
        if (wasUnl) GuiLock();
        GuiLabel({ x + 395, pY, 60, 20 }, "ms");

        pY += rowH;
        GuiLabel({ x + 20, pY, 120, 20 }, "LPF:");
        int lpfVal = (cfg.flag2 & PC8801::Config::lpfenable) ? 1 : 0;
        if (GuiToggleSlider({ x + 150, pY, 60, 20 }, "OFF;ON", &lpfVal)) {
            if (lpfVal) cfg.flag2 |= PC8801::Config::lpfenable; else cfg.flag2 &= ~PC8801::Config::lpfenable; changed = true;
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, 120, 20 }, "Precise Mix:");
        int precVal = (cfg.flags & PC8801::Config::precisemixing) ? 1 : 0;
        if (GuiToggleSlider({ x + 150, pY, 60, 20 }, "OFF;ON", &precVal)) {
            if (precVal) cfg.flags |= PC8801::Config::precisemixing; else cfg.flags &= ~PC8801::Config::precisemixing; changed = true;
        }
    }
    else if (activeTab == 2) { // Video
        GuiLabel({ x + 20, pY, 120, 20 }, "Window Scale:");
        Rectangle scaleRect = { x + 150, pY, 200, 24 };
        if (windowScaleEdit) { ddRect = scaleRect; ddText = "1x (640x400);2x (1280x800);3x (1920x1200)"; ddIndexPtr = &windowScale; ddEditPtr = &windowScaleEdit; }
        else if (GuiDropdownBox(scaleRect, "1x (640x400);2x (1280x800);3x (1920x1200)", &windowScale, false)) windowScaleEdit = true;

        pY += rowH + 4;
        GuiLabel({ x + 20, pY, 120, 20 }, "Fullscreen:");
        int fsVal = isFullscreen ? 1 : 0;
        if (GuiToggleSlider({ x + 150, pY, 60, 20 }, "OFF;ON", &fsVal)) {
            isFullscreen = (fsVal == 1); ToggleFullscreen();
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, 120, 20 }, "FullLine:");
        int flVal = (cfg.flags & PC8801::Config::fullline) ? 1 : 0;
        if (GuiToggleSlider({ x + 150, pY, 60, 20 }, "OFF;ON", &flVal)) {
            if (flVal) cfg.flags |= PC8801::Config::fullline; else cfg.flags &= ~PC8801::Config::fullline; changed = true;
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, 120, 20 }, "15KHz Mode:");
        int fv15Val = (cfg.flags & PC8801::Config::fv15k) ? 1 : 0;
        if (GuiToggleSlider({ x + 150, pY, 60, 20 }, "OFF;ON", &fv15Val)) {
            if (fv15Val) cfg.flags |= PC8801::Config::fv15k; else cfg.flags &= ~PC8801::Config::fv15k; changed = true;
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, 120, 20 }, "Digital Pal:");
        int digiVal = (cfg.flags & PC8801::Config::digitalpalette) ? 1 : 0;
        if (GuiToggleSlider({ x + 150, pY, 60, 20 }, "OFF;ON", &digiVal)) {
            if (digiVal) cfg.flags |= PC8801::Config::digitalpalette; else cfg.flags &= ~PC8801::Config::digitalpalette; changed = true;
        }

        pY += rowH;
        GuiLabel({ x + 20, pY, 120, 20 }, "PCG-8100:");
        int pcgVal = (cfg.flags & PC8801::Config::enablepcg) ? 1 : 0;
        if (GuiToggleSlider({ x + 150, pY, 60, 20 }, "OFF;ON", &pcgVal)) {
            if (pcgVal) cfg.flags |= PC8801::Config::enablepcg; else cfg.flags &= ~PC8801::Config::enablepcg; changed = true; resetPending = true;
        }
    }
    else if (activeTab == 3) { // Mixer
        Rectangle scrollBounds = { x + 10, y + 70, width - 20, height - 120 };
        Rectangle content = { 0, 0, width - 40, 520 };
        Rectangle view;
        if (!anyEdit && CheckCollisionPointRec(GetMousePosition(), scrollBounds)) mixerScroll.y += GetMouseWheelMove() * 20;
        GuiScrollPanel(scrollBounds, NULL, content, &mixerScroll, &view);
        if (mixerScroll.y > 0) mixerScroll.y = 0;
        if (mixerScroll.y < (view.height - content.height)) mixerScroll.y = view.height - content.height;

        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
            float sX = view.x + mixerScroll.x;
            float sY = view.y + mixerScroll.y;
            float curY = sY + 10;
            GuiLabel({ sX + 10, curY, 80, 20 }, "MASTER");
            float mValF = (float)cfg.mastervol;
            if (masterVolEdit) GuiSetState(STATE_DISABLED);
            if (GuiSlider({ sX + 90, curY, 240, 16 }, NULL, NULL, &mValF, 0, 128)) { cfg.mastervol = (int)mValF; changed = true; }
            bool wasUnl = false; if (masterVolEdit) { GuiUnlock(); wasUnl = true; }
            if (GuiValueBox({ sX + 340, curY, 50, 16 }, NULL, &cfg.mastervol, 0, 128, masterVolEdit)) {
                masterVolEdit = !masterVolEdit; if (!masterVolEdit) changed = true;
            }
            if (wasUnl) GuiLock();
            curY += 55;
            float group1H = 170;
            GuiGroupBox({ sX + 5, curY - 5, width - 65, group1H }, "Sound Sources (FM / SSG / ADPCM / Rhythm)");
            float box1Y = curY;
            const char* names[] = { "FM", "SSG", "ADPCM", "Rhythm" };
            int* vPtrs[] = { &cfg.volfm, &cfg.volssg, &cfg.voladpcm, &cfg.volrhythm };
            bool* ePtrs[] = { &volFmEdit, &volSsgEdit, &volAdpcmEdit, &volRhythmEdit };
            for (int i = 0; i < 4; i++) {
                GuiLabel({ sX + 20, box1Y + 10, 80, 20 }, names[i]);
                float valF = (float)*vPtrs[i];
                if (*ePtrs[i]) GuiSetState(STATE_DISABLED);
                if (GuiSlider({ sX + 90, box1Y + 10, 240, 16 }, NULL, NULL, &valF, -100, 40)) { *vPtrs[i] = (int)valF; changed = true; }
                wasUnl = false; if (*ePtrs[i]) { GuiUnlock(); wasUnl = true; }
                if (GuiValueBox({ sX + 340, box1Y + 10, 50, 16 }, NULL, vPtrs[i], -100, 40, *ePtrs[i])) {
                    *ePtrs[i] = !*ePtrs[i]; if (!*ePtrs[i]) changed = true;
                }
                if (wasUnl) GuiLock();
                GuiLabel({ sX + 395, box1Y + 10, 40, 20 }, "dB");
                box1Y += 28;
            }
            if (GuiButton({ sX + 20, box1Y + 15, 140, 22 }, "Default Sources")) { cfg.volfm = 0; cfg.volssg = 0; cfg.voladpcm = 0; cfg.volrhythm = 0; changed = true; }
            curY += group1H + 40;
            float group2H = 230;
            GuiGroupBox({ sX + 5, curY - 5, width - 65, group2H }, "Rhythm Details (BD / SD / TOP / HH / TOM / RIM)");
            float box2Y = curY;
            int* rPtrs[] = { &cfg.volbd, &cfg.volsd, &cfg.voltop, &cfg.volhh, &cfg.voltom, &cfg.volrim };
            const char* rNames[] = { "BD", "SD", "TOP", "HH", "TOM", "RIM" };
            for (int i = 0; i < 6; i++) {
                GuiLabel({ sX + 20, box2Y + 10, 40, 20 }, rNames[i]);
                float valF = (float)*rPtrs[i];
                if (volRhythmDetailEdit[i]) GuiSetState(STATE_DISABLED);
                if (GuiSlider({ sX + 65, box2Y + 10, 265, 16 }, NULL, NULL, &valF, -100, 40)) { *rPtrs[i] = (int)valF; changed = true; }
                wasUnl = false; if (volRhythmDetailEdit[i]) { GuiUnlock(); wasUnl = true; }
                if (GuiValueBox({ sX + 340, box2Y + 10, 50, 16 }, NULL, rPtrs[i], -100, 40, volRhythmDetailEdit[i])) {
                    volRhythmDetailEdit[i] = !volRhythmDetailEdit[i]; if (!volRhythmDetailEdit[i]) changed = true;
                }
                if (wasUnl) GuiLock();
                GuiLabel({ sX + 395, box2Y + 10, 40, 20 }, "dB");
                box2Y += 28;
            }
            if (GuiButton({ sX + 20, box2Y + 15, 140, 22 }, "Default Rhythm")) { cfg.volbd = 0; cfg.volsd = 0; cfg.voltop = 0; cfg.volhh = 0; cfg.voltom = 0; cfg.volrim = 0; changed = true; }
        EndScissorMode();
    }
    else if (activeTab == 4) { // Input
        float labelW = 170;

        Rectangle scrollBounds = { x + 10, y + 70, width - 20, height - 120 };
        Rectangle content = { 0, 0, width - 40, 360 };
        Rectangle view;
        if (!anyEdit && CheckCollisionPointRec(GetMousePosition(), scrollBounds)) inputScroll.y += GetMouseWheelMove() * 20;
        GuiScrollPanel(scrollBounds, NULL, content, &inputScroll, &view);
        if (inputScroll.y > 0) inputScroll.y = 0;
        if (inputScroll.y < (view.height - content.height)) inputScroll.y = view.height - content.height;

        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
            float sX = view.x + inputScroll.x;
            float sY = view.y + inputScroll.y;
            float curY = sY + 20;

            // 1. Keyboard Settings
            GuiGroupBox({ sX + 5, curY - 5, width - 65, rowH * 3 + 15 }, "Keyboard Settings");
            curY += 10;

            GuiLabel({ sX + 15, curY, labelW, 20 }, "Keyboard:");
            Rectangle kRect = { sX + 190, curY, 200, 24 };
            static int kIdx;
            if (!keyboardEdit) kIdx = cfg.keytype;
            if (keyboardEdit) { ddRect = kRect; ddText = "AT-106 JP;AT-101 US (US)"; ddIndexPtr = &kIdx; ddEditPtr = &keyboardEdit; }
            else if (GuiDropdownBox(kRect, "AT-106 JP;AT-101 US (US)", &kIdx, false)) keyboardEdit = true;
            curY += rowH + 4;
            GuiLabel({ sX + 15, curY, labelW, 20 }, "Cursor to Numpad:");
            int numpadVal = (cfg.flags & PC8801::Config::usearrowfor10) ? 1 : 0;
            if (GuiToggleSlider({ sX + 190, curY, 60, 20 }, "OFF;ON", &numpadVal)) {
                if (numpadVal) cfg.flags |= PC8801::Config::usearrowfor10; else cfg.flags &= ~PC8801::Config::usearrowfor10; changed = true;
            }

            curY += rowH;
            GuiLabel({ sX + 15, curY, labelW, 20 }, "Num row to Numpad:");
            int numrowVal = (cfg.flag2 & PC8801::Config::usenumrowfor10) ? 1 : 0;
            if (GuiToggleSlider({ sX + 190, curY, 60, 20 }, "OFF;ON", &numrowVal)) {
                if (numrowVal) cfg.flag2 |= PC8801::Config::usenumrowfor10; else cfg.flag2 &= ~PC8801::Config::usenumrowfor10; changed = true;
            }

            // 2. Joypad Settings
            curY += rowH + 25;
            GuiGroupBox({ sX + 5, curY - 5, width - 65, rowH * 2 + 15 }, "Joypad Settings");
            curY += 10;
            GuiLabel({ sX + 15, curY, labelW, 20 }, "Enable Joypad:");
            int padVal = (cfg.flags & PC8801::Config::enablepad) ? 1 : 0;
            if (GuiToggleSlider({ sX + 190, curY, 60, 20 }, "OFF;ON", &padVal)) {
                if (padVal) {
                    cfg.flags |= PC8801::Config::enablepad;
                    cfg.flags &= ~PC8801::Config::enablemouse; // Mutually exclusive in original M88
                } else cfg.flags &= ~PC8801::Config::enablepad;
                changed = true;
            }

            curY += rowH;
            GuiLabel({ sX + 15, curY, labelW, 20 }, "Swap Buttons:");
            if (!(cfg.flags & PC8801::Config::enablepad)) GuiSetState(STATE_DISABLED);
            int swapVal = (cfg.flags & PC8801::Config::swappadbuttons) ? 1 : 0;
            if (GuiToggleSlider({ sX + 190, curY, 60, 20 }, "OFF;ON", &swapVal)) {
                if (swapVal) cfg.flags |= PC8801::Config::swappadbuttons; else cfg.flags &= ~PC8801::Config::swappadbuttons; changed = true;
            }
            GuiSetState(STATE_NORMAL);

            // 3. Mouse Settings
            curY += rowH + 25;
            GuiGroupBox({ sX + 5, curY - 5, width - 65, rowH * 3 + 15 }, "Mouse Settings");
            curY += 10;
            GuiLabel({ sX + 15, curY, labelW, 20 }, "Enable Mouse:");
            int mouseVal = (cfg.flags & PC8801::Config::enablemouse) ? 1 : 0;
            if (GuiToggleSlider({ sX + 190, curY, 60, 20 }, "OFF;ON", &mouseVal)) {
                if (mouseVal) {
                    cfg.flags |= PC8801::Config::enablemouse;
                    cfg.flags &= ~PC8801::Config::enablepad; // Mutually exclusive
                } else cfg.flags &= ~PC8801::Config::enablemouse;
                changed = true;
            }

            curY += rowH;
            GuiLabel({ sX + 15, curY, labelW, 20 }, "Mouse Joy Mode:");
            if (!(cfg.flags & PC8801::Config::enablemouse)) GuiSetState(STATE_DISABLED);
            int mJoyVal = (cfg.flags & PC8801::Config::mousejoymode) ? 1 : 0;
            if (GuiToggleSlider({ sX + 190, curY, 60, 20 }, "OFF;ON", &mJoyVal)) {
                if (mJoyVal) cfg.flags |= PC8801::Config::mousejoymode; else cfg.flags &= ~PC8801::Config::mousejoymode; changed = true;
            }

            curY += rowH;
            GuiLabel({ sX + 15, curY, labelW, 20 }, "Sensitivity:");
            float mSensF = (float)cfg.mousesensibility;
            if (mouseSensEdit) GuiSetState(STATE_DISABLED);
            if (GuiSlider({ sX + 190, curY, 130, 16 }, NULL, NULL, &mSensF, 1, 10)) { cfg.mousesensibility = (uint)mSensF; changed = true; }
            bool wasUnl = false; if (mouseSensEdit) { GuiUnlock(); wasUnl = true; }
            if (GuiValueBox({ sX + 330, curY, 50, 16 }, NULL, &mouseSensVal, 1, 10, mouseSensEdit)) {
                mouseSensEdit = !mouseSensEdit;
                if (!mouseSensEdit) { cfg.mousesensibility = (uint)mouseSensVal; changed = true; }
            }
            if (wasUnl) GuiLock();
            GuiSetState(STATE_NORMAL);
        EndScissorMode();

        content.height = curY - sY + 15;
    }
    else if (activeTab == 5) { // About
        GuiLabel({ x + 20, pY, 400, 20 }, "M88M - PC-8801 Emulator for Modern Platforms");
        pY += 25;
        GuiLabel({ x + 20, pY, 400, 20 }, "Version: 0.2.0-beta");
        pY += 35;
        GuiLabel({ x + 20, pY, 400, 20 }, "Original M88: Copyright (C) cisc 1998-2003");
        pY += 20;
        GuiLabel({ x + 20, pY, 400, 20 }, "OPNA Emulation: fmgen by cisc");
        pY += 35;
        GuiLabel({ x + 20, pY, 400, 20 }, "Ported with Raylib & Raygui");
        pY += 20;
        GuiLabel({ x + 20, pY, 400, 20 }, "Contributor: Gemini / Claude Code");
    }

    if (changed) { coreRunner->RequestConfigApply(cfg, false); Config::Save(cfg); }

    if (anyEdit) GuiUnlock();

    GuiSetState(STATE_NORMAL);
    if (!anyEdit) {
        if (GuiButton({ x + width - 120, y + height - 40, 100, 28 }, "Back")) {
            ToggleMenu(coreRunner);
            return;
        }
    } else {
        GuiSetState(STATE_DISABLED);
        GuiButton({ x + width - 120, y + height - 40, 100, 28 }, "Back");
        GuiSetState(STATE_NORMAL);
    }

    // 6. Deferred Dropdown Drawing
    if (isDropdownOpen && ddEditPtr) {
        DrawRectangleRec({ x, y + 30, width, height - 30 }, Fade(BLACK, 0.4f));

        int itemCount = 1;
        for (const char* c = ddText; *c; c++) if (*c == ';') itemCount++;
        Rectangle expBounds = ddRect;
        expBounds.height += itemCount * ddRect.height;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !CheckCollisionPointRec(GetMousePosition(), expBounds)) {
            *ddEditPtr = false;
        }

        if (GuiDropdownBox(ddRect, ddText, ddIndexPtr, true)) {
            *ddEditPtr = false;
            if (ddEditPtr == &basicModeEdit) {
                if (*ddIndexPtr == 0) cfg.basicmode = PC8801::Config::N88V1;
                else if (*ddIndexPtr == 1) cfg.basicmode = PC8801::Config::N88V2;
                else if (*ddIndexPtr == 2) cfg.basicmode = PC8801::Config::N80;
                else if (*ddIndexPtr == 3) cfg.basicmode = PC8801::Config::N80V2;
                changed = true; resetPending = true;
            } else if (ddEditPtr == &cpuModeEdit) { cfg.cpumode = *ddIndexPtr; changed = true; }
            else if (ddEditPtr == &port44Edit) {
                if (*ddIndexPtr == 0) { cfg.flag2 |= PC8801::Config::disableopn44; cfg.flags &= ~PC8801::Config::enableopna; }
                else if (*ddIndexPtr == 1) { cfg.flag2 &= ~PC8801::Config::disableopn44; cfg.flags &= ~PC8801::Config::enableopna; }
                else { cfg.flag2 &= ~PC8801::Config::disableopn44; cfg.flags |= PC8801::Config::enableopna; }
                changed = true; resetPending = true;
            } else if (ddEditPtr == &portA8Edit) {
                cfg.flags &= ~(PC8801::Config::opnona8 | PC8801::Config::opnaona8);
                if (*ddIndexPtr == 1) cfg.flags |= PC8801::Config::opnona8;
                else if (*ddIndexPtr == 2) cfg.flags |= PC8801::Config::opnaona8;
                changed = true; resetPending = true;
            } else if (ddEditPtr == &samplingEdit) {
                if (*ddIndexPtr == 0) cfg.sound = 44100; else if (*ddIndexPtr == 1) cfg.sound = 48000; else if (*ddIndexPtr == 2) cfg.sound = 55467; else if (*ddIndexPtr == 3) cfg.sound = 96000;
                changed = true; resetPending = true;
            } else if (ddEditPtr == &windowScaleEdit) {
                int scale = (*ddIndexPtr) + 1;
                SetWindowSize(640 * scale, (400 * scale) + 24);
            } else if (ddEditPtr == &keyboardEdit) { cfg.keytype = *ddIndexPtr; changed = true; resetPending = true; }

            if (changed) { coreRunner->RequestConfigApply(cfg, false); Config::Save(cfg); }
        }
    }
}

void UIManager::DrawStatusBar(DiskManager* diskmgr) {
    float sW = (float)GetScreenWidth();
    float sH = (float)GetScreenHeight();
    GuiStatusBar({ 0, sH - 24, sW, 24 }, "");

    float centerY = sH - 13.0f;
    float textY = sH - 20.0f; // Adjusted for 16px font centering (24 - 16 = 8, so 4px padding)

    int d1 = diskmgr->GetCurrentDisk(1);
    const char* t1_raw = (d1 >= 0) ? diskmgr->GetImageTitle(1, d1) : nullptr;
    std::string t1 = (t1_raw) ? Paths::SJIStoUTF8(std::string(t1_raw, 16)) : "Empty";
    if (t1_raw) {
        size_t last = t1.find_last_not_of(" \0", t1.length());
        if (last != std::string::npos) t1 = t1.substr(0, last + 1);
    }

    Color ledOn = StyleColor(STATUSBAR, BORDER_COLOR_FOCUSED);
    Color ledOff = StyleFade(STATUSBAR, BORDER_COLOR_FOCUSED, 0.25f);
    Color statusText = StyleColor(STATUSBAR, TEXT_COLOR_NORMAL);

    Color l1 = (statusdisplay.GetFDState(1) & 1) ? ledOn : ledOff;
    DrawCircle(15, (int)centerY, 4, l1);

    DrawEnText("FDD2:", 25, (int)textY, statusText);
    if (ContainsJapanese(t1) && IsFontValid(fontJp)) {
        DrawTextEx(fontJp, t1.c_str(), { 75, sH - 21 }, 16, 1, statusText);
    } else {
        DrawEnText(t1.c_str(), 75, (int)textY, statusText);
    }

    int d0 = diskmgr->GetCurrentDisk(0);
    const char* t0_raw = (d0 >= 0) ? diskmgr->GetImageTitle(0, d0) : nullptr;
    std::string t0 = (d0 >= 0) ? Paths::SJIStoUTF8(std::string(t0_raw, 16)) : "Empty";
    if (t0_raw) {
        size_t last = t0.find_last_not_of(" \0", t0.length());
        if (last != std::string::npos) t0 = t0.substr(0, last + 1);
    }

    Color l0 = (statusdisplay.GetFDState(0) & 1) ? ledOn : ledOff;
    DrawCircle(215, (int)centerY, 4, l0);

    DrawEnText("FDD1:", 225, (int)textY, statusText);
    if (ContainsJapanese(t0) && IsFontValid(fontJp)) {
        DrawTextEx(fontJp, t0.c_str(), { 275, sH - 21 }, 16, 1, statusText);
    } else {
        DrawEnText(t0.c_str(), 275, (int)textY, statusText);
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
    DrawEnText(infoStr.c_str(), (int)sW - 200, (int)textY, statusText);

    DrawEnText(TextFormat("%d FPS", GetFPS()), (int)sW - 80, (int)textY, statusText);
}

void UIManager::OpenNativeDialog(DiskManager* diskmgr, int drive) {
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "Disk Image", "d88,d77,88i,dim,dx9,784,dsk" } };

    std::string defaultPathStr;
    const nfdchar_t* defaultPath = NULL;
    if (Config::Get().flags & PC8801::Config::savedirectory) {
        if (!lastOpenedPath[drive].empty()) {
            defaultPathStr = GetDirFromPath(lastOpenedPath[drive]);
            if (!defaultPathStr.empty()) defaultPath = defaultPathStr.c_str();
        }
    }

    if (NFD_OpenDialog(&outPath, filterItem, 1, defaultPath) == NFD_OKAY) {
        lastOpenedPath[drive] = outPath;
        if (diskmgr->Mount(drive, outPath, false, 0, false)) {
            if (diskmgr->GetNumDisks(drive) > 1) {
                selectingDiskForDrive = drive;
            }
        }
        NFD_FreePath(outPath);
    }
}

void UIManager::DrawConfirmDialog(bool& shouldExit, PC88* pc88, CoreRunner* coreRunner) {
    float width = 300;
    float height = 150;
    float x = (float)GetScreenWidth() / 2 - width / 2;
    float y = (float)GetScreenHeight() / 2 - height / 2;

    const char* title = (modalState == MODAL_CONFIRM_RESET) ? "Confirm Reset" : "Confirm Quit";
    const char* msg = (modalState == MODAL_CONFIRM_RESET) ? "Are you sure you want to reset?" : "Are you sure you want to quit?";

    if (GuiWindowBox({ x, y, width, height }, title)) {
        modalState = MODAL_NONE;
    }

    GuiLabel({ x + 20, y + 40, width - 40, 20 }, msg);

    if (GuiButton({ x + 40, y + 90, 100, 30 }, "Yes")) {
        if (modalState == MODAL_CONFIRM_RESET) {
            if (coreRunner) coreRunner->RequestReset();
            else pc88->Reset();
            ToggleMenu(coreRunner);
        } else if (modalState == MODAL_CONFIRM_QUIT) {
            shouldExit = true;
        }
        modalState = MODAL_NONE;
    }

    if (GuiButton({ x + 160, y + 90, 100, 30 }, "No")) {
        modalState = MODAL_NONE;
    }
}
