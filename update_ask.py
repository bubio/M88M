import re

# Update disk_dialog.h
with open('src/raylib/disk_dialog.h', 'r') as f:
    h_content = f.read()

if 'void RequestQuitConfirm()' not in h_content:
    h_content = h_content.replace('void ToggleMenu(class CoreRunner* coreRunner = nullptr);',
                                  'void ToggleMenu(class CoreRunner* coreRunner = nullptr);\n    void RequestQuitConfirm() { modalState = MODAL_CONFIRM_QUIT; showMenu = true; }')
    with open('src/raylib/disk_dialog.h', 'w') as f:
        f.write(h_content)


# Update app.cpp
with open('src/raylib/app.cpp', 'r') as f:
    app_content = f.read()

if 'if (WindowShouldClose()) {' not in app_content:
    app_content = app_content.replace('while (!WindowShouldClose() && !shouldExit)', 'while (!shouldExit)')
    
    inject = """        if (WindowShouldClose()) {
            if (PC8801::Config::Get().flags & PC8801::Config::askbeforereset) {
                core.GetUIManager()->RequestQuitConfirm();
            } else {
                shouldExit = true;
            }
        }
        core.UpdateUI(shouldExit);"""
    app_content = app_content.replace('core.UpdateUI(shouldExit);', inject)
    with open('src/raylib/app.cpp', 'w') as f:
        f.write(app_content)

# Update disk_dialog.cpp
with open('src/raylib/disk_dialog.cpp', 'r') as f:
    cpp_content = f.read()

# Add DrawConfirmDialog at the end of the file
if 'void UIManager::DrawConfirmDialog' not in cpp_content:
    cpp_content += """
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
"""

with open('src/raylib/disk_dialog.cpp', 'w') as f:
    f.write(cpp_content)

