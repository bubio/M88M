#pragma once

#include "diskmgr.h"
#include "pc88/config.h"

class DiskDialog {
public:
    DiskDialog();
    ~DiskDialog();

    void Draw(DiskManager* diskmgr);
    void DrawSettings(PC8801::Config& cfg, bool* open);
    void OpenNativeDialog(DiskManager* diskmgr, int drive);
};
