// Copyright (c) 2026 Wavestorm Software
// A to-do list: Enter in the text box adds a task, checkboxes complete it, the trash icon removes it.
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "runner.hpp"

struct Task {
    std::string text;
    bool done = false;
};

int main(int argc, char** argv) {
    static rd::Rect win{30, 30, 400, 440};
    static std::vector<Task> tasks{
        {"Read the Radon docs", true}, {"Build the gallery", false}, {"Ship something", false}};
    static char input[64] = "";
    static rd::TextBuffer input_buf{input, sizeof input, 0};
    static int filter = 0;
    static const char* const tabs[] = {"All", "Active", "Done"};

    demo::AppConfig cfg;
    cfg.title = "To-do";
    cfg.width = 480, cfg.height = 520;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        using namespace rd;
        if (!begin_window(ui, "Tasks", win, WindowDefault | WindowScrollable)) return;
        const float add_cols[] = {-80, 70};
        layout_row(ui, 2, add_cols, 2, 0);
        Response r = textbox(ui, "##new", input_buf, "What needs doing?");
        bool add = button(ui, "Add", ButtonStyle::Primary).clicked || r.clicked;  // Enter submits
        if (add && input_buf.length > 0) {
            tasks.push_back({std::string(input, (size_t)input_buf.length), false});
            input_buf.length = 0;
            input[0] = 0;
        }
        layout_row(ui, 1, nullptr, 0, 0);
        tab_bar(ui, "##filter", filter, tabs, 3);

        int remaining = 0, remove = -1;
        for (auto& t : tasks) remaining += t.done ? 0 : 1;
        for (size_t i = 0; i < tasks.size(); i++) {
            Task& t = tasks[i];
            if ((filter == 1 && t.done) || (filter == 2 && !t.done)) continue;
            push_id(ui, std::to_string(i));  // same label, different row: scope the ID
            const float cols[] = {-34, 30};
            layout_row(ui, 2, cols, 2, 0);
            checkbox(ui, t.text, t.done);
            if (icon_button(ui, "del", Icon::Trash).clicked) remove = (int)i;
            pop_id(ui);
        }
        if (remove >= 0) tasks.erase(tasks.begin() + remove);

        layout_row(ui, 1, nullptr, 0, 0);
        char b[48];
        std::snprintf(b, sizeof b, "%d task%s left", remaining, remaining == 1 ? "" : "s");
        label_colored(ui, b, ui.theme().text_muted);
        end_window(ui);
    };
    return demo::run(argc, argv, cfg);
}
