// Copyright (c) 2026 Wavestorm Software
// A settings form: text, combo, radios, switches, sliders and Save / Reset buttons.
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <cstring>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect win{30, 30, 420, 460};
    static char user[32] = "player_one", status[64] = "";
    static rd::TextBuffer user_buf{user, sizeof user, 10};
    static int quality = 1, language = 0, volume = 70;
    static bool vsync = true, fullscreen = false, subtitles = true;
    static float brightness = 0.6f;
    static const char* const langs[] = {"English", "Nederlands", "Deutsch", "Francais"};

    demo::AppConfig cfg;
    cfg.title = "Settings form";
    cfg.width = 520, cfg.height = 540;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        using namespace rd;
        if (!begin_window(ui, "Settings", win, WindowDefault | WindowScrollable)) return;
        const float cols[] = {110, -1};  // label column + fill column

        separator_text(ui, "Account");
        layout_row(ui, 2, cols, 2, 0);
        label(ui, "Username");
        textbox(ui, "##user", user_buf, "Choose a name");
        label(ui, "Language");
        combo(ui, "##lang", language, langs, 4);

        separator_text(ui, "Graphics");
        layout_row(ui, 3, nullptr, 0, 0);
        radio_button(ui, "Low", quality, 0);
        radio_button(ui, "Medium", quality, 1);
        radio_button(ui, "High", quality, 2);
        layout_row(ui, 2, nullptr, 0, 0);
        toggle(ui, "VSync", vsync);
        toggle(ui, "Fullscreen", fullscreen);
        layout_row(ui, 2, cols, 2, 0);
        label(ui, "Brightness");
        slider(ui, "##bright", brightness, 0, 1);

        separator_text(ui, "Audio");
        layout_row(ui, 2, cols, 2, 0);
        label(ui, "Volume");
        slider_int(ui, "##vol", volume, 0, 100);
        layout_row(ui, 1, nullptr, 0, 0);
        checkbox(ui, "Show subtitles", subtitles);

        spacing(ui, 10);
        layout_row(ui, 2, nullptr, 0, 0);
        if (button(ui, "Save", ButtonStyle::Primary).clicked)
            std::snprintf(status, sizeof status, "Saved settings for %s", user);
        if (button(ui, "Reset").clicked) {
            quality = 1, volume = 70, brightness = 0.6f, vsync = true, fullscreen = false;
            std::strcpy(status, "Reset to defaults");
        }
        layout_row(ui, 1, nullptr, 0, 0);
        label_colored(ui, status, ui.theme().success);
        end_window(ui);
    };
    return demo::run(argc, argv, cfg);
}
