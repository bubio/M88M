#include "paths.h"
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

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

} // namespace Paths
