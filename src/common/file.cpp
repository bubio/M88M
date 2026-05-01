// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	stdio-backed FileIO implementation for the cross-platform build.
// ----------------------------------------------------------------------------
#include "file.h"
#include <string.h>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>

static std::wstring UTF8ToWide(const char* utf8) {
    if (!utf8 || !*utf8) return L"";
    int nw = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (nw <= 0) return L"";
    std::vector<wchar_t> wbuf(nw);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf.data(), nw);
    return std::wstring(wbuf.data());
}

static std::wstring ModeToWide(const char* mode) {
    if (!mode) return L"";
    std::wstring wmode;
    while (*mode) wmode += (wchar_t)*mode++;
    return wmode;
}
#endif

FileIO::FileIO()
    : fp_(nullptr), flags_(0), lorigin_(0), error_(success)
{
    path_[0] = 0;
}

FileIO::FileIO(const char* filename, uint flg)
    : fp_(nullptr), flags_(0), lorigin_(0), error_(success)
{
    path_[0] = 0;
    Open(filename, flg);
}

FileIO::~FileIO()
{
    Close();
}

bool FileIO::Open(const char* filename, uint flg)
{
    Close();

    strncpy_s(path_, MAX_PATH, filename, _TRUNCATE);

    const char* mode = (flg & readonly) ? "rb" : "r+b";
#ifdef _WIN32
    std::wstring wpath = UTF8ToWide(filename);
    fp_ = _wfopen(wpath.c_str(), ModeToWide(mode).c_str());
    if (!fp_ && !(flg & readonly)) {
        fp_ = _wfopen(wpath.c_str(), L"rb");
        if (fp_) flg |= readonly;
    }
#else
    fp_ = fopen(filename, mode);
    if (!fp_ && !(flg & readonly)) {
        // Existing file but no write permission — fall back to read-only.
        fp_ = fopen(filename, "rb");
        if (fp_) flg |= readonly;
    }
#endif

    if (!fp_) {
        error_ = file_not_found;
        flags_ = 0;
        return false;
    }

    flags_   = open | (flg & readonly);
    lorigin_ = 0;
    error_   = success;
    return true;
}

bool FileIO::CreateNew(const char* filename)
{
    Close();

    strncpy_s(path_, MAX_PATH, filename, _TRUNCATE);

#ifdef _WIN32
    fp_ = _wfopen(UTF8ToWide(filename).c_str(), L"w+b");
#else
    fp_ = fopen(filename, "w+b");
#endif
    if (!fp_) {
        error_ = unknown;
        flags_ = 0;
        return false;
    }
    flags_   = open | create;
    lorigin_ = 0;
    error_   = success;
    return true;
}

bool FileIO::Reopen(uint flg)
{
    if (!path_[0]) return false;
    return Open(path_, flg);
}

void FileIO::Close()
{
    if (fp_) {
        fclose(fp_);
        fp_ = nullptr;
    }
    flags_ = 0;
}

int32 FileIO::Read(void* dest, int32 len)
{
    if (!fp_) return -1;
    size_t n = fread(dest, 1, (size_t)len, fp_);
    return (int32)n;
}

int32 FileIO::Write(const void* src, int32 len)
{
    if (!fp_ || (flags_ & readonly)) return -1;
    size_t n = fwrite(src, 1, (size_t)len, fp_);
    return (int32)n;
}

bool FileIO::Seek(int32 fpos, SeekMethod method)
{
    if (!fp_) return false;
    int whence = SEEK_SET;
    long base = 0;
    switch (method) {
    case begin:   whence = SEEK_SET; base = (long)lorigin_; break;
    case current: whence = SEEK_CUR; base = 0;              break;
    case end:     whence = SEEK_END; base = 0;              break;
    }
    return fseek(fp_, base + (long)fpos, whence) == 0;
}

int32 FileIO::Tellp()
{
    if (!fp_) return 0;
    long pos = ftell(fp_);
    return (int32)(pos - (long)lorigin_);
}

bool FileIO::SetEndOfFile()
{
    // Best-effort: stdio has no portable truncate. The Win32 path used
    // SetEndOfFile() to grow newly-created snapshot files; on POSIX a
    // subsequent fwrite implicitly extends the file, so the call is
    // effectively a no-op for our usage.
    if (!fp_) return false;
    fflush(fp_);
    return true;
}
