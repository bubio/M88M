#include "paths.h"
#include <cstdlib>
#include <sys/stat.h>
#include <vector>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#define getcwd _getcwd
#define mkdir(p, m) _mkdir(p)
#else
#include <unistd.h>
#include <iconv.h>
#include <errno.h>
#endif

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
#ifdef __APPLE__
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (mainBundle) {
        CFURLRef bundleURL = CFBundleCopyBundleURL(mainBundle);
        if (bundleURL) {
            char path[1024];
            if (CFURLGetFileSystemRepresentation(bundleURL, true, (UInt8 *)path, sizeof(path))) {
                std::string fullPath(path);
                // On macOS, if it's a bundle, return the directory containing the bundle
                size_t lastSlash = fullPath.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    CFRelease(bundleURL);
                    return fullPath.substr(0, lastSlash);
                }
            }
            CFRelease(bundleURL);
        }
    }
#endif
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
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
#elif defined(_WIN32)
    const char* appData = std::getenv("APPDATA");
    if (appData) {
        path = std::string(appData) + "/M88M";
    } else {
        const char* userProfile = std::getenv("USERPROFILE");
        if (userProfile) {
            path = std::string(userProfile) + "/.m88m";
        }
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
#if !defined(__APPLE__) && !defined(_WIN32)
        const char* home = std::getenv("HOME");
        if (home) EnsureDirectory(std::string(home) + "/.config");
#endif
        EnsureDirectory(path);
    }
    return path;
}

std::string SJIStoUTF8(const std::string& strOrg) {
    if (strOrg.empty()) return "";

#ifdef _WIN32
    // SJIS (CP932) -> WideChar
    int nw = MultiByteToWideChar(932, 0, strOrg.c_str(), (int)strOrg.length(), nullptr, 0);
    if (nw <= 0) return strOrg;
    std::vector<wchar_t> wbuf(nw);
    MultiByteToWideChar(932, 0, strOrg.c_str(), (int)strOrg.length(), wbuf.data(), nw);

    // WideChar -> UTF-8
    int nu = WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), nw, nullptr, 0, nullptr, nullptr);
    if (nu <= 0) return "";
    std::vector<char> ubuf(nu);
    WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), nw, ubuf.data(), nu, nullptr, nullptr);
    return std::string(ubuf.data(), nu);
#else
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
#endif
}

} // namespace Paths
