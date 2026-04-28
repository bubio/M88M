#include "raylib.h"
#include "raygui.h"
#include "screen_view.h"
#include "core_runner.h"
#include "paths.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

int main() {
    // Initialization
    const int screenWidth = 640;
    const int screenHeight = 424; // 400 (emulation) + 24 (status bar)

    InitWindow(screenWidth, screenHeight, "M88M - PC-8801 Emulator");
    SetExitKey(0); // Disable ESC exit

    RaylibDraw draw;
    if (!draw.Init(640, 400, 8)) {
        std::cerr << "Failed to initialize draw" << std::endl;
        return 1;
    }

    CoreRunner core;
    core.Init(&draw);
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
