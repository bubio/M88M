#include "disk_dialog.h"
#include "raylib.h"
#include "nfd.h"
#include <string>
#include <iostream>

DiskDialog::DiskDialog() {
    NFD_Init();
}

DiskDialog::~DiskDialog() {
    NFD_Quit();
}

void DiskDialog::Draw(DiskManager* diskmgr) {
    int x = 10;
    int y = GetScreenHeight() - 40;
    
    for (uint dr = 0; dr < 2; dr++) {
        int index = diskmgr->GetCurrentDisk(dr);
        std::string text = "Drive " + std::to_string(dr + 1) + " (F" + std::to_string(dr + 1) + "): ";
        if (index >= 0) {
            const char* title = diskmgr->GetImageTitle(dr, index);
            text += (title && title[0]) ? title : "(Untitled)";
        } else {
            text += "Empty";
        }
        
        DrawText(text.c_str(), x, y, 10, RAYWHITE);
        y += 15;
    }
}

void DiskDialog::OpenNativeDialog(DiskManager* diskmgr, int drive) {
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "Disk Image", "d88,d77,88i,dim,dx9,784,dsk" } };
    
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    
    if (result == NFD_OKAY) {
        std::cout << "Success! Selected path: " << outPath << std::endl;
        diskmgr->Mount(drive, outPath, false, 0, false);
        NFD_FreePath(outPath);
    } else if (result == NFD_CANCEL) {
        std::cout << "User pressed cancel." << std::endl;
    } else {
        std::cerr << "Error: " << NFD_GetError() << std::endl;
    }
}
