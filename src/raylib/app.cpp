#include "raylib.h"
#include "raygui.h"
#include "screen_view.h"
#include "core_runner.h"
#include "paths.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#ifdef _WIN32
#include <io.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#endif

int main() {
    // Initialization
    const int screenWidth = 640;
    const int screenHeight = 424; // 400 (emulation) + 24 (status bar)

    InitWindow(screenWidth, screenHeight, "M88M - PC-8801 Emulator");
    SetExitKey(0); // Disable ESC exit

    // Load Japanese Font with explicit numeric codepoint ranges
    std::vector<int> cp;
    for (int i = 32; i < 127; i++) cp.push_back(i);             // ASCII
    for (int i = 0x3000; i <= 0x30FF; i++) cp.push_back(i);    // Symbols, Hiragana, Katakana
    for (int i = 0xFF61; i <= 0xFF9F; i++) cp.push_back(i);    // Half-width Katakana
    for (int i = 0x4E00; i <= 0x9FFF; i++) cp.push_back(i);    // CJK Unified Ideographs (expanded)

    // Font paths for different platforms
    const char* fontPaths[] = {
#ifdef __APPLE__
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc",
        "/System/Library/Fonts/Cache/Hiragino Sans GB.ttc"
#elif defined(_WIN32)
        "C:\\Windows\\Fonts\\msgothic.ttc",
        "C:\\Windows\\Fonts\\msmincho.ttc",
        "C:\\Windows\\Fonts\\meiryo.ttc",
        "C:\\Windows\\Fonts\\meiryob.ttc"
#else
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
#endif
    };
    
    Font fontJp = { 0 };
    for (const char* path : fontPaths) {
        if (access(path, F_OK) == 0) {
            fontJp = LoadFontEx(path, 32, cp.data(), (int)cp.size());
            if (IsFontValid(fontJp)) break;
        }
    }
    RaylibDraw draw;
    if (!draw.Init(640, 400, 8)) {
        std::cerr << "Failed to initialize draw" << std::endl;
        return 1;
    }

    CoreRunner core;
    core.Init(&draw);
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
        // Update
        core.UpdateUI(shouldExit);
        core.UpdateInput();

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count > 0) {
                core.GetDiskManager()->Mount(0, droppedFiles.paths[0], false, 0, false);
            }
            UnloadDroppedFiles(droppedFiles);
        }

        // Draw
        BeginDrawing();
            ClearBackground(BLACK);

            draw.Render();
            
            // Unified Overlay UI
            core.DrawUI(shouldExit);

            // ROM Error Dialog
            if (core.HasRomError()) {
                float boxWidth = 740; 
                float boxHeight = 360;
                float x = (float)GetScreenWidth()/2 - boxWidth/2;
                float y = (float)GetScreenHeight()/2 - boxHeight/2;

                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f));
                if (GuiWindowBox({ x, y, boxWidth, boxHeight }, "BIOS ROM Error")) shouldExit = true;
                
                std::stringstream ss(core.GetRomError());
                std::string line;
                int lineY = (int)y + 60;
                Color textColor = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
                while (std::getline(ss, line, '\n')) {
                    DrawText(line.c_str(), (int)x + 25, lineY, 20, textColor);
                    lineY += 25;
                }
                if (GuiButton({ x + boxWidth/2 - 50, y + boxHeight - 50, 100, 30 }, "Exit")) shouldExit = true;
            }
        EndDrawing();
    }

    core.Stop();
    draw.Cleanup();
    CloseWindow();
    return 0;
}
