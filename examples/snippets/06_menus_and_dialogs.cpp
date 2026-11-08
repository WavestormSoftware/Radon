// Copyright (c) 2026 Wavestorm Software
// A menu bar, a popup and a modal confirmation dialog.
#include <SDL3/SDL_main.h>

#include <cstdio>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect win{30, 30, 520, 320};
    static char log[96] = "Pick a menu item";
    static bool show_grid = true, dirty = true;
    demo::AppConfig cfg;
    cfg.title = "Menus and dialogs";
    cfg.width = 600, cfg.height = 400;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        using namespace rd;
        if (begin_window(ui, "Editor", win)) {
            if (begin_menu_bar(ui)) {
                if (begin_menu(ui, "File")) {
                    if (menu_item(ui, "New", "Ctrl+N")) std::snprintf(log, sizeof log, "File > New");
                    if (menu_item(ui, "Save", "Ctrl+S")) {
                        dirty = false;
                        std::snprintf(log, sizeof log, "Saved");
                    }
                    if (menu_item(ui, "Quit"))
                        open_modal(ui, "quit");  // open from anywhere in the same ID scope
                    end_menu(ui);
                }
                if (begin_menu(ui, "View")) {
                    if (menu_item(ui, "Grid", nullptr, show_grid)) show_grid = !show_grid;  // checkmark
                    end_menu(ui);
                }
                end_menu_bar(ui);
            }
            layout_row(ui, 1, nullptr, 0, 0);
            label_colored(ui, log, ui.theme().text_muted);
            layout_row(ui, 2, nullptr, 0, 0);
            if (button(ui, "Options").clicked) open_popup(ui, "options");
            if (button(ui, "Quit...", ButtonStyle::Danger).clicked) open_modal(ui, "quit");
            if (popup_begin(ui, "options")) {  // anchored under the last widget
                layout_row(ui, 1, nullptr, 0, 0);
                checkbox(ui, "Show grid", show_grid);
                checkbox(ui, "Unsaved changes", dirty);
                if (button(ui, "Done").clicked) close_popup(ui);
                popup_end(ui);
            }
            if (modal_begin(ui, "quit", "Quit?", 320)) {  // dims and blocks everything behind it
                layout_row(ui, 1, nullptr, 0, 0);
                label_wrapped(ui, dirty ? "You have unsaved changes. Quit anyway?" : "Quit the editor?");
                layout_row(ui, 2, nullptr, 0, 0);
                if (button(ui, "Cancel").clicked) close_modal(ui);
                if (button(ui, "Quit", ButtonStyle::Danger).clicked) {
                    close_modal(ui);
                    std::snprintf(log, sizeof log, "(would quit)");
                }
                modal_end(ui);
            }
            end_window(ui);
        }
    };
    return demo::run(argc, argv, cfg);
}
