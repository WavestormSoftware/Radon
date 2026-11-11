// Copyright (c) 2026 Wavestorm Software
// Game pieces in one window: a 2D canvas with a moving sprite, a sound effect and gamepad state.
#include <SDL3/SDL_main.h>

#include <radon_sdl3_gl.hpp>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "runner.hpp"

// A 0.15 s 880 Hz beep as an in-memory 16-bit mono WAV, so the example needs no asset.
static std::vector<uint8_t> make_beep() {
    const int rate = 22050, n = rate * 15 / 100;
    std::vector<uint8_t> w(44 + (size_t)n * 2);
    auto u32 = [&](size_t o, uint32_t v) { std::memcpy(&w[o], &v, 4); };
    auto u16 = [&](size_t o, uint16_t v) { std::memcpy(&w[o], &v, 2); };
    std::memcpy(&w[0], "RIFF", 4);
    u32(4, (uint32_t)w.size() - 8);
    std::memcpy(&w[8], "WAVEfmt ", 8);
    u32(16, 16);
    u16(20, 1);
    u16(22, 1);
    u32(24, rate);
    u32(28, rate * 2);
    u16(32, 2);
    u16(34, 16);
    std::memcpy(&w[36], "data", 4);
    u32(40, (uint32_t)n * 2);
    for (int i = 0; i < n; i++) {
        const float env = 1.0f - (float)i / (float)n;
        u16(44 + (size_t)i * 2,
            (uint16_t)(int16_t)(8000.0f * env * std::sin(6.2831853f * 880.0f * (float)i / rate)));
    }
    return w;
}

int main(int argc, char** argv) {
    static rd::Rect win{20, 20, 400, 360};
    static demo::Runner* run = nullptr;
    static rd::sdl3::Sound* beep = nullptr;
    static float x = 200, y = 80, vx = 140, vy = 110, size = 24;
    demo::AppConfig cfg;
    cfg.title = "Game canvas";
    cfg.width = 440, cfg.height = 400;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        const float dt = 1.0f / 60.0f;
        rd::sdl3::GamepadState pad;
        const bool has_pad = rd::sdl3::gamepad_state(run->platform(), 0, pad);
        if (has_pad) vx += pad.left.x * 400 * dt, vy += pad.left.y * 400 * dt;
        x += vx * dt, y += vy * dt;
        const float W = 360, H = 200;
        bool bounced = false;
        if (x < 0 || x + size > W) vx = -vx, x = x < 0 ? 0 : W - size, bounced = true;
        if (y < 0 || y + size > H) vy = -vy, y = y < 0 ? 0 : H - size, bounced = true;
        if (bounced) rd::sdl3::play_sound(beep, 0.3f);

        if (rd::begin_window(ui, "Game pieces", win)) {
            rd::layout_row(ui, 1, nullptr, 0, 0);
            rd::Canvas c = rd::canvas_begin(ui, {W, H}, {20, 24, 36, 255});
            c.line({0, H / 2}, {W, H / 2}, {60, 70, 90, 255});
            c.fill_rect({x, y, size, size}, {255, 160, 0, 255}, 4);
            c.circle({W / 2, H / 2}, 14, {80, 200, 255, 255});
            if (c.response.pressed) x = c.local.x - size / 2, y = c.local.y - size / 2;
            rd::canvas_end(ui);
            char text[96];
            std::snprintf(text, sizeof text,
                          has_pad ? "gamepad: stick %.2f, %.2f" : "no gamepad (click the canvas)", pad.left.x,
                          pad.left.y);
            rd::label(ui, text);
            if (rd::button(ui, "Play sound").clicked) rd::sdl3::play_sound(beep);
            rd::end_window(ui);
        }
    };
    demo::Runner r;
    run = &r;
    if (!r.start(argc, argv, cfg)) return 1;
    const std::vector<uint8_t> wav = make_beep();
    beep = rd::sdl3::load_sound_memory(wav.data(), wav.size());
    while (r.step()) {
    }
    rd::sdl3::destroy_sound(beep);
    r.stop();
    return 0;
}
