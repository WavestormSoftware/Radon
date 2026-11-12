// Copyright (c) 2026 Wavestorm Software
// A borderless window with Radon's own titlebar: rounded corners, a search box in the bar, native drag and resize.
#include <SDL3/SDL_main.h>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::TextBuffer search;
    static int clicks = 0;
    demo::AppConfig cfg;
    cfg.title = "Frameless";
    cfg.width = 720, cfg.height = 440;
    cfg.custom_titlebar = true;
    cfg.transparent = true;
    cfg.frame = [](rd::Context& ui, const rd::Input& in, int w, int h) {
        rd::window_background(ui);
        if (rd::titlebar_begin(ui, "Frameless")) {
            const float widths[] = {64, 64, 220};
            rd::layout_row(ui, 3, widths, 3, 0);
            rd::button(ui, "File", rd::ButtonStyle::Ghost);
            rd::button(ui, "Edit", rd::ButtonStyle::Ghost);
            rd::search_box(ui, "##search", search);
            rd::titlebar_end(ui);
        }
        const float top = rd::titlebar_height(ui);
        rd::Rect body{0, top, (float)w, (float)h - top};
        if (rd::begin_window(ui, "body", body, rd::WindowTransparent)) {
            rd::layout_row(ui, 1, nullptr, 0, 0);
            rd::label(ui, in.window_maximized ? "Maximized" : "Drag the bar, resize from any edge");
            if (rd::button(ui, "Click me", rd::ButtonStyle::Primary).clicked) clicks++;
            rd::progress_bar(ui, (float)(clicks % 11) / 10.0f);
            rd::end_window(ui);
        }
    };
    return demo::run(argc, argv, cfg);
}
