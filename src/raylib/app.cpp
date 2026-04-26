#include "raylib.h"
#include "screen_view.h"
#include "core_runner.h"
#include "paths.h"
#include <iostream>

int main() {
    // Initialization
    // --------------------------------------------------------------------------------------
    const int screenWidth = 640;
    const int screenHeight = 400;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "M88M - PC-8801 Emulator");

    RaylibDraw draw;
    if (!draw.Init(640, 400, 8)) {
        std::cerr << "Failed to initialize draw" << std::endl;
        return 1;
    }

    CoreRunner core;
    if (!core.Init(&draw)) {
        std::cerr << "Failed to initialize core (probably missing ROMs)" << std::endl;
        // We continue anyway to show the window, but core won't run.
    } else {
        core.Start();
    }

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    // --------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        // ----------------------------------------------------------------------------------
        core.UpdateInput();

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count > 0) {
                // Mount first file to Drive 1
                core.GetDiskManager()->Mount(0, droppedFiles.paths[0], false, 0, false);
            }
            UnloadDroppedFiles(droppedFiles);
        }

        if (IsKeyPressed(KEY_F1)) {
            core.OpenDiskDialog(0);
        }
        if (IsKeyPressed(KEY_F2)) {
            core.OpenDiskDialog(1);
        }
        // ----------------------------------------------------------------------------------

        // Draw
        // ----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(BLACK);

            draw.Render();
            
            core.DrawUI();

            DrawFPS(10, 10);

        EndDrawing();
        // ----------------------------------------------------------------------------------
    }

    // De-Initialization
    // --------------------------------------------------------------------------------------
    core.Stop();
    draw.Cleanup();
    CloseWindow();        // Close window and OpenGL context
    // --------------------------------------------------------------------------------------

    return 0;
}
