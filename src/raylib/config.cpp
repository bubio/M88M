#include "config.h"
#include <cstring>

namespace Config {

static PC8801::Config g_config;

void Load(PC8801::Config& cfg) {
    memset(&cfg, 0, sizeof(cfg));
    
    // Default settings
    cfg.basicmode = PC8801::Config::N88V2;
    cfg.clock = 4; // 4MHz
    cfg.speed = 100;
    cfg.mainsubratio = 1;
    cfg.cpumode = PC8801::Config::msauto;
    
    // Flags
    cfg.flags = PC8801::Config::enableopna | 
                PC8801::Config::subcpucontrol |
                PC8801::Config::precisemixing |
                PC8801::Config::mixsoundalways;
    
    cfg.flag2 = PC8801::Config::usefmclock;

    // Volume
    cfg.volfm = 64;
    cfg.volssg = 64;
    cfg.voladpcm = 64;
    cfg.volrhythm = 64;
    
    cfg.volbd = 64;
    cfg.volsd = 64;
    cfg.voltop = 64;
    cfg.volhh = 64;
    cfg.voltom = 64;
    cfg.volrim = 64;
}

void Save(const PC8801::Config& cfg) {
    // TODO: Implement INI saving
}

PC8801::Config& Get() {
    static bool initialized = false;
    if (!initialized) {
        Load(g_config);
        initialized = true;
    }
    return g_config;
}

}
