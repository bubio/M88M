#include "raylib.h"
#include "raygui.h"
#include "screen_view.h"
#include "core_runner.h"
#include "config.h"
#include "paths.h"
#include "raylib_mouse.cpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#ifndef _WIN32
#include <unistd.h>
#endif

// MSVC UTF-8 Support
#ifdef _MSC_VER
    #pragma execution_character_set("utf-8")
#endif

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define access _access
#define F_OK 0
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx

// Resource ID from m88m.rc
#ifndef IDR_FONT_NOTOSANS
#define IDR_FONT_NOTOSANS 101
#endif
#ifndef IDR_FONT_LATIN
#define IDR_FONT_LATIN 102
#endif
#endif

#ifdef M88_EMBED_FONT
#include "embedded_font.h"
#endif

static Font LoadJapaneseFont() {
    // Codepoints to load
    std::vector<int> cp;
    for (int i = 32; i < 127; i++) cp.push_back(i);
    for (int i = 0x2000; i <= 0x206F; i++) cp.push_back(i); // General Punctuation
    for (int i = 0x3000; i <= 0x30FF; i++) cp.push_back(i); // CJK Symbols, Hiragana, Katakana
    for (int i = 0x4E00; i <= 0x9FFF; i++) cp.push_back(i); // CJK Unified Ideographs
    for (int i = 0xFF00; i <= 0xFFEF; i++) cp.push_back(i); // Halfwidth and Fullwidth Forms

    Font font = { 0 };

#ifdef M88_EMBED_FONT
    font = LoadFontFromMemory(".ttf", embedded_font_jp_data, (int)embedded_font_jp_size, 24, cp.data(), (int)cp.size());
    if (IsFontValid(font) && font.glyphCount > 300) return font;
#endif

#ifdef _WIN32
    // Try loading from Windows Resource first (embedded in exe)
    // RT_RCDATA is (LPCSTR)10, MAKEINTRESOURCEA(id) is (LPCSTR)id
    HRSRC hRes = FindResourceA(NULL, (LPCSTR)IDR_FONT_NOTOSANS, (LPCSTR)10);
    if (hRes) {
        HGLOBAL hData = LoadResource(NULL, hRes);
        if (hData) {
            void* pData = LockResource(hData);
            unsigned int size = SizeofResource(NULL, hRes);
            if (pData && size > 0) {
                font = LoadFontFromMemory(".ttf", (const unsigned char*)pData, (int)size, 24, cp.data(), (int)cp.size());
                if (IsFontValid(font) && font.glyphCount > 300) {
                    return font;
                }
            }
        }
    }
#endif

    // Fallback: Try loading from external file or bundle
    std::vector<std::string> fontCandidates;
    
#ifdef __APPLE__
    // macOS: Look in the app bundle's Resources/fonts directory
    const char* base = GetApplicationDirectory();
    if (base) {
        fontCandidates.push_back(std::string(base) + "fonts/NotoSansJP-Regular.ttf");
    }
#endif

    fontCandidates.push_back("assets/NotoSansJP-Regular.ttf");
    fontCandidates.push_back("../assets/NotoSansJP-Regular.ttf");
    fontCandidates.push_back("../../assets/NotoSansJP-Regular.ttf");
    
    // Platform-specific system fonts for extra robustness
#ifdef __APPLE__
    fontCandidates.push_back("/System/Library/Fonts/Supplemental/Arial Unicode.ttf");
    fontCandidates.push_back("/Library/Fonts/Arial Unicode.ttf");
    fontCandidates.push_back("/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc");
#endif
    
    for (const auto& path : fontCandidates) {
        if (access(path.c_str(), F_OK) == 0) {
            font = LoadFontEx(path.c_str(), 24, cp.data(), (int)cp.size());
            if (IsFontValid(font) && font.glyphCount > 300) return font;
            if (IsFontValid(font)) UnloadFont(font);
        }
    }
    
    Font emptyFont = { 0 };
    return emptyFont;
}

