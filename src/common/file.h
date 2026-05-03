// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Portable FileIO (replaces win32/file.h on cross-platform builds).
//
//	The Win32 build still resolves "file.h" to src/win32/file.h via the PCH
//	include path. On macOS/Linux/MinGW the include path order picks up this
//	stdio-backed implementation instead. The public surface matches the
//	original FileIO contract used by core code (opna, diskmgr, kanjirom,
//	memory, subsys, tapemgr, crtc).
// ----------------------------------------------------------------------------
#pragma once

#include "types.h"
#include <stdio.h>
#include <string>

class FileIO
{
public:
    enum Flags
    {
        open     = 0x000001,
        readonly = 0x000002,
        create   = 0x000004,
    };

    enum SeekMethod
    {
        begin = 0, current = 1, end = 2,
    };

    enum Error
    {
        success = 0,
        file_not_found,
        sharing_violation,
        unknown = -1,
    };

    FileIO();
    FileIO(const char* filename, uint flg = 0);
    virtual ~FileIO();

    bool  Open(const char* filename, uint flg = 0);
    bool  CreateNew(const char* filename);
    bool  Reopen(uint flg = 0);
    void  Close();
    Error GetError() { return error_; }

    int32 Read(void* dest, int32 len);
    int32 Write(const void* src, int32 len);
    bool  Seek(int32 fpos, SeekMethod method);
    int32 Tellp();
    bool  SetEndOfFile();

    uint  GetFlags() { return flags_; }
    void  SetLogicalOrigin(int32 origin) { lorigin_ = (uint32)origin; }

    static std::string ResolvePathCaseInsensitive(const std::string& path);

private:
    FileIO(const FileIO&);
    const FileIO& operator=(const FileIO&);

    FILE*  fp_;
    uint   flags_;
    uint32 lorigin_;
    Error  error_;
    char   path_[MAX_PATH];
};
