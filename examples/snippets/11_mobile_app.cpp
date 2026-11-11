// Copyright (c) 2026 Wavestorm Software
// A phone-style app: touch theme, safe area, bottom navigation, a bottom sheet, toasts and a long list.
// Run it at phone size:  radon_ex_11_mobile_app --size 400x800
#include <SDL3/SDL_main.h>

#include <cstdio>

#include "runner.hpp"

int main(int argc, char** argv) {
    using namespace rd;
    demo::AppConfig cfg;
    cfg.title = "Radon: mobile app";
    cfg.width = 400;
    cfg.height = 800;
    cfg.init = [](Context& ui, const demo::Fonts& f, const demo::Options& o) {
        ui.set_theme(Theme::touch(demo::make_theme(f, !o.light)));  // 44 px rows for fingers
    };
    cfg.frame = [](Context& ui, const Input& in, int, int) {
        static int tab = 0, selected = -1;
        static bool wifi = true, sync = false, open_sheet = false;
        static float volume = 0.6f;
        const Rect safe = in.safe_rect();  // the screen minus notches and system bars
        const float bar = 64;
        set_toast_offset(ui, bar);

        Rect body{safe.x, safe.y, safe.w, safe.h - bar};
        if (begin_window(ui, "body", body, WindowScrollable)) {
            transition_begin(ui, "tab", tab);
            layout_row(ui, 1, nullptr, 0, 0);
            if (tab == 0) {
                heading(ui, "Inbox");
                int first = 0, last = 0;
                list_clipper_begin(ui, 2000, 0, first, last);  // only the visible rows are built
                for (int i = first; i < last; i++) {
                    char row[32];
                    std::snprintf(row, sizeof row, "Message %d", i + 1);
                    layout_row(ui, 1, nullptr, 0, 0);
                    if (list_item(ui, row, selected == i, Icon::User).clicked) {
                        selected = i;
                        open_sheet = true;
                    }
                }
                list_clipper_end(ui);
            } else if (tab == 1) {
                heading(ui, "Settings");
                toggle(ui, "Wi-Fi", wifi);
                toggle(ui, "Background sync", sync);
                slider(ui, "##volume", volume, 0, 1, "Volume %.2f");
                if (button(ui, "Save", ButtonStyle::Primary).clicked)
                    toast(ui, "Settings saved", Tone::Success);
            } else {
                heading(ui, "About");
                label_wrapped(ui,
                              "Flick to scroll, long-press for context menus, and use the system back "
                              "button or a tap outside to close sheets.");
            }
            transition_end(ui);
            end_window(ui);
        }

        Rect nav{0, safe.y + safe.h - bar, in.screen_size.x, bar + in.safe_area.h};
        if (begin_window(ui, "nav", nav, WindowNone)) {
            static const Icon icons[] = {Icon::Home, Icon::Settings, Icon::Info};
            static const char* const names[] = {"Inbox", "Settings", "About"};
            layout_row(ui, 1, nullptr, 0, bar - 2 * ui.theme().padding);
            nav_bar(ui, "##nav", tab, icons, names, 3);
            end_window(ui);
        }

        if (open_sheet) open_modal(ui, "actions");
        open_sheet = false;
        if (drawer_begin(ui, "actions", Side::Bottom, 230)) {
            layout_row(ui, 1, nullptr, 0, 0);
            if (list_item(ui, "Reply", false, Icon::Pencil).clicked) close_modal(ui);
            if (list_item(ui, "Share", false, Icon::Share).clicked) close_modal(ui);
            if (list_item(ui, "Delete", false, Icon::Trash).clicked) {
                toast(ui, "Message deleted", Tone::Error);
                close_modal(ui);
            }
            drawer_end(ui);
        }
    };
    return demo::run(argc, argv, cfg);
}
