#include "config.h"
#include <cstring>

namespace Config {

static PC8801::Config g_config;

void Load(PC8801::Config& cfg) {
    memset(&cfg, 0, sizeof(cfg));
    
    cfg.basicmode = PC8801::Config::N88V2; // V2 Mode (SR or later)
    cfg.clock = 4; // 4MHz
    cfg.speed = 100;
    cfg.mainsubratio = 1;
    cfg.cpumode = PC8801::Config::msauto;
    
    // M88オリジナルのデフォルト値 (1829 = 0x725)
    // M88オリジナルのデフォルト (0x725)
    cfg.dipsw = 1829;
    
    cfg.flags = PC8801::Config::enableopna | 
                PC8801::Config::subcpucontrol |
                PC8801::Config::precisemixing |
                PC8801::Config::mixsoundalways;
    
    cfg.flag2 = PC8801::Config::usefmclock;
    
    cfg.volfm = 64; cfg.volssg = 64; cfg.voladpcm = 64; cfg.volrhythm = 64;
}

void Save(const PC8801::Config& cfg) {}

PC8801::Config& Get() {
    static bool initialized = false;
    if (!initialized) { Load(g_config); initialized = true; }
    return g_config;
}

}
