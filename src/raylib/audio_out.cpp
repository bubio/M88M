#include "audio_out.h"
#include "pc88/config.h"
#include <algorithm>

static RaylibSound* s_current_sound = nullptr;

static void GlobalAudioCallback(void* buffer, unsigned int frames) {
    if (s_current_sound) {
        s_current_sound->FillOutput((int16_t*)buffer, frames);
    }
}

RaylibSound::RaylibSound() : outputSource(nullptr), sampleRate(48000) { stream = {0}; }
RaylibSound::~RaylibSound() { Cleanup(); }

void RaylibSound::Init() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    SetAudioStreamBufferSizeDefault(1024);
    stream = LoadAudioStream(sampleRate, 16, 2);
}

void RaylibSound::Cleanup() {
    if (IsAudioStreamValid(stream)) { StopAudioStream(stream); UnloadAudioStream(stream); stream = {0}; }
    CloseAudioDevice();
    if (s_current_sound == this) s_current_sound = nullptr;
}

void RaylibSound::ClearBuffer() {
    if (IsAudioStreamValid(stream)) {
        StopAudioStream(stream);
        PlayAudioStream(stream);
    }
}

void RaylibSound::SetSource(SoundSource* src) {
    outputSource.store(src, std::memory_order_release);
}

void RaylibSound::FillOutput(int16_t* buffer, unsigned int frames) {
    SoundSource* src = outputSource.load(std::memory_order_acquire);
    if (src) {
        int got = src->Get(buffer, frames);
        if (got < (int)frames) {
            std::fill(buffer + got * 2, buffer + frames * 2, 0);
        }
    } else {
        std::fill(buffer, buffer + frames * 2, 0);
    }
}

void RaylibSound::Start() {
    s_current_sound = this;
    SetAudioStreamCallback(stream, GlobalAudioCallback);
    PlayAudioStream(stream);
}

void RaylibSound::Pause(bool paused) {
    if (IsAudioStreamValid(stream)) {
        if (paused) PauseAudioStream(stream);
        else ResumeAudioStream(stream);
    }
}

void RaylibSound::SetVolume(const PC8801::Config* cfg) {
    if (cfg) {
        ::SetMasterVolume((float)cfg->mastervol / 128.0f);
    }
}
