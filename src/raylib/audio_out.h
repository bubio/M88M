#pragma once

#include "raylib.h"
#include "soundsrc.h"
#include <mutex>
#include <cstdint>

namespace PC8801 { class Config; }

class RaylibSound {
public:
    RaylibSound();
    virtual ~RaylibSound();

    void Init();
    void Start();
    void Pause(bool paused);
    void SetVolume(const PC8801::Config* cfg);
    void Cleanup();
    void SetSource(SoundSource* src);

    void FillOutput(int16_t* buffer, unsigned int frames);

private:
    static void AudioCallback(void* buffer, unsigned int frames);

    AudioStream stream;
    SoundSource* outputSource;
    std::recursive_mutex mutex;
    int sampleRate;
};
