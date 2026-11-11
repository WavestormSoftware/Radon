// Copyright (c) 2026 Wavestorm Software
// Fire-and-forget sound effects on SDL3 audio streams (SDL mixes streams that share a device).
#include <SDL3/SDL.h>

#include "radon_sdl3_gl.hpp"

namespace rd::sdl3 {

struct Sound {
    SDL_AudioSpec spec{};
    Uint8* data = nullptr;
    Uint32 len = 0;
};

namespace {
constexpr int kVoices = 8;
SDL_AudioStream* g_voice[kVoices] = {};
bool g_init = false;

bool ensure_audio() {
    if (!g_init) g_init = SDL_InitSubSystem(SDL_INIT_AUDIO);
    return g_init;
}

Sound* finish(SDL_IOStream* io) {
    if (!io || !ensure_audio()) {
        if (io) SDL_CloseIO(io);
        return nullptr;
    }
    auto* s = new Sound;
    if (!SDL_LoadWAV_IO(io, true, &s->spec, &s->data, &s->len)) {
        delete s;
        return nullptr;
    }
    return s;
}
}  // namespace

Sound* load_sound(const char* path) {
    return finish(SDL_IOFromFile(path, "rb"));
}

Sound* load_sound_memory(const uint8_t* wav, size_t size) {
    return wav && size ? finish(SDL_IOFromConstMem(wav, size)) : nullptr;
}

bool play_sound(Sound* s, float volume) {
    if (!s || !g_init) return false;
    for (SDL_AudioStream*& v : g_voice) {
        if (!v) {
            v = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &s->spec, nullptr, nullptr);
            if (!v) return false;
            SDL_ResumeAudioStreamDevice(v);
        } else if (SDL_GetAudioStreamQueued(v) > 0 || SDL_GetAudioStreamAvailable(v) > 0) {
            continue;  // still playing
        }
        SDL_SetAudioStreamFormat(v, &s->spec, nullptr);
        SDL_SetAudioStreamGain(v, volume < 0 ? 0 : (volume > 1 ? 1 : volume));
        return SDL_PutAudioStreamData(v, s->data, (int)s->len);
    }
    return false;
}

void destroy_sound(Sound* s) {
    if (!s) return;
    SDL_free(s->data);
    delete s;
}

void audio_shutdown() {
    for (SDL_AudioStream*& v : g_voice) {
        if (v) SDL_DestroyAudioStream(v);
        v = nullptr;
    }
    if (g_init) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    g_init = false;
}

}  // namespace rd::sdl3
