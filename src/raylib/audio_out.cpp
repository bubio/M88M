#include "audio_out.h"
#include <algorithm>
#include <iostream>

// Global pointer for the static callback to use
static RaylibSound* s_current_sound = nullptr;

static void GlobalAudioCallback(void* buffer, unsigned int frames) {
    if (s_current_sound) {
        short* out = (short*)buffer;
        static std::vector<int32_t> mixBuf;
        mixBuf.assign(frames * 2, 0);

        s_current_sound->MixInternal(mixBuf.data(), frames);

        for (unsigned int i = 0; i < frames * 2; i++) {
            int32_t s = mixBuf[i];
            if (s > 32767) s = 32767;
            if (s < -32768) s = -32768;
            out[i] = (short)s;
        }
    }
}

RaylibSound::RaylibSound() : sampleRate(44100) {
    stream = {0};
}

RaylibSound::~RaylibSound() {
    Cleanup();
}

void RaylibSound::Init() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        std::cerr << "Failed to initialize audio device" << std::endl;
        return;
    }

    SetAudioStreamBufferSizeDefault(1024);
    stream = LoadAudioStream(sampleRate, 16, 2);
    // Callback will be set in Start()
}

void RaylibSound::Cleanup() {
    if (IsAudioStreamValid(stream)) {
        StopAudioStream(stream);
        UnloadAudioStream(stream);
        stream = {0};
    }
    CloseAudioDevice();
    if (s_current_sound == this) s_current_sound = nullptr;
}

bool IFCALL RaylibSound::Connect(ISoundSource* src) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    sources.push_back(src);
    src->Connect(this);
    src->SetRate(sampleRate);
    return true;
}

bool IFCALL RaylibSound::Disconnect(ISoundSource* src) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = std::find(sources.begin(), sources.end(), src);
    if (it != sources.end()) {
        sources.erase(it);
        return true;
    }
    return false;
}

bool IFCALL RaylibSound::Update(ISoundSource* src) {
    return true;
}

int IFCALL RaylibSound::GetSubsampleTime(ISoundSource* src) {
    return 0;
}

void RaylibSound::MixInternal(int32_t* buffer, unsigned int frames) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    for (auto* src : sources) {
        src->Mix(buffer, frames);
    }
}

void RaylibSound::Start() {
    s_current_sound = this;
    SetAudioStreamCallback(stream, GlobalAudioCallback);
    PlayAudioStream(stream);
}
