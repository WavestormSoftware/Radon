// Copyright (c) 2026 Wavestorm Software
// The Radon gallery. Options: --page N --light --size WxH --font <ttf> --screenshot <png> --frames N
// --pointer x,y
#include <SDL3/SDL_main.h>

#include "gallery.hpp"

int main(int argc, char** argv) {
    demo::Gallery gallery;
    demo::Runner runner;
    demo::AppConfig cfg;
    cfg.title = "Radon gallery";
    cfg.init = [&](rd::Context&, const demo::Fonts& f, const demo::Options& o) { gallery.setup(f, o.page); };
    cfg.frame = [&](rd::Context& ctx, const rd::Input& in, int w, int h) { gallery.frame(ctx, in, w, h); };
    cfg.system_theme = [&](rd::Context& ctx, bool dark) { gallery.set_dark(ctx, dark); };
    if (!runner.start(argc, argv, cfg)) return 1;
    gallery.set_runner(&runner);
    auto art = demo::art_rgba(256, 128);
    gallery.set_checker(runner.create_texture(256, 128, art.data()));
    while (runner.step()) {
    }
    return 0;
}
