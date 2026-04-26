#pragma once

#include "diskmgr.h"

class DiskDialog {
public:
    DiskDialog();
    ~DiskDialog();

    void Draw(DiskManager* diskmgr);
};
