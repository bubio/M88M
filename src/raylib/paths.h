#pragma once

#include <string>

namespace Paths {
    std::string GetRomDir();
    std::string GetConfigDir();
    std::string GetAppDir();
    
    bool DirectoryExists(const std::string& path);
    void EnsureDirectory(const std::string& path);
    
    std::string SJIStoUTF8(const std::string& sjis);
    std::string NormalizeNFC(const std::string& input);
}
