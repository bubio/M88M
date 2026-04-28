#include "paths.h"
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <iconv.h>
#include <vector>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace Paths {

static bool DirectoryExists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) return false;
    return (info.st_mode & S_IFDIR) != 0;
}

static void EnsureDirectory(const std::string& path) {
    if (!DirectoryExists(path)) {
        mkdir(path.c_str(), 0755);
    }
}

std::string GetAppDir() {
    char buffer[1024];
    uint32_t size = sizeof(buffer);
    if (getcwd(buffer, size) != nullptr) {
        return std::string(buffer);
    }
    return ".";
}

std::string GetRomDir() {
    const char* env = std::getenv("M88M_ROM_DIR");
    if (env) return std::string(env);

    std::string appDir = GetAppDir();
    std::string localRom = appDir + "/roms";
    if (DirectoryExists(localRom)) return localRom;

    return GetConfigDir() + "/roms";
}

std::string GetConfigDir() {
    std::string path;
#ifdef __APPLE__
    const char* home = std::getenv("HOME");
    if (home) {
        path = std::string(home) + "/Library/Application Support/M88M";
    }
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg) {
        path = std::string(xdg) + "/m88m";
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            path = std::string(home) + "/.config/m88m";
        }
    }
#endif
    if (!path.empty()) {
        // Recursive directory creation would be better, but for now:
#ifndef __APPLE__
        EnsureDirectory(std::string(std::getenv("HOME")) + "/.config");
#endif
        EnsureDirectory(path);
    }
    return path;
}

std::string SJIStoUTF8(const std::string& strOrg) {
    if (strOrg.empty()) return "";

    const char* pszEncSrc = "CP932";
    const char* pszEncDst = "UTF-8";

    iconv_t hConv = iconv_open(pszEncDst, pszEncSrc);
    if (hConv == (iconv_t)-1) {
        hConv = iconv_open(pszEncDst, "SHIFT-JIS");
    }
    
    if (hConv == (iconv_t)-1) return strOrg;

    std::vector<char> vectConv(strOrg.length() * 4 + 16, 0);
    char* pszSrc = const_cast<char*>(strOrg.c_str());
    size_t nSrcLeft = strOrg.length();
    char* pszDst = &vectConv[0];
    size_t nDstLeft = vectConv.size();

    while (nSrcLeft > 0) {
        size_t nResult = iconv(hConv, &pszSrc, &nSrcLeft, &pszDst, &nDstLeft);
        if (nResult != (size_t)-1) {
            continue;
        }
        if (errno == E2BIG) {
            size_t nUsed = vectConv.size() - nDstLeft;
            vectConv.resize(vectConv.size() * 2 + 16, 0);
            pszDst = &vectConv[0] + nUsed;
            nDstLeft = vectConv.size() - nUsed;
            continue;
        }
        break;
    }
    
    std::string result(&vectConv[0], vectConv.size() - nDstLeft);
    iconv_close(hConv);
    return result;
}

} // namespace Paths
