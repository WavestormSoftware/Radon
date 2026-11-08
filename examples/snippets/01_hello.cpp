// Copyright (c) 2026 Wavestorm Software
// The smallest Radon app: a window, a label and a button.
#include <SDL3/SDL_main.h>

#include <cstdio>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect win{40, 40, 320, 150};
    static int clicks = 0;
    demo::AppConfig cfg;
    cfg.title = "Hello Radon";
    cfg.width = 480, cfg.height = 280;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        if (rd::begin_window(ui, "Hello", win)) {
            rd::layout_row(ui, 1, nullptr, 0, 0);  // one full-width cell per row
            char text[64];
            std::snprintf(text, sizeof text, "You clicked %d times", clicks);
            rd::label(ui, text);
            if (rd::button(ui, "Click me", rd::ButtonStyle::Primary).clicked) clicks++;
            rd::end_window(ui);
        }
    };
    return demo::run(argc, argv, cfg);
}
