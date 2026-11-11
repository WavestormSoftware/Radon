// Copyright (c) 2026 Wavestorm Software
// A phone-style app: touch theme, safe area, bottom navigation, pull to refresh, swipeable rows, an
// edge-swipe drawer, a bottom sheet you can swipe away, wheel and time pickers, and toasts. Run it at phone
// size:  radon_ex_11_mobile_app --size 400x800
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
        static int tab = 0, selected = -1, deleted = 0, size = 2, hour = 7, minute = 30;
        static bool wifi = true, sync = false, open_sheet = false, refreshing = false;
        static float volume = 0.6f, loading = 0;
        const Rect safe = in.safe_rect();  // the screen minus notches and system bars
        const float bar = 64;
        set_toast_offset(ui, bar);

        Rect body{safe.x, safe.y, safe.w, safe.h - bar};
        if (begin_window(ui, "body", body, WindowScrollable)) {
            if (tab == 0 && pull_to_refresh(ui, "refresh", refreshing)) loading = 1.0f;
            if (refreshing && (loading -= 1.0f / 60) <= 0) {  // pretend to fetch mail
                refreshing = false;
                toast(ui, "Inbox is up to date", Tone::Success);
            }
            transition_begin(ui, "tab", tab);
            layout_row(ui, 1, nullptr, 0, 0);
            if (tab == 0) {
                heading(ui, "Inbox");
                const SwipeAction actions[] = {{"Delete", Icon::Trash, ui.theme().error}};
                int first = 0, last = 0;
                list_clipper_begin(ui, 2000, 0, first, last);  // only the visible rows are built
                for (int i = first; i < last; i++) {
                    char row[32];
                    std::snprintf(row, sizeof row, "Message %d", i + 1 + deleted);
                    layout_row(ui, 1, nullptr, 0, 0);
                    int act = -1;
                    if (swipe_item(ui, row, actions, 1, act, selected == i)
                            .clicked) {  // swipe left to delete
                        selected = i;
                        open_sheet = true;
                    }
                    if (act == 0) {
                        deleted++;
                        toast(ui, "Message deleted", Tone::Error);
                    }
                }
                list_clipper_end(ui);
            } else if (tab == 1) {
                heading(ui, "Settings");
                toggle(ui, "Wi-Fi", wifi);
                toggle(ui, "Background sync", sync);
                slider(ui, "##volume", volume, 0, 1, "Volume %.2f");
                label(ui, "Wake up at");
                time_picker(ui, "##alarm", hour, minute, false);
                static const char* const sizes[] = {"Small", "Medium", "Large", "Huge"};
                label(ui, "Text size");
                wheel_picker(ui, "##size", size, sizes, 4, 3);
                if (button(ui, "Save", ButtonStyle::Primary).clicked)
                    toast(ui, "Settings saved", Tone::Success);
            } else {
                heading(ui, "About");
                markdown(ui,
                         "Flick to scroll and **pull down** to refresh the inbox. Swipe a message *left* to "
                         "delete it, swipe in from the left edge for the menu, and swipe sheets down to "
                         "close them.");
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

        if (edge_swipe(ui, Side::Left)) open_modal(ui, "menu");
        if (drawer_begin(ui, "menu", Side::Left, 260)) {
            layout_row(ui, 1, nullptr, 0, 0);
            heading(ui, "Mail");
            static const char* const names[] = {"Inbox", "Settings", "About"};
            static const Icon icons[] = {Icon::Mail, Icon::Settings, Icon::Info};
            for (int i = 0; i < 3; i++)
                if (list_item(ui, names[i], tab == i, icons[i]).clicked) {
                    tab = i;
                    close_modal(ui);
                }
            drawer_end(ui);
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
