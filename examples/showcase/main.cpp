// Copyright (c) 2026 Wavestorm Software
// Four small products built with Radon, each with its own theme and typefaces:
//   --page 0  Tilekit, a 2D level editor (docking, Canvas, tree, inspector)
//   --page 1  Halcyon, a synthesizer plugin (knobs, Canvas scope and keyboard)
//   --page 2  Stride, a running app: this week   (phone size: --size 390x844)
//   --page 3  Stride, a running app: training plan
//   --page 4  Live ops, a game-server dashboard  (--light or --dark)
#include <SDL3/SDL_main.h>

#include "runner.hpp"
#include "showcase.hpp"

int main(int argc, char** argv) {
    static const showcase::App* const apps[] = {&showcase::tilekit, &showcase::halcyon,
                                                &showcase::stride_today, &showcase::stride_plan,
                                                &showcase::liveops};
    static const showcase::App* app = apps[0];
    demo::AppConfig cfg;
    cfg.title = "Radon showcase";
    cfg.width = 1440;
    cfg.height = 900;
    cfg.config.max_fonts = 24;
    cfg.config.atlas_size = 2048;
    cfg.config.max_vertices = 262144;
    cfg.config.max_indices = 786432;
    cfg.config.max_draw_commands = 8192;
    cfg.init = [](rd::Context& ui, const demo::Fonts& f, const demo::Options& o) {
        app = apps[o.page >= 0 && o.page < 5 ? o.page : 0];
        ui.set_theme(app->setup(ui, f.body, !o.light));
    };
    cfg.frame = [](rd::Context& ui, const rd::Input& in, int w, int h) { app->frame(ui, in, w, h); };
    cfg.system_theme = [](rd::Context&, bool) {};  // each app owns its look
    return demo::run(argc, argv, cfg);
}
