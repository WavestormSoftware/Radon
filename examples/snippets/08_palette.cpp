// Copyright (c) 2026 Wavestorm Software
// A palette builder: pick a color with the picker, click a swatch to edit it.
#include <SDL3/SDL_main.h>

#include <cstdio>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect win{20, 20, 380, 520};
    static rd::Color palette[8] = {{236, 72, 153, 255}, {77, 163, 255, 255},  {61, 214, 140, 255},
                                   {245, 181, 74, 255}, {160, 120, 255, 255}, {255, 93, 108, 255},
                                   {20, 24, 30, 255},   {223, 228, 234, 255}};
    static int selected = 0;
    demo::AppConfig cfg;
    cfg.title = "Palette";
    cfg.width = 440, cfg.height = 580;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        using namespace rd;
        if (!begin_window(ui, "Palette", win, WindowDefault | WindowScrollable)) return;
        separator_text(ui, "Swatches");
        const float w[] = {36};
        layout_row(ui, 8, w, 1, 0);
        for (int i = 0; i < 8; i++) {
            char id[8];
            std::snprintf(id, sizeof id, "sw%d", i);
            if (color_button(ui, id, palette[i]).clicked) selected = i;
        }
        char b[32];
        std::snprintf(b, sizeof b, "Editing swatch %d", selected + 1);
        separator_text(ui, b);
        color_picker(ui, "##picker", palette[selected], true);  // edits the color in place
        end_window(ui);
    };
    return demo::run(argc, argv, cfg);
}
