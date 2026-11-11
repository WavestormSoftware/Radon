// Copyright (c) 2026 Wavestorm Software
// A full-window "creative app" layout: menu bar, toolbar, canvas, color / brush / layer panels.
// The code lives in examples/common/gallery.cpp (Gallery::dashboard); this just opens that page.
#include <SDL3/SDL_main.h>

#include "gallery.hpp"

int main(int argc, char** argv) {
    demo::Gallery gallery;
    demo::Runner runner;
    demo::AppConfig cfg;
    cfg.title = "Dashboard";
    cfg.width = 1280, cfg.height = 760;
    cfg.init = [&](rd::Context&, const demo::Fonts& f, const demo::Options&) { gallery.setup(f, 5); };
    cfg.frame = [&](rd::Context& ctx, const rd::Input& in, int w, int h) { gallery.frame(ctx, in, w, h); };
    if (!runner.start(argc, argv, cfg)) return 1;
    auto art = demo::art_rgba(512, 256);
    gallery.set_checker(runner.create_texture(512, 256, art.data()));
    while (runner.step()) {
    }
    return 0;
}
