#include "audio_out.h"
#include "pc88/config.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#ifdef __HAIKU__
#include <MediaDefs.h>
#include <SoundPlayer.h>
#endif

static RaylibSound* s_current_sound = nullptr;

static void GlobalAudioCallback(void* buffer, unsigned int frames) {
    if (s_current_sound) {
        s_current_sound->FillOutput((int16_t*)buffer, frames);
    }
}

#ifdef __HAIKU__
static void HaikuAudioCallback(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format) {
    RaylibSound* sound = (RaylibSound*)cookie;
    unsigned int frames = (unsigned int)(size / (sizeof(int16_t) * 2));
    sound->FillOutput((int16_t*)buffer, frames);

    static unsigned int callbackCount = 0;
    static unsigned int nonSilentCount = 0;
    int peak = 0;
    int16_t* samples = (int16_t*)buffer;
    size_t sampleCount = size / sizeof(int16_t);
    for (size_t i = 0; i < sampleCount; i++) {
        int value = std::abs((int)samples[i]);
        if (value > peak) peak = value;
    }
    if (peak > 0) nonSilentCount++;
    callbackCount++;
    if ((callbackCount & 63u) == 1u) {
        std::fprintf(stderr, "M88M: haiku audio callback count=%u peak=%d nonSilent=%u rate=%.0f\n",
            callbackCount, peak, nonSilentCount, format.frame_rate);
    }
}
#endif

RaylibSound::RaylibSound() : outputSource(nullptr), sampleRate(48000), streamBufferFrames(0)
#ifdef __HAIKU__
    , haikuPlayer(nullptr)
#endif
{
    stream = {0};
}
RaylibSound::~RaylibSound() { Cleanup(); }

void RaylibSound::Init(uint32 rate, int deviceBufferFrames) {
    sampleRate = rate;
    // The "Sound Buffer" setting drives this device buffer, which is what
    // actually governs dropout resilience. The audio callback resamples and
    // mixes OPNA on demand (fillwhenempty), so the SRC ring size alone does not
    // add real-time slack; only a larger device buffer lets an occasional slow
    // callback finish before the queued audio runs out (at the cost of latency).
    if (deviceBufferFrames < 512) deviceBufferFrames = 512;
    streamBufferFrames = deviceBufferFrames;

#ifdef __HAIKU__
    media_raw_audio_format format = {};
    format.frame_rate = (float)sampleRate;
    format.channel_count = 2;
    format.format = media_raw_audio_format::B_AUDIO_SHORT;
    format.byte_order = B_MEDIA_HOST_ENDIAN;
    format.buffer_size = (size_t)streamBufferFrames * 2 * sizeof(int16_t);

    haikuPlayer = new BSoundPlayer(&format, "M88M", HaikuAudioCallback, nullptr, this);
    status_t status = haikuPlayer ? haikuPlayer->InitCheck() : B_NO_MEMORY;
    if (status != B_OK) {
        std::fprintf(stderr, "M88M: haiku audio init failed status=%ld\n", (long)status);
        delete haikuPlayer;
        haikuPlayer = nullptr;
        return;
    }
    haikuPlayer->SetHasData(true);
    std::fprintf(stderr, "M88M: haiku audio init rate=%d bufferFrames=%d bufferBytes=%zu\n",
        sampleRate, streamBufferFrames, format.buffer_size);
#else
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    SetAudioStreamBufferSizeDefault(deviceBufferFrames);
    stream = LoadAudioStream(sampleRate, 16, 2);
    updateBuffer.assign((size_t)streamBufferFrames * 2, 0);
#endif
}

void RaylibSound::Cleanup() {
#ifdef __HAIKU__
    if (haikuPlayer) {
        haikuPlayer->Stop(true, true);
        delete haikuPlayer;
        haikuPlayer = nullptr;
    }
#else
    if (IsAudioStreamValid(stream)) { StopAudioStream(stream); UnloadAudioStream(stream); stream = {0}; }
    CloseAudioDevice();
    if (s_current_sound == this) s_current_sound = nullptr;
#endif
    updateBuffer.clear();
    streamBufferFrames = 0;
}

void RaylibSound::ClearBuffer() {
#ifdef __HAIKU__
    if (haikuPlayer) haikuPlayer->SetHasData(true);
#else
    if (IsAudioStreamValid(stream)) {
        StopAudioStream(stream);
        PlayAudioStream(stream);
    }
#endif
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
    if (haikuPlayer) {
        status_t status = haikuPlayer->Start();
        haikuPlayer->SetHasData(true);
        std::fprintf(stderr, "M88M: haiku audio started status=%ld\n", (long)status);
    }
#else
    s_current_sound = this;
    SetAudioStreamCallback(stream, GlobalAudioCallback);
    PlayAudioStream(stream);
#endif
}

void RaylibSound::Pause(bool paused) {
#ifdef __HAIKU__
    if (haikuPlayer) haikuPlayer->SetHasData(!paused);
#else
    if (IsAudioStreamValid(stream)) {
        if (paused) PauseAudioStream(stream);
        else ResumeAudioStream(stream);
    }
#endif
}

void RaylibSound::SetVolume(const PC8801::Config* cfg) {
    if (cfg) {
#ifdef __HAIKU__
        if (haikuPlayer) haikuPlayer->SetVolume((float)cfg->mastervol / 128.0f);
#else
        ::SetMasterVolume((float)cfg->mastervol / 128.0f);
#endif
    }
}
