// Copyright (c) 2026 Wavestorm Software
// Desktop demo. Options: --font <ttf>  --screenshot <png>  --frames <n>  --hover-demo
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "demo.hpp"
#include "radon_sdl3_gl.hpp"

int main(int argc, char** argv) {
    const char* font_path = nullptr;
    const char* shot = nullptr;
    int frames = 0;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--font") && i + 1 < argc) font_path = argv[++i];
        else if (!std::strcmp(argv[i], "--screenshot") && i + 1 < argc) shot = argv[++i];
        else if (!std::strcmp(argv[i], "--frames") && i + 1 < argc) frames = std::atoi(argv[++i]);
    }
    if (shot && frames <= 0) frames = 30;

    std::vector<uint8_t> font = demo::find_font(font_path);

    rd::sdl3::PlatformConfig pc;
    pc.title = "Radon demo";
    rd::sdl3::Platform* plat = rd::sdl3::create_platform(pc);
    if (!plat) {
        std::fprintf(stderr, "radon demo: cannot create window/GL context: %s\n", SDL_GetError());
        return 1;
    }
    rd::gl::RendererConfig rc;
    rc.gles = rd::sdl3::is_gles(plat);
    rd::gl::Renderer* rend = rd::gl::create_renderer(rc);
    if (!rend) {
        std::fprintf(stderr, "radon demo: renderer setup failed\n");
        return 1;
    }
    uint8_t checker[32 * 32 * 4];
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            bool on = ((x / 8) + (y / 8)) & 1;
            uint8_t* p = &checker[(y * 32 + x) * 4];
            p[0] = on ? 0x4d : 0x20;
            p[1] = on ? 0xa3 : 0x20;
            p[2] = on ? 0xff : 0x20;
            p[3] = 255;
        }

    rd::Context ctx;
    rd::sdl3::attach(plat, ctx);
    demo::App app;
    app.init(ctx, font, rd::gl::create_texture(rend, 32, 32, checker));

    rd::Input input;
    float dt = 0;
    int n = 0;
    int ret = 0;
    while (rd::sdl3::poll(plat, input, dt)) {
        int fw, fh;
        rd::sdl3::framebuffer_size(plat, fw, fh);
        ctx.set_dpi_scale(rd::sdl3::dpi_scale(plat));
        if (shot) {  // scripted pointer: hover the "Click me" button area so the shot shows a hover state
            input.pointer = {240, 150};
            input.pointer_down = false;
        }
        ctx.begin_frame(input, dt);
        app.frame(ctx, input, (int)((float)fw / ctx.dpi_scale()), (int)((float)fh / ctx.dpi_scale()));
        ctx.end_frame();

        rd::gl::clear(rend, ctx.theme().background);
        rd::gl::render(rend, ctx, fw, fh);
        if (shot && ++n >= frames) {
            std::vector<uint8_t> px((size_t)fw * fh * 4);
            rd::gl::read_pixels(rend, fw, fh, px.data());
            SDL_Surface* s = SDL_CreateSurfaceFrom(fw, fh, SDL_PIXELFORMAT_RGBA32, px.data(), fw * 4);
            if (s && SDL_SavePNG(s, shot)) std::printf("radon demo: wrote %s (%dx%d)\n", shot, fw, fh);
            else { std::fprintf(stderr, "radon demo: screenshot failed: %s\n", SDL_GetError()); ret = 1; }
            if (s) SDL_DestroySurface(s);
            break;
        }
        rd::sdl3::finish_frame(plat, ctx);
    }
    rd::gl::destroy_renderer(rend);
    rd::sdl3::destroy_platform(plat);
    return ret;
}
