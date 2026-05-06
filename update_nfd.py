import re

with open('src/raylib/disk_dialog.cpp', 'r') as f:
    content = f.read()

open_both = """void UIManager::OpenBothDrives(DiskManager* diskmgr) {
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "Disk Image", "d88,d77,88i,dim,dx9,784,dsk" } };
    
    std::string defaultPathStr;
    const nfdchar_t* defaultPath = NULL;
    if (Config::Get().flags & PC8801::Config::savedirectory) {
        if (!lastOpenedPath[0].empty()) {
            defaultPathStr = GetDirFromPath(lastOpenedPath[0]);
            if (!defaultPathStr.empty()) defaultPath = defaultPathStr.c_str();
        } else if (!lastOpenedPath[1].empty()) {
            defaultPathStr = GetDirFromPath(lastOpenedPath[1]);
            if (!defaultPathStr.empty()) defaultPath = defaultPathStr.c_str();
        }
    }

    if (NFD_OpenDialog(&outPath, filterItem, 1, defaultPath) == NFD_OKAY) {
        lastOpenedPath[0] = outPath;
        lastOpenedPath[1] = outPath;
        if (diskmgr->Mount(0, outPath, false, 0, false)) {
            if (diskmgr->GetNumDisks(0) > 1) {
                diskmgr->Mount(1, outPath, false, 1, false);
            }
        }
        NFD_FreePath(outPath);
    }
}"""

open_native = """void UIManager::OpenNativeDialog(DiskManager* diskmgr, int drive) {
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = { { "Disk Image", "d88,d77,88i,dim,dx9,784,dsk" } };
    
    std::string defaultPathStr;
    const nfdchar_t* defaultPath = NULL;
    if (Config::Get().flags & PC8801::Config::savedirectory) {
        if (!lastOpenedPath[drive].empty()) {
            defaultPathStr = GetDirFromPath(lastOpenedPath[drive]);
            if (!defaultPathStr.empty()) defaultPath = defaultPathStr.c_str();
        }
    }

    if (NFD_OpenDialog(&outPath, filterItem, 1, defaultPath) == NFD_OKAY) {
        lastOpenedPath[drive] = outPath;
        if (diskmgr->Mount(drive, outPath, false, 0, false)) {
            if (diskmgr->GetNumDisks(drive) > 1) {
                selectingDiskForDrive = drive;
            }
        }
        NFD_FreePath(outPath);
    }
}"""

content = re.sub(r'void UIManager::OpenBothDrives\(DiskManager\* diskmgr\) \{.*?\n\}', open_both, content, flags=re.DOTALL)
content = re.sub(r'void UIManager::OpenNativeDialog\(DiskManager\* diskmgr, int drive\) \{.*?\n\}', open_native, content, flags=re.DOTALL)

with open('src/raylib/disk_dialog.cpp', 'w') as f:
    f.write(content)
