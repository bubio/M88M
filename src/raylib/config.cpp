#include "config.h"
#include "paths.h"
#include "common/file.h"
#include <cstring>
#include <sys/stat.h>

namespace Config {

static PC8801::Config g_config;

static std::string GetConfigFilePath() {
    std::string dir = Paths::GetConfigDir();
    struct stat st;
    if (stat(dir.c_str(), &st) != 0) {
        mkdir(dir.c_str(), 0755);
    }
    return dir + "/config.bin";
}

void Load(PC8801::Config& cfg) {
    memset(&cfg, 0, sizeof(cfg));

    // Set defaults first
    cfg.basicmode = PC8801::Config::N88V2;
    cfg.clock = 40; // 4MHz
    cfg.mainsubratio = 1; // 4MHz mode uses ratio 1
    cfg.speed = 100; // matches PC88::Proceed eff = clock * speed / 100
    cfg.cpumode = PC8801::Config::msauto;
    cfg.dipsw = 1829; // Includes bit 5 = 1 (4MHz)
    cfg.flags = PC8801::Config::enableopna |
                PC8801::Config::subcpucontrol |
                PC8801::Config::enablewait |
                PC8801::Config::precisemixing |
                PC8801::Config::mixsoundalways;
    cfg.flag2 = 0;
    cfg.volfm = 0; cfg.volssg = 0; cfg.voladpcm = 0; cfg.volrhythm = 0;
    cfg.volbd = 0; cfg.volsd = 0; cfg.voltop = 0;
    cfg.volhh = 0; cfg.voltom = 0; cfg.volrim = 0;
    cfg.mastervol = 64; // 50%
    cfg.soundbuffer = 4096;

    // Try to load from file
    std::string path = GetConfigFilePath();
    FileIO file;
    if (file.Open(path.c_str(), FileIO::open | FileIO::readonly)) {
        file.Read(&cfg, sizeof(cfg));
        file.Close();
        // Recalculate mainsubratio in case it was saved inconsistently
        cfg.mainsubratio = (cfg.clock >= 60) ? 2 : 1;
    } else {
        // Save defaults if file doesn't exist
        Save(cfg);
    }
}

void Save(const PC8801::Config& cfg) {
    std::string path = GetConfigFilePath();
    FileIO file;
    if (file.CreateNew(path.c_str())) {
        file.Write(&cfg, sizeof(cfg));
        file.Close();
        fprintf(stderr, "[Config] Saved to %s\n", path.c_str());
    } else {
        fprintf(stderr, "[Config] Failed to create save file: %s\n", path.c_str());
    }
}

PC8801::Config& Get() {
    static bool initialized = false;
    if (!initialized) { Load(g_config); initialized = true; }
    return g_config;
}

}
