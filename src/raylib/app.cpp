#include "raylib.h"
#include "raygui.h"
#include "screen_view.h"
#include "core_runner.h"
#include "paths.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

// MSVC UTF-8 Support
#ifdef _MSC_VER
    #pragma execution_character_set("utf-8")
#endif

#ifdef _WIN32
#include <io.h>
#define access _access
#define F_OK 0
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx
#endif

int main() {
    const int screenWidth = 640;
    const int screenHeight = 424; // 400 (emulation) + 24 (status bar)

    InitWindow(screenWidth, screenHeight, "M88M - PC-8801 Emulator");
    SetExitKey(0);

    // Standard codepoints for Japanese
    std::vector<int> cp;
    for (int i = 32; i < 127; i++) cp.push_back(i);
    for (int i = 0x3000; i <= 0x30FF; i++) cp.push_back(i);
    for (int i = 0xFF61; i <= 0xFF9F; i++) cp.push_back(i);
    for (int i = 0x4E00; i <= 0x6000; i++) cp.push_back(i);

    const char* fontCandidates[] = {
        "assets/NotoSansJP-Regular.ttf",
        "../assets/NotoSansJP-Regular.ttf",
        "../../assets/NotoSansJP-Regular.ttf"
    };
    
    Font fontJp = { 0 };
    for (const char* path : fontCandidates) {
        if (access(path, F_OK) == 0) {
            fontJp = LoadFontEx(path, 32, cp.data(), (int)cp.size());
            if (IsFontValid(fontJp) && fontJp.glyphCount > 300) {
                break;
            }
            if (IsFontValid(fontJp)) UnloadFont(fontJp);
            fontJp = { 0 };
        }
    }
    
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
