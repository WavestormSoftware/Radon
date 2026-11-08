// Copyright (c) 2026 Wavestorm Software
// Android / iOS entry point. SDL provides the activity (Android) and app delegate (iOS).
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>

#include "demo.hpp"
#include "radon_sdl3_gl.hpp"

int main(int, char**) {
    std::vector<uint8_t> font = demo::find_font(nullptr);
    rd::sdl3::PlatformConfig pc;
    pc.title = "Radon demo";
    rd::sdl3::Platform* plat = rd::sdl3::create_platform(pc);
    if (!plat) return 1;
    rd::gl::RendererConfig rc;
    rc.gles = true;
    rd::gl::Renderer* rend = rd::gl::create_renderer(rc);
    if (!rend) return 1;
    uint8_t checker[32 * 32 * 4];
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            bool on = ((x / 8) + (y / 8)) & 1;
            uint8_t* p = &checker[(y * 32 + x) * 4];
            p[0] = on ? 0x4d : 0x20, p[1] = on ? 0xa3 : 0x20, p[2] = on ? 0xff : 0x20, p[3] = 255;
        }
    rd::Context ctx;
    rd::sdl3::attach(plat, ctx);
    demo::App app;
    app.init(ctx, font, rd::gl::create_texture(rend, 32, 32, checker));

    rd::Input input;
    float dt = 0;
    while (rd::sdl3::poll(plat, input, dt)) {
        int fw, fh;
        rd::sdl3::framebuffer_size(plat, fw, fh);
        ctx.set_dpi_scale(rd::sdl3::dpi_scale(plat));
        ctx.begin_frame(input, dt);
        app.frame(ctx, input, (int)((float)fw / ctx.dpi_scale()), (int)((float)fh / ctx.dpi_scale()));
        ctx.end_frame();
        rd::gl::clear(rend, ctx.theme().background);
        rd::gl::render(rend, ctx, fw, fh);
        rd::sdl3::finish_frame(plat, ctx);
    }
    rd::gl::destroy_renderer(rend);
    rd::sdl3::destroy_platform(plat);
    return 0;
}
