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
    const int screenWidth = 800;
    const int screenHeight = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "M88M - PC-8801 Emulator");

    RaylibDraw draw;
    if (!draw.Init(640, 400, 8)) {
        std::cerr << "Failed to initialize draw" << std::endl;
        return 1;
    }

    CoreRunner core;
    core.Init(&draw);
    core.Start();

    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        core.UpdateInput();

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count > 0) {
                core.GetDiskManager()->Mount(0, droppedFiles.paths[0], false, 0, false);
            }
            UnloadDroppedFiles(droppedFiles);
        }

        if (IsKeyPressed(KEY_F1)) core.OpenDiskDialog(0);
        if (IsKeyPressed(KEY_F2)) core.OpenDiskDialog(1);
        if (IsKeyPressed(KEY_F10)) core.ToggleSettings();

        // Draw
        BeginDrawing();
            ClearBackground(BLACK);

            draw.Render();
            core.DrawUI();

            // ROM Error Dialog (Robust Implementation)
            if (core.HasRomError()) {
                float boxWidth = 740;
                float boxHeight = 360;
                float x = (float)GetScreenWidth()/2 - boxWidth/2;
                float y = (float)GetScreenHeight()/2 - boxHeight/2;

                // Dim the background
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

                if (GuiWindowBox({ x, y, boxWidth, boxHeight }, "BIOS ROM Error")) {
                    core.ClearRomError();
                }
                
                // Draw error message line by line
                std::stringstream ss(core.GetRomError());
                std::string line;
                int lineY = (int)y + 60;
                Color textColor = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));

                while (std::getline(ss, line, '\n')) {
                    DrawText(line.c_str(), (int)x + 25, lineY, 20, textColor);
                    lineY += 25;
                }
                
                if (GuiButton({ x + boxWidth/2 - 50, y + boxHeight - 50, 100, 30 }, "Close")) {
                    core.ClearRomError();
                }
            }

            DrawFPS(GetScreenWidth() - 80, 10);
        EndDrawing();
    }

    // De-Initialization
    core.Stop();
    draw.Cleanup();
    CloseWindow();

    return 0;
}
