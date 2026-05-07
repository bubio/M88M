#pragma once

#include <string>

namespace Paths {
    std::string GetRomDir();
    std::string GetConfigDir();
    std::string GetAppDir();
    
    bool DirectoryExists(const std::string& path);
    bool FileExists(const std::string& path);
    void EnsureDirectory(const std::string& path);
    long long GetFileModTime(const std::string& path);
    
    std::string SJIStoUTF8(const std::string& sjis);
    std::string NormalizeNFC(const std::string& input);

#ifdef _WIN32
    std::wstring UTF8ToWide(const std::string& utf8);
#endif
}
