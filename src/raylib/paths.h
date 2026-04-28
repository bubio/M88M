#pragma once

#include <string>

namespace Paths {
    std::string GetRomDir();
    std::string GetConfigDir();
    std::string GetAppDir();
    
    std::string SJIStoUTF8(const std::string& sjis);
}
