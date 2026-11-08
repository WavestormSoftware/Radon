// Copyright (c) 2026 Wavestorm Software
// A calculator: a grid of buttons built with layout_row and equal columns.
#include <SDL3/SDL_main.h>

#include <cstdio>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect win{30, 30, 300, 400};
    static double acc = 0, entry = 0;
    static char op = 0;
    static bool fresh = true;
    demo::AppConfig cfg;
    cfg.title = "Calculator";
    cfg.width = 360, cfg.height = 470;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        using namespace rd;
        if (!begin_window(ui, "Calculator", win, WindowTitleBar | WindowBorder | WindowMovable)) return;
        char shown[32];
        std::snprintf(shown, sizeof shown, "%g", entry);
        layout_row(ui, 1, nullptr, 0, 44);
        label_colored(ui, shown, ui.theme().accent);
        static const char* const keys[] = {"7", "8", "9", "/", "4", "5", "6", "*",
                                           "1", "2", "3", "-", "C", "0", "=", "+"};
        for (int r = 0; r < 4; r++) {
            layout_row(ui, 4, nullptr, 0, 48);
            for (int c = 0; c < 4; c++) {
                const char* k = keys[r * 4 + c];
                const bool is_op = k[0] == '/' || k[0] == '*' || k[0] == '-' || k[0] == '+' || k[0] == '=';
                if (!button(ui, k, is_op ? ButtonStyle::Primary : ButtonStyle::Default).clicked) continue;
                if (k[0] >= '0' && k[0] <= '9') {
                    entry = fresh ? k[0] - '0' : entry * 10 + (k[0] - '0');
                    fresh = false;
                } else if (k[0] == 'C') {
                    acc = entry = 0, op = 0, fresh = true;
                } else {
                    if (op == '+')
                        entry = acc + entry;
                    else if (op == '-')
                        entry = acc - entry;
                    else if (op == '*')
                        entry = acc * entry;
                    else if (op == '/' && entry != 0)
                        entry = acc / entry;
                    acc = entry;
                    op = k[0] == '=' ? 0 : k[0];
                    fresh = true;
                }
            }
        }
        end_window(ui);
    };
    return demo::run(argc, argv, cfg);
}
