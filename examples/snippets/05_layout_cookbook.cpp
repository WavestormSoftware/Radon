// Copyright (c) 2026 Wavestorm Software
// Layout recipes: fixed and fill widths, equal columns, nested columns, tall rows and child regions.
#include <SDL3/SDL_main.h>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect win{20, 20, 520, 640};
    demo::AppConfig cfg;
    cfg.title = "Layout cookbook";
    cfg.width = 580, cfg.height = 700;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        using namespace rd;
        if (!begin_window(ui, "Layout", win, WindowDefault | WindowScrollable)) return;

        separator_text(ui, "label + fill");
        const float a[] = {100, -1};  // positive = pixels, negative = fill (-1 fills exactly)
        layout_row(ui, 2, a, 2, 0);
        button(ui, "100 px");
        button(ui, "fill");

        separator_text(ui, "fill + fixed");
        const float b[] = {-100, 100};
        layout_row(ui, 2, b, 2, 0);
        button(ui, "fill");
        button(ui, "100 px");

        separator_text(ui, "equal columns (null widths)");
        layout_row(ui, 4, nullptr, 0, 0);
        for (const char* t : {"A", "B", "C", "D"}) button(ui, t);

        separator_text(ui, "custom row height");
        layout_row(ui, 2, nullptr, 0, 56);
        button(ui, "56 px tall");
        button(ui, "also 56");

        separator_text(ui, "nested columns");
        const float halves[] = {-1, -1};
        layout_row(ui, 2, halves, 2, 0);
        layout_begin_column(ui);
        layout_row(ui, 1, nullptr, 0, 0);
        button(ui, "left 1");
        button(ui, "left 2");
        button(ui, "left 3");
        layout_end_column(ui);
        layout_begin_column(ui);
        layout_row(ui, 1, nullptr, 0, 0);
        button(ui, "right 1");
        button(ui, "right 2");
        layout_end_column(ui);

        separator_text(ui, "scrolling child region");
        if (begin_child(ui, "child", 110)) {  // takes a full row
            for (int i = 1; i <= 12; i++) {
                layout_row(ui, 1, nullptr, 0, 0);
                char t[32];
                std::snprintf(t, sizeof t, "row %d", i);
                label(ui, t);
            }
            end_child(ui);
        }

        separator_text(ui, "indent");
        layout_row(ui, 1, nullptr, 0, 0);
        label(ui, "no indent");
        indent(ui);
        layout_row(ui, 1, nullptr, 0, 0);
        label(ui, "indented once");
        indent(ui);
        layout_row(ui, 1, nullptr, 0, 0);
        label(ui, "indented twice");
        unindent(ui);
        unindent(ui);
        end_window(ui);
    };
    return demo::run(argc, argv, cfg);
}
