#pragma once

#include "ifcommon.h"
#include "raylib.h"
#include <vector>
#include <mutex>
#include <cstdint>

namespace PC8801 { class Config; }

class RaylibSound : public ISoundControl {
public:
    RaylibSound();
    virtual ~RaylibSound();

    // ISoundControl implementation
    bool IFCALL Connect(ISoundSource* src) override;
    bool IFCALL Disconnect(ISoundSource* src) override;
    
    bool IFCALL Update(ISoundSource* src) override;
    int  IFCALL GetSubsampleTime(ISoundSource* src) override;

    void Init();
    void Start();
    void Pause(bool paused);
    void SetVolume(const PC8801::Config* cfg);
    void Cleanup();

    void MixInternal(int32_t* buffer, unsigned int frames);

private:
    static void AudioCallback(void* buffer, unsigned int frames);

    AudioStream stream;
    std::vector<ISoundSource*> sources;
    std::recursive_mutex mutex;
    int sampleRate;
};
