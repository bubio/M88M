#pragma once

#include "diskmgr.h"

class DiskDialog {
public:
    DiskDialog();
    ~DiskDialog();

    void Draw(DiskManager* diskmgr);
    void OpenNativeDialog(DiskManager* diskmgr, int drive);
};