static Font LoadLatinFont() {
    std::vector<int> cp;
    for (int i = 32; i < 127; i++) cp.push_back(i);

    Font font = { 0 };

#ifdef M88_EMBED_FONT
    font = LoadFontFromMemory(".ttf", embedded_font_latin_data, (int)embedded_font_latin_size, 16, cp.data(), (int)cp.size());
    if (IsFontValid(font) && font.glyphCount > 50) return font;
#endif

#ifdef _WIN32
    // Try loading from Windows Resource first (embedded in exe)
    // RT_RCDATA is (LPCSTR)10, MAKEINTRESOURCEA(id) is (LPCSTR)id
    HRSRC hRes = FindResourceA(NULL, (LPCSTR)IDR_FONT_LATIN, (LPCSTR)10);
    if (hRes) {
        HGLOBAL hData = LoadResource(NULL, hRes);
        if (hData) {
            void* pData = LockResource(hData);
            unsigned int size = SizeofResource(NULL, hRes);
            if (pData && size > 0) {
                font = LoadFontFromMemory(".ttf", (const unsigned char*)pData, (int)size, 16, cp.data(), (int)cp.size());
                if (IsFontValid(font) && font.glyphCount > 50) {
                    return font;
                }
            }
        }
    }
#endif

    std::vector<std::string> candidates;
#ifdef __APPLE__
    const char* base = GetApplicationDirectory();
    if (base) {
        candidates.push_back(std::string(base) + "fonts/ChicagoKare-Regular.ttf");
        candidates.push_back(std::string(base) + "../Resources/fonts/ChicagoKare-Regular.ttf");
        candidates.push_back(std::string(base) + "Resources/fonts/ChicagoKare-Regular.ttf");
    }
#endif
    candidates.push_back("assets/ChicagoKare-Regular.ttf");
    candidates.push_back("../assets/ChicagoKare-Regular.ttf");
    candidates.push_back("../../assets/ChicagoKare-Regular.ttf");

    for (const auto& path : candidates) {
        if (access(path.c_str(), F_OK) == 0) {
            font = LoadFontEx(path.c_str(), 16, cp.data(), (int)cp.size());
            if (IsFontValid(font) && font.glyphCount > 50) return font;
            if (IsFontValid(font)) UnloadFont(font);
        }
    }

    Font emptyFont = { 0 };
    return emptyFont;
}

int main() {
#ifdef _WIN32
    // If not running from a console, redirect stderr to a log file
    if (!GetConsoleWindow()) {
        std::string logDir = Paths::GetConfigDir();
        Paths::EnsureDirectory(logDir);
        std::string logPath = logDir + "/m88m.log";
        freopen(logPath.c_str(), "w", stderr);
    }
#endif
    const int screenWidth = 640;
    const int screenHeight = 424; // 400 (emulation) + 24 (status bar)

    InitWindow(screenWidth, screenHeight, "M88M - PC-8801 Emulator");
    SetExitKey(0); // Disable ESC exit

    Font fontJp = LoadJapaneseFont();
    Font fontEn = LoadLatinFont();

    RaylibDraw draw;
    if (!draw.Init(640, 400, 8)) return 1;

    CoreRunner core;

    if (IsFontValid(fontJp)) {
        core.GetUIManager()->SetJPFont(fontJp);
        SetTextureFilter(fontJp.texture, TEXTURE_FILTER_BILINEAR);
    }
    if (IsFontValid(fontEn)) {
        core.GetUIManager()->SetENFont(fontEn);
        SetTextureFilter(fontEn.texture, TEXTURE_FILTER_POINT);
    }

    if (!core.Init(&draw)) {
        // Continue even if init failed to show ROM error dialog
    }
    core.Start();

    SetTargetFPS(60);
    bool shouldExit = false;

    // Main game loop
    while (!shouldExit)
    {
                if (WindowShouldClose()) {
            if (Config::Get().flags & PC8801::Config::askbeforereset) {
                core.GetUIManager()->RequestQuitConfirm();
            } else {
                shouldExit = true;
            }
        }
        core.UpdateUI(shouldExit);
        core.UpdateInput();

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count > 0) {
                core.GetUIManager()->MountDisk(core.GetDiskManager(), droppedFiles.paths[0], 0, 1);
            }
            UnloadDroppedFiles(droppedFiles);
        }

        BeginDrawing();
            ClearBackground(BLACK);
            draw.Render();
            core.DrawUI(shouldExit);

            if (core.HasRomError()) {
                float boxWidth = 500; float boxHeight = 360;
                float x = (float)GetScreenWidth()/2 - boxWidth/2;
                float y = (float)GetScreenHeight()/2 - boxHeight/2;
                {
                    Color bg = GetColor((unsigned)GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
                    bg.a = (unsigned char)(0.8f * 255);
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), bg);
                }
                if (GuiWindowBox({ x, y, boxWidth, boxHeight }, "BIOS ROM Error")) shouldExit = true;
                std::stringstream ss(core.GetRomError());
                std::string line;
                int lineY = (int)y + 60;
                Color textColor = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
                while (std::getline(ss, line, '\n')) {
                    if (IsFontValid(fontEn)) {
                        DrawTextEx(fontEn, line.c_str(), { x + 25, (float)lineY }, 14, 1, textColor);
                    } else {
                        DrawText(line.c_str(), (int)x + 25, lineY, 10, textColor);
                    }
                    lineY += 25;
                }
                if (GuiButton({ x + boxWidth/2 - 50, y + boxHeight - 50, 100, 30 }, "Exit")) shouldExit = true;
            }
        EndDrawing();
    }

    core.Stop();
    draw.Cleanup();
    if (IsFontValid(fontJp)) UnloadFont(fontJp);
    if (IsFontValid(fontEn)) UnloadFont(fontEn);
    CloseWindow();
    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    return main();
}
#endif
