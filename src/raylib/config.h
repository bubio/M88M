#pragma once

#include "pc88/config.h"

namespace Config {
    void Load(PC8801::Config& cfg);
    void Save(const PC8801::Config& cfg);
    
    // Get current global config
    PC8801::Config& Get();
}
