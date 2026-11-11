// Copyright (c) 2026 Wavestorm Software
#include <radon/radon.hpp>

int main() {
    rd::Context ui;
    rd::Input in;
    ui.begin_frame(in, 1.0f / 60);
    rd::Rect r{0, 0, 100, 100};
    if (rd::begin_window(ui, "w", r)) {
        rd::button(ui, "ok");
        rd::end_window(ui);
    }
    ui.end_frame();
    return ui.error() ? 1 : 0;
}
