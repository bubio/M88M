#include "disk_dialog.h"
#include "raylib.h"
#include "raygui.h"
#include "nfd.h"
#include "config.h"
#include "pc88.h"
#include <string>
#include <iostream>

DiskDialog::DiskDialog() {
    NFD_Init();
}

DiskDialog::~DiskDialog() {
    NFD_Quit();
}

void DiskDialog::Draw(DiskManager* diskmgr) {
    // Status Bar
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    GuiStatusBar({ 0, screenH - 20, screenW, 20 }, "M88M Ready");

    // Drive Buttons
    for (int i = 0; i < 2; i++) {
        int index = diskmgr->GetCurrentDisk(i);
        std::string label = "Drive " + std::to_string(i + 1) + ": ";
        if (index >= 0) {
            const char* title = diskmgr->GetImageTitle(i, index);
            label += (title && title[0]) ? title : "Mounted";
        } else {
            label += "Empty";
        }

        if (GuiButton({ 10.0f + i * 160.0f, 10.0f, 150.0f, 24.0f }, label.c_str())) {
            OpenNativeDialog(diskmgr, i);
        }
    }
}

void DiskDialog::DrawSettings(PC8801::Config& cfg, bool* open) {
    if (!*open) return;

    if (GuiWindowBox({ 100, 50, 440, 320 }, "M88M Configuration")) {
        *open = false;
    }

    float y = 90;
    GuiLabel({ 120, y, 100, 20 }, "CPU Clock (MHz)");
    float clock = (float)cfg.clock;
    if (GuiSliderBar({ 220, y, 200, 20 }, "1", "8", &clock, 1, 8)) {
        cfg.clock = (int)clock;
    }
    
    y += 40;
    GuiLabel({ 120, y, 100, 20 }, "Basic Mode");
    bool isV2 = (cfg.basicmode == PC8801::Config::N88V2);
    if (GuiCheckBox({ 220, y, 20, 20 }, "N88-V2 Mode", &isV2)) {
        cfg.basicmode = isV2 ? PC8801::Config::N88V2 : PC8801::Config::N88V1;
    }

    y += 40;
    GuiLabel({ 120, y, 100, 20 }, "Sound Volume");
    float vol = (float)cfg.volfm;
    if (GuiSliderBar({ 220, y, 200, 20 }, "0", "128", &vol, 0, 128)) {
        cfg.volfm = cfg.volssg = cfg.voladpcm = cfg.volrhythm = (int)vol;
    }

    y += 80;
    if (GuiButton({ 270, y, 100, 30 }, "Close")) {
        *open = false;
    }
}

void DiskDialog::OpenNativeDialog(DiskManager* diskmgr, int drive) {
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "Disk Image", "d88,d77,88i,dim,dx9,784,dsk" } };
    
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    
    if (result == NFD_OKAY) {
        diskmgr->Mount(drive, outPath, false, 0, false);
        NFD_FreePath(outPath);
    }
}
