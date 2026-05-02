#include "raylib.h"
#include "raygui.h"
#include "screen_view.h"
#include "core_runner.h"
#include "paths.h"
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
#endif

static Font LoadJapaneseFont() {
    // Codepoints to load
    std::vector<int> cp;
    for (int i = 32; i < 127; i++) cp.push_back(i);
    for (int i = 0x3000; i <= 0x30FF; i++) cp.push_back(i);
    for (int i = 0xFF61; i <= 0xFF9F; i++) cp.push_back(i);
    for (int i = 0x4E00; i <= 0x6000; i++) cp.push_back(i);

    Font font = { 0 };

#ifdef _WIN32
    // Try loading from Windows Resource first (embedded in exe)
    // Use RT_RCDATA or a custom string if "FONT" fails, but .rc uses "FONT"
    HRSRC hRes = FindResourceA(NULL, (LPCSTR)IDR_FONT_NOTOSANS, "FONT");
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

int main() {
    const int screenWidth = 640;
    const int screenHeight = 424; // 400 (emulation) + 24 (status bar)

    InitWindow(screenWidth, screenHeight, "M88M - PC-8801 Emulator");
    SetExitKey(0); // Disable ESC exit

    Font fontJp = LoadJapaneseFont();
    
    RaylibDraw draw;
    if (!draw.Init(640, 400, 8)) return 1;

    CoreRunner core;
    if (!core.Init(&draw)) {
        // Continue even if init failed to show ROM error dialog
    }
    
    if (IsFontValid(fontJp)) {
        core.GetUIManager()->SetJPFont(fontJp);
        SetTextureFilter(fontJp.texture, TEXTURE_FILTER_BILINEAR);
    }
    core.Start();

    SetTargetFPS(60);
    bool shouldExit = false;

    // Main game loop
    while (!WindowShouldClose() && !shouldExit)
    {
        core.UpdateUI(shouldExit);
        core.UpdateInput();

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count > 0) {
                core.GetDiskManager()->Mount(0, droppedFiles.paths[0], false, 0, false);
            }
            UnloadDroppedFiles(droppedFiles);
        }

        BeginDrawing();
            ClearBackground(BLACK);
            draw.Render();
            core.DrawUI(shouldExit);

            if (core.HasRomError()) {
                float boxWidth = 740; float boxHeight = 360;
                float x = (float)GetScreenWidth()/2 - boxWidth/2;
                float y = (float)GetScreenHeight()/2 - boxHeight/2;
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f));
                if (GuiWindowBox({ x, y, boxWidth, boxHeight }, "BIOS ROM Error")) shouldExit = true;
                std::stringstream ss(core.GetRomError());
                std::string line;
                int lineY = (int)y + 60;
                while (std::getline(ss, line, '\n')) {
                    DrawText(line.c_str(), (int)x + 25, lineY, 20, WHITE);
                    lineY += 25;
                }
                if (GuiButton({ x + boxWidth/2 - 50, y + boxHeight - 50, 100, 30 }, "Exit")) shouldExit = true;
            }
        EndDrawing();
    }

    core.Stop();
    draw.Cleanup();
    if (IsFontValid(fontJp)) UnloadFont(fontJp);
    CloseWindow();
    return 0;
}
