#pragma once

#include "raylib.h"
#include "soundsrc.h"
#include <atomic>
#include <cstdint>
#include <vector>

namespace PC8801 { class Config; }

class RaylibSound {
public:
    RaylibSound();
    virtual ~RaylibSound();

    void Init(uint32 rate, int deviceBufferFrames);
    void Start();
    void Pause(bool paused);
    void SetVolume(const PC8801::Config* cfg);
    void Cleanup();
    void ClearBuffer();
    void SetSource(SoundSource* src);

    void FillOutput(int16_t* buffer, unsigned int frames);
    void Update();

private:
    static void AudioCallback(void* buffer, unsigned int frames);

    AudioStream stream;
    std::atomic<SoundSource*> outputSource;
    int sampleRate;
    int streamBufferFrames;
    std::vector<int16_t> updateBuffer;
};
