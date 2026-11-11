// Copyright (c) 2026 Wavestorm Software
// Web gallery: the same UI as the desktop gallery, driven by emscripten_set_main_loop.
#include <SDL3/SDL_main.h>
#include <emscripten.h>

#include "gallery.hpp"

namespace {
demo::Gallery gallery;
demo::Runner runner;
void loop() {
    if (!runner.step()) emscripten_cancel_main_loop();
}
}  // namespace

int main(int argc, char** argv) {
    demo::AppConfig cfg;
    cfg.title = "Radon gallery";
    cfg.init = [](rd::Context&, const demo::Fonts& f, const demo::Options& o) { gallery.setup(f, o.page); };
    cfg.frame = [](rd::Context& ctx, const rd::Input& in, int w, int h) { gallery.frame(ctx, in, w, h); };
    cfg.system_theme = [](rd::Context& ctx, bool dark) { gallery.set_dark(ctx, dark); };
    if (!runner.start(argc, argv, cfg)) return 1;
    gallery.set_runner(&runner);
    auto art = demo::art_rgba(512, 256);
    gallery.set_checker(runner.create_texture(512, 256, art.data()));
    emscripten_set_main_loop(loop, 0, 1);
    return 0;
}
