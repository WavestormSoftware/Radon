// Copyright (c) 2026 Wavestorm Software
// Web demo: the same UI as the desktop demo, driven by emscripten_set_main_loop.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <emscripten.h>

#include <cstdio>

#include "demo.hpp"
#include "radon_sdl3_gl.hpp"

namespace {
struct State {
    rd::sdl3::Platform* plat = nullptr;
    rd::gl::Renderer* rend = nullptr;
    rd::Context* ctx = nullptr;
    demo::App app;
    rd::Input input;
    std::vector<uint8_t> font;
};

void loop(void* arg) {
    State& s = *static_cast<State*>(arg);
    float dt = 0;
    if (!rd::sdl3::poll(s.plat, s.input, dt)) {
        emscripten_cancel_main_loop();
        return;
    }
    int fw, fh;
    rd::sdl3::framebuffer_size(s.plat, fw, fh);
    s.ctx->set_dpi_scale(rd::sdl3::dpi_scale(s.plat));
    s.ctx->begin_frame(s.input, dt);
    s.app.frame(*s.ctx, s.input, (int)((float)fw / s.ctx->dpi_scale()), (int)((float)fh / s.ctx->dpi_scale()));
    s.ctx->end_frame();
    rd::gl::clear(s.rend, s.ctx->theme().background);
    rd::gl::render(s.rend, *s.ctx, fw, fh);
    rd::sdl3::finish_frame(s.plat, *s.ctx);
}
}  // namespace

int main(int, char**) {
    static State s;
    s.font = demo::find_font(nullptr);
    rd::sdl3::PlatformConfig pc;
    pc.title = "Radon demo";
    s.plat = rd::sdl3::create_platform(pc);
    if (!s.plat) {
        std::fprintf(stderr, "radon web: cannot create window: %s\n", SDL_GetError());
        return 1;
    }
    rd::gl::RendererConfig rc;
    rc.gles = true;
    s.rend = rd::gl::create_renderer(rc);
    if (!s.rend) return 1;
    static uint8_t checker[32 * 32 * 4];
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            bool on = ((x / 8) + (y / 8)) & 1;
            uint8_t* p = &checker[(y * 32 + x) * 4];
            p[0] = on ? 0x4d : 0x20, p[1] = on ? 0xa3 : 0x20, p[2] = on ? 0xff : 0x20, p[3] = 255;
        }
    s.ctx = new rd::Context;
    rd::sdl3::attach(s.plat, *s.ctx);
    s.app.init(*s.ctx, s.font, rd::gl::create_texture(s.rend, 32, 32, checker));
    emscripten_set_main_loop_arg(loop, &s, 0, 1);
    return 0;
}
