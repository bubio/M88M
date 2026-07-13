#include "audio_out.h"
#include "pc88/config.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

static RaylibSound* s_current_sound = nullptr;

static void GlobalAudioCallback(void* buffer, unsigned int frames) {
    if (s_current_sound) {
        s_current_sound->FillOutput((int16_t*)buffer, frames);
    }
}

RaylibSound::RaylibSound() : outputSource(nullptr), sampleRate(48000), streamBufferFrames(0) { stream = {0}; }
RaylibSound::~RaylibSound() { Cleanup(); }

void RaylibSound::Init(uint32 rate, int deviceBufferFrames) {
    sampleRate = rate;
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    // The "Sound Buffer" setting drives this device buffer, which is what
    // actually governs dropout resilience. The audio callback resamples and
    // mixes OPNA on demand (fillwhenempty), so the SRC ring size alone does not
    // add real-time slack; only a larger device buffer lets an occasional slow
    // callback finish before the queued audio runs out (at the cost of latency).
    if (deviceBufferFrames < 512) deviceBufferFrames = 512;
    SetAudioStreamBufferSizeDefault(deviceBufferFrames);
    stream = LoadAudioStream(sampleRate, 16, 2);
    streamBufferFrames = deviceBufferFrames;
    updateBuffer.assign((size_t)streamBufferFrames * 2, 0);
#ifdef __HAIKU__
    std::fprintf(stderr, "M88M: audio init rate=%d bufferFrames=%d streamValid=%d deviceReady=%d\n",
        sampleRate, streamBufferFrames, IsAudioStreamValid(stream) ? 1 : 0, IsAudioDeviceReady() ? 1 : 0);
#endif
}

void RaylibSound::Cleanup() {
    if (IsAudioStreamValid(stream)) { StopAudioStream(stream); UnloadAudioStream(stream); stream = {0}; }
    CloseAudioDevice();
    if (s_current_sound == this) s_current_sound = nullptr;
    updateBuffer.clear();
    streamBufferFrames = 0;
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
#ifdef __HAIKU__
    if (IsAudioStreamValid(stream)) {
        PlayAudioStream(stream);
        std::fprintf(stderr, "M88M: audio started with polling stream updates\n");
    }
#else
    s_current_sound = this;
    SetAudioStreamCallback(stream, GlobalAudioCallback);
    PlayAudioStream(stream);
#endif
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

void RaylibSound::Update() {
#ifdef __HAIKU__
    if (!IsAudioStreamValid(stream) || updateBuffer.empty()) return;

    static unsigned int updateCount = 0;
    static unsigned int nonSilentCount = 0;

    int updates = 0;
    while (IsAudioStreamProcessed(stream) && updates < 4) {
        FillOutput(updateBuffer.data(), (unsigned int)streamBufferFrames);
        int peak = 0;
        for (int16_t sample : updateBuffer) {
            int value = std::abs((int)sample);
            if (value > peak) peak = value;
        }
        if (peak > 0) nonSilentCount++;
        UpdateAudioStream(stream, updateBuffer.data(), streamBufferFrames);
        updateCount++;
        updates++;

        if ((updateCount & 63u) == 1u) {
            std::fprintf(stderr, "M88M: audio update count=%u peak=%d nonSilent=%u\n",
                updateCount, peak, nonSilentCount);
        }
    }
#endif
}
