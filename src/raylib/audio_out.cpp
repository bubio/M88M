#include "audio_out.h"
#include "pc88/config.h"
#include <algorithm>
#include <iostream>
#include <cstdio>

static RaylibSound* s_current_sound = nullptr;

static void GlobalAudioCallback(void* buffer, unsigned int frames) {
    if (s_current_sound) {
        short* out = (short*)buffer;
        static std::vector<int32_t> mixBuf;
        mixBuf.assign(frames * 2, 0);
        s_current_sound->MixInternal(mixBuf.data(), frames);

        long long sum = 0;
        for (unsigned int i = 0; i < frames * 2; i++) {
            int32_t s = mixBuf[i];
            if (s > 32767) s = 32767;
            if (s < -32768) s = -32768;
            out[i] = (short)s;
            sum += (s < 0 ? -s : s);
        }
        
        static int audioCounter = 0;
        if (audioCounter++ % 100 == 0 && sum > 0) {
            fprintf(stderr, "[VERIFY] Audio is ACTIVE (avg amplitude: %lld)\n", sum / (frames * 2));
            fflush(stderr);
        }
    }
}

RaylibSound::RaylibSound() : sampleRate(44100) { stream = {0}; }
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

bool IFCALL RaylibSound::Connect(ISoundSource* src) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    for (auto* s : sources) if (s == src) return true;
    sources.push_back(src);
    src->SetRate(sampleRate);
    return true;
}

bool IFCALL RaylibSound::Disconnect(ISoundSource* src) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = std::find(sources.begin(), sources.end(), src);
    if (it != sources.end()) { sources.erase(it); return true; }
    return false;
}

bool IFCALL RaylibSound::Update(ISoundSource* src) { return true; }
int IFCALL RaylibSound::GetSubsampleTime(ISoundSource* src) { return 0; }

void RaylibSound::MixInternal(int32_t* buffer, unsigned int frames) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    for (auto* src : sources) src->Mix(buffer, frames);
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
