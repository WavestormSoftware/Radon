// Copyright (c) 2026 Wavestorm Software
// Frosted glass: a gradient scene, a blurred translucent window over it, and a spring-animated card.
#include <SDL3/SDL_main.h>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect win{70, 60, 320, 200};
    static bool wide = false;
    demo::AppConfig cfg;
    cfg.title = "Glass";
    cfg.width = 720, cfg.height = 440;
    cfg.init = [](rd::Context& ui, const demo::Fonts& fonts, const demo::Options&) {
        rd::Theme t = rd::Theme::glass(demo::make_theme(fonts, true));
        ui.set_theme(t);
    };
    cfg.frame = [](rd::Context& ui, const rd::Input&, int w, int h) {
        rd::Style bg;
        bg.fill = rd::Paint::linear({0x2a, 0x10, 0x5e, 255}, {0x0b, 0x6e, 0x8a, 255}, 35);
        rd::box(ui, {0, 0, (float)w, (float)h}, bg);
        const rd::Color blobs[] = {{0xff, 0x5d, 0x9e, 255}, {0xff, 0xb5, 0x4a, 255}, {0x3d, 0xd6, 0x8c, 255}};
        for (int i = 0; i < 3; i++) {
            rd::Style b;
            b.fill = rd::Paint::radial(blobs[i], {blobs[i].r, blobs[i].g, blobs[i].b, 0});
            rd::box(ui, {40.0f + 190 * (float)i, 90.0f + 60 * (float)(i % 2), 220, 220}, b);
        }
        if (rd::begin_window(ui, "Glass", win, rd::WindowDefault)) {
            rd::layout_row(ui, 1, nullptr, 0, 0);
            rd::label(ui, "Everything behind this window is blurred.");
            if (rd::button(ui, "Primary", rd::ButtonStyle::Primary).clicked) wide = !wide;
            rd::end_window(ui);
        }
        // A spring moves the card: click the button to toggle it.
        const float x = rd::spring(ui, "card-x", wide ? 440.0f : 400.0f);
        rd::Style card;
        card.fill = rd::Color{255, 255, 255, 40};
        card.border = 1;
        card.border_paint = rd::Color{255, 255, 255, 90};
        card.radius = 20;
        card.backdrop_blur = 16;
        rd::box(ui, {x, 120, 240, 150}, card);
    };
    return demo::run(argc, argv, cfg);
}
