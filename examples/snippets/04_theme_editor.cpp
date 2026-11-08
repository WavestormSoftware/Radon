// Copyright (c) 2026 Wavestorm Software
// Edit the theme live. Everything in Radon reads colors and metrics from rd::Theme.
#include <SDL3/SDL_main.h>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect editor{20, 20, 380, 640}, preview{420, 20, 340, 360};
    static rd::Theme theme;
    static demo::Fonts fonts;
    static int preset = 0;
    static float f = 0.4f;
    static bool on = true, chk = true;
    static int radio = 0;
    static const char* const presets[] = {"Wavestorm dark", "Light", "Midnight"};

    demo::AppConfig cfg;
    cfg.title = "Theme editor";
    cfg.width = 800, cfg.height = 700;
    cfg.init = [](rd::Context& ui, const demo::Fonts& fs, const demo::Options&) {
        fonts = fs;
        theme = ui.theme();
    };
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        using namespace rd;
        bool changed = false;
        const float cols[] = {120, -1};
        if (begin_window(ui, "Theme", editor, WindowDefault | WindowScrollable)) {
            layout_row(ui, 2, cols, 2, 0);
            label(ui, "Preset");
            if (combo(ui, "##preset", preset, presets, 3)) {
                theme = preset == 0 ? Theme::dark() : (preset == 1 ? Theme::light() : Theme::midnight());
                theme.font_body = fonts.body, theme.font_heading = fonts.heading,
                theme.font_small = fonts.small;
                changed = true;
            }
            separator_text(ui, "Accent");
            Color accent = theme.accent;
            if (color_picker(ui, "##accent", accent, false)) {
                theme = Theme::with_accent(theme, accent);  // updates accent, focus ring and selection
                changed = true;
            }
            separator_text(ui, "Metrics");
            layout_row(ui, 2, cols, 2, 0);
            label(ui, "Corner radius");
            changed |= slider(ui, "##cr", theme.corner_radius, 0, 14, "%.0f").changed;
            label(ui, "Window radius");
            changed |= slider(ui, "##wr", theme.window_radius, 0, 20, "%.0f").changed;
            label(ui, "Spacing");
            changed |= slider(ui, "##sp", theme.spacing, 0, 16, "%.0f").changed;
            label(ui, "Padding");
            changed |= slider(ui, "##pd", theme.padding, 2, 20, "%.0f").changed;
            label(ui, "Row height");
            changed |= slider(ui, "##rh", theme.cell_height, 20, 44, "%.0f").changed;
            label(ui, "Shadows");
            changed |= toggle(ui, "##shadow", theme.shadows).changed;
            end_window(ui);
        }
        if (begin_window(ui, "Preview", preview)) {
            layout_row(ui, 3, nullptr, 0, 0);
            button(ui, "Default");
            button(ui, "Primary", ButtonStyle::Primary);
            button(ui, "Danger", ButtonStyle::Danger);
            layout_row(ui, 2, nullptr, 0, 0);
            checkbox(ui, "Checkbox", chk);
            toggle(ui, "Switch", on);
            layout_row(ui, 3, nullptr, 0, 0);
            radio_button(ui, "A", radio, 0);
            radio_button(ui, "B", radio, 1);
            radio_button(ui, "C", radio, 2);
            layout_row(ui, 1, nullptr, 0, 0);
            slider(ui, "##f", f, 0, 1);
            progress_bar(ui, f);
            end_window(ui);
        }
        if (changed) ui.set_theme(theme);  // takes effect for the next widget call
    };
    return demo::run(argc, argv, cfg);
}
