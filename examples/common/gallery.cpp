// Copyright (c) 2026 Wavestorm Software
#include "gallery.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

namespace demo {
using namespace rd;

namespace {

const char* const kPages[] = {
    "Widgets",           "Inputs", "Layout",  "Lists and menus", "Theming",  "Dashboard", "Data",
    "Motion and native", "Charts", "Pickers", "Rich text",       "Gestures", "Docking"};
constexpr int kPageCount = 13, kDashboard = 5, kDocking = 12;
const Icon kPageIcons[] = {Icon::Star, Icon::Pencil,   Icon::Menu, Icon::Folder, Icon::Settings,
                           Icon::Eye,  Icon::Calendar, Icon::Play, Icon::Grid,   Icon::Clock,
                           Icon::Code, Icon::Phone,    Icon::List};
bool desktop_only(int page) {
    return page == kDashboard || page == kDocking;
}

bool s_eye(Context& ctx, int i, bool visible) {
    char id[8];
    std::snprintf(id, sizeof id, "eye%d", i);
    return icon_button(ctx, id, visible ? Icon::Eye : Icon::EyeOff).clicked;
}
bool s_lock(Context& ctx, int i, bool locked) {
    char id[8];
    std::snprintf(id, sizeof id, "lk%d", i);
    return icon_button(ctx, id, locked ? Icon::Lock : Icon::Unlock).clicked;
}

void row2(Context& ctx, const char* label_text) {
    static const float cols[] = {110, -1};
    layout_row(ctx, 2, cols, 2, 0);
    label(ctx, label_text);
}

}  // namespace

void Gallery::apply_theme(Context& ctx) {
    theme_.font_body = fonts_.body;
    theme_.font_heading = fonts_.heading;
    theme_.font_small = fonts_.small;
    theme_.font_large = fonts_.large;
    ctx.set_theme(touch_ ? Theme::touch(theme_) : theme_);
}

void Gallery::panel_buttons(Context& ctx) {
    separator_text(ctx, "Buttons");
    layout_row(ctx, 4, nullptr, 0, 0);
    if (button(ctx, "Default").clicked) clicks_++;
    if (button(ctx, "Primary", ButtonStyle::Primary).clicked) clicks_++;
    button(ctx, "Ghost", ButtonStyle::Ghost);
    button(ctx, "Danger", ButtonStyle::Danger);
    tooltip(ctx, "Danger buttons use the theme error color");
    layout_row(ctx, 1, nullptr, 0, 0);
    char b[48];
    std::snprintf(b, sizeof b, "Clicked %d times", clicks_);
    label_colored(ctx, b, ctx.theme().text_muted);
    separator_text(ctx, "Icon buttons");
    static const float w[] = {34};
    layout_row(ctx, 8, w, 1, 0);
    const Icon icons[] = {Icon::Plus,   Icon::Minus,  Icon::Search, Icon::Trash,
                          Icon::Folder, Icon::Pencil, Icon::Undo,   Icon::Redo};
    const char* ids[] = {"plus", "minus", "search", "trash", "folder", "pencil", "undo", "redo"};
    for (int i = 0; i < 8; i++) icon_button(ctx, ids[i], icons[i]);
}

void Gallery::panel_toggles(Context& ctx) {
    separator_text(ctx, "Checkboxes, radios and switches");
    layout_row(ctx, 2, nullptr, 0, 0);
    checkbox(ctx, "Enabled", check_a_);
    checkbox(ctx, "Notifications", check_b_);
    layout_row(ctx, 3, nullptr, 0, 0);
    radio_button(ctx, "Small", radio_, 0);
    radio_button(ctx, "Medium", radio_, 1);
    radio_button(ctx, "Large", radio_, 2);
    layout_row(ctx, 2, nullptr, 0, 0);
    toggle(ctx, "Smoothing", toggle_a_);
    toggle(ctx, "Snap to grid", toggle_b_);
}

void Gallery::panel_sliders(Context& ctx) {
    separator_text(ctx, "Numbers");
    row2(ctx, "Slider");
    slider(ctx, "##sf", slider_f_, 0, 1);
    row2(ctx, "Int slider");
    slider_int(ctx, "##si", slider_i_, 0, 100);
    row2(ctx, "Drag float");
    drag_float(ctx, "##df", drag_f_, 0.05f);
    row2(ctx, "Drag int");
    drag_int(ctx, "##di", drag_i_);
    row2(ctx, "Stepper");
    stepper(ctx, "##stf", step_f_, 0.25f, 0, 5);
    row2(ctx, "Int stepper");
    stepper_int(ctx, "##sti", step_i_, 1, 0, 10);
}

void Gallery::panel_feedback(Context& ctx) {
    separator_text(ctx, "Feedback");
    row2(ctx, "Progress");
    progress_bar(ctx, slider_f_);
    row2(ctx, "Spinner");
    spinner(ctx);
    row2(ctx, "Badges");
    layout_row(ctx, 4, nullptr, 0, 0);
    label(ctx, "");
    badge(ctx, "new", ctx.theme().accent);
    badge(ctx, "ok", ctx.theme().success);
    badge(ctx, "warn", ctx.theme().warning);
    separator_text(ctx, "Plots");
    layout_row(ctx, 1, nullptr, 0, 0);
    // a rolling sine wave, advanced from the frame clock
    plot_clock_ += 1.0f / 60;
    if (plot_clock_ > 0.05f) {
        plot_clock_ = 0;
        plot_[plot_head_] =
            std::sin((float)plot_head_ * 0.35f + (float)ctx.atlas_version() * 0.0f) * 0.5f + 0.5f;
        plot_head_ = (plot_head_ + 1) % 48;
        for (int i = 0; i < 48; i++) plot_[i] = 0.5f + 0.4f * std::sin((float)(i + plot_head_) * 0.3f);
    }
    plot_lines(ctx, plot_, 48, 0, 1, 56);
    plot_histogram(ctx, plot_, 24, 0, 1, 40);
}

void Gallery::panel_text(Context& ctx) {
    separator_text(ctx, "Text input");
    row2(ctx, "Name");
    textbox(ctx, "##name", name_buf_);
    row2(ctx, "Search");
    textbox(ctx, "##search", search_buf_, "Type to search...");
    row2(ctx, "Email");
    textbox(ctx, "##email", email_buf_, "name@example.com");
    layout_row(ctx, 1, nullptr, 0, 0);
    label_colored(ctx, "Click or Tab into a field. Ctrl+A/C/X/V, Shift+arrows and Home/End work.",
                  ctx.theme().text_muted);
    row2(ctx, "Combo");
    static const char* const items[] = {"Small", "Medium", "Large", "Extra large"};
    combo(ctx, "##combo", combo_, items, 4);
}

void Gallery::panel_colors(Context& ctx) {
    separator_text(ctx, "Color");
    color_picker(ctx, "##picker", color_, true);
    separator_text(ctx, "Swatches");
    static const float w[] = {44};
    layout_row(ctx, 6, w, 1, 0);
    const char* ids[] = {"s0", "s1", "s2", "s3", "s4", "s5"};
    for (int i = 0; i < 6; i++)
        if (color_button(ctx, ids[i], swatches_[i]).clicked) color_ = swatches_[i];
}

void Gallery::panel_tabs(Context& ctx) {
    separator_text(ctx, "Tabs");
    static const char* const tabs[] = {"Overview", "Activity", "Settings"};
    layout_row(ctx, 1, nullptr, 0, 0);
    tab_bar(ctx, "##tabs", tab_, tabs, 3);
    layout_row(ctx, 1, nullptr, 0, 0);
    if (tab_ == 0)
        label_wrapped(
            ctx, "Tabs return true when the selection changes. The selected tab gets the accent underline.");
    else if (tab_ == 1) {
        bullet_text(ctx, "Created a document");
        bullet_text(ctx, "Changed the theme");
    } else {
        toggle(ctx, "Dark mode", toggle_a_);
    }
}

void Gallery::panel_rows(Context& ctx) {
    separator_text(ctx, "Rows: fixed and fill widths");
    const float a[] = {120, -1};
    layout_row(ctx, 2, a, 2, 0);
    button(ctx, "120 px");
    button(ctx, "fill");
    const float b[] = {-(101 + ctx.theme().spacing), 100};  // fill, leaving room for the 100 px cell
    layout_row(ctx, 2, b, 2, 0);
    button(ctx, "fill");
    button(ctx, "100 px");
    const float c[] = {60, -1, 60};
    layout_row(ctx, 3, c, 3, 0);
    button(ctx, "60");
    button(ctx, "fill");
    button(ctx, "60");
    layout_row(ctx, 4, nullptr, 0, 0);
    for (const char* t : {"1/4", "1/4 ", "1/4  ", "1/4   "}) button(ctx, t);
    layout_row(ctx, 1, nullptr, 0, 40);
    button(ctx, "Tall row (40 px)");
}

void Gallery::panel_columns(Context& ctx) {
    separator_text(ctx, "Nested columns");
    layout_row(ctx, 2, nullptr, 0, 0);  // two equal columns
    layout_begin_column(ctx);
    layout_row(ctx, 1, nullptr, 0, 0);
    button(ctx, "Left 1");
    button(ctx, "Left 2");
    button(ctx, "Left 3");
    layout_end_column(ctx);
    layout_begin_column(ctx);
    layout_row(ctx, 1, nullptr, 0, 0);
    button(ctx, "Right 1");
    button(ctx, "Right 2");
    layout_end_column(ctx);
    layout_row(ctx, 1, nullptr, 0, 0);
    separator(ctx);
    spacing(ctx, 8);
    layout_row(ctx, 1, nullptr, 0, 0);
    label_wrapped(
        ctx,
        "Wrapped labels grow their row to fit the text. Resize the window to see this paragraph reflow.");
}

void Gallery::panel_regions(Context& ctx) {
    separator_text(ctx, "Scrolling region");
    if (begin_child(ctx, "region", 120)) {
        for (int i = 0; i < 14; i++) {
            char b[32];
            std::snprintf(b, sizeof b, "Line %d in a child region", i + 1);
            layout_row(ctx, 1, nullptr, 0, 0);
            label(ctx, b);
        }
        end_child(ctx);
    }
}

void Gallery::panel_lists(Context& ctx) {
    separator_text(ctx, "List box");
    static const char* const items[] = {"Ink Pen", "Fineliner", "Round Brush", "Pencil HB",
                                        "Marker",  "Airbrush",  "Charcoal",    "Chalk"};
    listbox(ctx, "##lb", list_sel_, items, 8, 150);
    layout_row(ctx, 1, nullptr, 0, 0);
    char b[64];
    std::snprintf(b, sizeof b, "Selected: %s", items[list_sel_]);
    label_colored(ctx, b, ctx.theme().text_muted);
}

void Gallery::panel_tree(Context& ctx) {
    separator_text(ctx, "Tree");
    layout_row(ctx, 1, nullptr, 0, 0);
    if (tree_node(ctx, "Artwork", open_[0])) {
        layout_row(ctx, 1, nullptr, 0, 0);
        if (tree_node(ctx, "Characters", open_[1])) {
            layout_row(ctx, 1, nullptr, 0, 0);
            bullet_text(ctx, "Cat");
            bullet_text(ctx, "Girl");
            tree_pop(ctx);
        }
        layout_row(ctx, 1, nullptr, 0, 0);
        if (tree_node(ctx, "Backgrounds", open_[2])) {
            layout_row(ctx, 1, nullptr, 0, 0);
            bullet_text(ctx, "City");
            bullet_text(ctx, "Sky");
            tree_pop(ctx);
        }
        tree_pop(ctx);
    }
}

void Gallery::panel_menus(Context& ctx) {
    separator_text(ctx, "Menus, popups and dialogs");
    if (begin_menu_bar(ctx)) {
        if (begin_menu(ctx, "File")) {
            if (menu_item(ctx, "New", "Ctrl+N")) std::snprintf(log_, sizeof log_, "File > New");
            if (menu_item(ctx, "Open...", "Ctrl+O")) std::snprintf(log_, sizeof log_, "File > Open");
            if (menu_item(ctx, "Quit")) open_modal(ctx, "confirm");
            end_menu(ctx);
        }
        if (begin_menu(ctx, "View")) {
            if (menu_item(ctx, "Show hints", nullptr, show_hint_)) show_hint_ = !show_hint_;
            if (menu_item(ctx, "Zoom in", "+")) std::snprintf(log_, sizeof log_, "View > Zoom in");
            end_menu(ctx);
        }
        end_menu_bar(ctx);
    }
    layout_row(ctx, 1, nullptr, 0, 0);
    label_colored(ctx, log_, ctx.theme().text_muted);
    layout_row(ctx, 2, nullptr, 0, 0);
    if (button(ctx, "Popup").clicked) open_popup(ctx, "pop");
    if (button(ctx, "Dialog", ButtonStyle::Primary).clicked) open_modal(ctx, "confirm");
    if (popup_begin(ctx, "pop")) {
        layout_row(ctx, 1, nullptr, 0, 0);
        label(ctx, "A popup anchored under the button");
        if (button(ctx, "Close").clicked) close_popup(ctx);
        popup_end(ctx);
    }
    if (modal_begin(ctx, "confirm", "Confirm", 320)) {
        layout_row(ctx, 1, nullptr, 0, 0);
        label_wrapped(ctx,
                      "Modal dialogs dim and block the UI behind them. Press Escape or choose a button.");
        layout_row(ctx, 2, nullptr, 0, 0);
        if (button(ctx, "Cancel").clicked) close_modal(ctx);
        if (button(ctx, "OK", ButtonStyle::Primary).clicked) close_modal(ctx);
        modal_end(ctx);
    }
    if (show_hint_) {
        layout_row(ctx, 1, nullptr, 0, 0);
        label_colored(ctx, "Hover any button for a tooltip.", ctx.theme().text_muted);
    }
}

void Gallery::panel_theme(Context& ctx) {
    separator_text(ctx, "Preset");
    static const char* const presets[] = {"Wavestorm dark", "Light", "Midnight (blue)"};
    row2(ctx, "Theme");
    if (combo(ctx, "##preset", preset_, presets, 3)) {
        theme_ = preset_ == 0 ? Theme::dark() : (preset_ == 1 ? Theme::light() : Theme::midnight());
        apply_theme(ctx);
    }
    separator_text(ctx, "Accent");
    Color accent = theme_.accent;
    if (color_picker(ctx, "##accent", accent, false)) {
        theme_ = Theme::with_accent(theme_, accent);
        apply_theme(ctx);
    }
    separator_text(ctx, "Metrics");
    bool changed = false;
    row2(ctx, "Corner radius");
    changed |= slider(ctx, "##cr", theme_.corner_radius, 0, 14, "%.0f").changed;
    row2(ctx, "Window radius");
    changed |= slider(ctx, "##wr", theme_.window_radius, 0, 20, "%.0f").changed;
    row2(ctx, "Spacing");
    changed |= slider(ctx, "##sp", theme_.spacing, 0, 16, "%.0f").changed;
    row2(ctx, "Padding");
    changed |= slider(ctx, "##pd", theme_.padding, 2, 20, "%.0f").changed;
    row2(ctx, "Row height");
    changed |= slider(ctx, "##rh", theme_.cell_height, 20, 44, "%.0f").changed;
    row2(ctx, "Shadows");
    changed |= toggle(ctx, "##sh", theme_.shadows).changed;
    if (changed) apply_theme(ctx);
}

void Gallery::panel_theme_preview(Context& ctx) {
    separator_text(ctx, "Preview");
    layout_row(ctx, 3, nullptr, 0, 0);
    button(ctx, "Default");
    button(ctx, "Primary", ButtonStyle::Primary);
    button(ctx, "Danger", ButtonStyle::Danger);
    layout_row(ctx, 2, nullptr, 0, 0);
    checkbox(ctx, "Checkbox", check_a_);
    toggle(ctx, "Switch", toggle_a_);
    row2(ctx, "Slider");
    slider(ctx, "##pv", slider_f_, 0, 1);
    row2(ctx, "Text");
    textbox(ctx, "##pvt", name_buf_);
    layout_row(ctx, 1, nullptr, 0, 0);
    progress_bar(ctx, slider_f_);
    separator_text(ctx, "A scoped override (push_theme)");
    Theme alt = Theme::with_accent(ctx.theme(), {61, 214, 140, 255});
    push_theme(ctx, alt);
    layout_row(ctx, 2, nullptr, 0, 0);
    button(ctx, "Green primary", ButtonStyle::Primary);
    toggle(ctx, "Green switch", toggle_b_);
    pop_theme(ctx);
}

void Gallery::dashboard(Context& ctx, const Input& in, int w, int h) {
    const Theme& t = ctx.theme();
    const float W = (float)w, H = (float)h, rw = 290, tb = 52, top = 84, bot = 34;
    (void)in;
    dash_[0] = {0, 0, W, top};
    dash_[1] = {0, top, tb, H - top - bot};
    dash_[2] = {tb + 8, top + 4, W - tb - rw - 24, H - top - bot - 8};
    const float avail = H - top - bot - 8, h3 = 262, h4 = 176;
    dash_[3] = {W - rw - 4, top + 4, rw, h3};
    dash_[4] = {W - rw - 4, top + 12 + h3, rw, h4};
    dash_[5] = {W - rw - 4, top + 20 + h3 + h4, rw, avail - h3 - h4 - 16};
    dash_[6] = {0, H - bot, W, bot};

    if (begin_window(ctx, "dash-top", dash_[0], WindowNone)) {
        begin_menu_bar(ctx);
        for (const char* m :
             {"File", "Edit", "Image", "Layer", "Select", "Filter", "View", "Window", "Help"}) {
            if (begin_menu(ctx, m)) {
                menu_item(ctx, "Item one");
                menu_item(ctx, "Item two", "Ctrl+K");
                end_menu(ctx);
            }
        }
        end_menu_bar(ctx);
        const float cols[] = {200, 54, -140, 54, -1};
        layout_row(ctx, 5, cols, 5, 0);
        static const char* const brushes[] = {"Freehand Brush", "Ink Pen", "Fineliner", "Round Brush"};
        combo(ctx, "##brush", brush_, brushes, 4);
        label(ctx, "Size");
        slider(ctx, "##size", size_, 1, 100, "%.0f px");
        label(ctx, "Opacity");
        slider(ctx, "##op", opacity_, 0, 100, "%.0f%%");
        end_window(ctx);
    }
    if (begin_window(ctx, "dash-tools", dash_[1], WindowNone)) {
        const Icon tools[] = {Icon::Menu,   Icon::Plus,     Icon::Pencil, Icon::Minus,
                              Icon::Folder, Icon::Settings, Icon::Search, Icon::Eye};
        const char* ids[] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7"};
        for (int i = 0; i < 8; i++) {
            layout_row(ctx, 1, nullptr, 0, 36);
            if (icon_button(ctx, ids[i], tools[i], tool_ == i).clicked) tool_ = i;
        }
        end_window(ctx);
    }
    if (begin_window(ctx, "dash-canvas", dash_[2], WindowNone)) {
        layout_row(ctx, 1, nullptr, 0, 0);
        label_colored(ctx, "cityscape  1983 x 793 px", t.text_muted);
        if (checker_) {
            layout_row(ctx, 1, nullptr, 0, 0);
            image(ctx, checker_, {dash_[2].w - 32, dash_[2].h - 80});
        }
        end_window(ctx);
    }
    if (begin_window(ctx, "dash-color", dash_[3], WindowBorder)) {
        static const char* const tabs[] = {"Color", "Swatches", "Gradients", "Patterns"};
        layout_row(ctx, 1, nullptr, 0, 0);
        tab_bar(ctx, "##ct", dash_tab_, tabs, 4);
        if (dash_tab_ == 1) {
            static const float sw[] = {40};
            layout_row(ctx, 6, sw, 1, 0);
            for (int i = 0; i < 6; i++) {
                char id[8];
                std::snprintf(id, sizeof id, "sw%d", i);
                if (color_button(ctx, id, swatches_[i]).clicked) brush_color_ = swatches_[i];
            }
        } else {
            color_picker(ctx, "##bc", brush_color_, true);
        }
        end_window(ctx);
    }
    if (begin_window(ctx, "dash-brushes", dash_[4], WindowBorder)) {
        static const char* const tabs[] = {"Brushes", "Navigator", "History"};
        layout_row(ctx, 1, nullptr, 0, 0);
        tab_bar(ctx, "##bt", brush_tab_, tabs, 3);
        layout_row(ctx, 1, nullptr, 0, 0);
        textbox(ctx, "##bs", brush_search_buf_, "Search brushes...");
        static const char* const items[] = {"All brushes", "Favorites", "Inking",  "Painting",
                                            "Shading",     "Sketching", "Textures"};
        listbox(ctx, "##bl", brush_, items, 7, -1);
        end_window(ctx);
    }
    if (begin_window(ctx, "dash-layers", dash_[5], WindowBorder)) {
        static const char* const tabs[] = {"Layers", "Channels", "Paths"};
        layout_row(ctx, 1, nullptr, 0, 0);
        tab_bar(ctx, "##lt", layers_tab_, tabs, 3);
        const float cols[] = {-1, 60};
        layout_row(ctx, 2, cols, 2, 0);
        label_colored(ctx, "Normal blend", t.text_muted);
        label_colored(ctx, "100%", t.text_muted);
        static const char* const names[] = {"Cat", "Desk", "Plants", "Artwork", "City", "Sky", "Background"};
        for (int i = 0; i < 7; i++) {
            const float rc[] = {30, -34, 30};
            layout_row(ctx, 3, rc, 3, 0);
            if (s_eye(ctx, i, vis_[i])) vis_[i] = !vis_[i];
            if (list_item(ctx, names[i], layer_sel_ == i).clicked) layer_sel_ = i;
            if (s_lock(ctx, i, lock_[i])) lock_[i] = !lock_[i];
        }
        end_window(ctx);
    }
    if (begin_window(ctx, "dash-status", dash_[6], WindowNone)) {
        const float cols[] = {70, 170, 140, -200, 190};
        layout_row(ctx, 5, cols, 5, 0);
        char b[32];
        std::snprintf(b, sizeof b, "%.0f%%", (double)(zoom_ * 100));
        label_colored(ctx, b, t.text_muted);
        label_colored(ctx, "1983 x 793 px", t.text_muted);
        label_colored(ctx, "300 DPI", t.text_muted);
        label_colored(ctx, "", t.text_muted);
        slider(ctx, "##zoom", zoom_, 0.1f, 4.0f, "%.2fx");
        end_window(ctx);
    }
}

void Gallery::panel_inputs2(Context& ctx) {
    separator_text(ctx, "More text input");
    row2(ctx, "Password");
    password_box(ctx, "##pw", pw_buf_, "At least 8 characters");
    row2(ctx, "Search box");
    search_box(ctx, "##find", find_buf_, "Search...");
    layout_row(ctx, 1, nullptr, 0, 0);
    text_area(ctx, "##notes", notes_buf_, 96, "Notes");
}

void Gallery::panel_choices(Context& ctx) {
    separator_text(ctx, "Choices");
    static const char* const spans[] = {"Day", "Week", "Month"};
    layout_row(ctx, 1, nullptr, 0, 0);
    segmented(ctx, "##seg", seg_, spans, 3);
    row2(ctx, "Range");
    range_slider(ctx, "##range", range_lo_, range_hi_, 0, 100);
    layout_row(ctx, 1, nullptr, 0, 0);
    char b[48];
    std::snprintf(b, sizeof b, "%.0f to %.0f", (double)range_lo_, (double)range_hi_);
    label_colored(ctx, b, ctx.theme().text_muted);
    static const char* const tags[] = {"Design", "Code", "Docs", "Tests"};
    layout_row(ctx, 4, nullptr, 0, 0);
    for (int i = 0; i < 4; i++)
        if (chip(ctx, tags[i], chips_[i]).clicked) chips_[i] = !chips_[i];
    row2(ctx, "Rating");
    rating(ctx, "##stars", stars_);
    row2(ctx, "Date");
    date_picker(ctx, "##date", year_, month_, day_);
}

void Gallery::panel_table(Context& ctx) {
    separator_text(ctx, "Sortable table");
    struct Row {
        const char* name;
        const char* kind;
        int kb;
    };
    static const Row rows[] = {{"brushes.cfg", "Config", 4},
                               {"cat.png", "Image", 812},
                               {"city.psd", "Artwork", 48210},
                               {"notes.txt", "Text", 2},
                               {"palette.gpl", "Palette", 1},
                               {"readme.md", "Text", 6},
                               {"sky.png", "Image", 1930},
                               {"sketch-01.kra", "Artwork", 9120},
                               {"sketch-02.kra", "Artwork", 7714},
                               {"swatches.ase", "Palette", 3},
                               {"theme.json", "Config", 2},
                               {"thumb.jpg", "Image", 96},
                               {"todo.txt", "Text", 1},
                               {"wave.svg", "Vector", 14}};
    constexpr int n = (int)(sizeof rows / sizeof rows[0]), per = 5;
    int order[n];
    for (int i = 0; i < n; i++) order[i] = i;
    std::sort(order, order + n, [&](int a, int b) {
        int c = sort_col_ == 2   ? rows[a].kb - rows[b].kb
                : sort_col_ == 1 ? std::strcmp(rows[a].kind, rows[b].kind)
                                 : std::strcmp(rows[a].name, rows[b].name);
        if (c == 0) c = a - b;
        return sort_asc_ ? c < 0 : c > 0;
    });
    static const char* const cols[] = {"Name", "Type", "Size"};
    static const float widths[] = {-170, 80, -1};
    if (table_header(ctx, "tbl", cols, 3, widths, &sort_col_, &sort_asc_)) table_page_ = 0;
    for (int i = table_page_ * per; i < n && i < (table_page_ + 1) * per; i++) {
        const Row& r = rows[order[i]];
        if (table_row(ctx, i, table_sel_ == order[i]).clicked) table_sel_ = order[i];
        label(ctx, r.name);
        label_colored(ctx, r.kind, ctx.theme().text_muted);
        char sz[24];
        std::snprintf(sz, sizeof sz, "%d KB", r.kb);
        label(ctx, sz);
    }
    layout_row(ctx, 1, nullptr, 0, 0);
    pagination(ctx, "##pages", table_page_, (n + per - 1) / per);
}

void Gallery::panel_clipper(Context& ctx) {
    separator_text(ctx, "100,000 rows with list_clipper");
    if (begin_child(ctx, "big", 190)) {
        int first = 0, last = 0;
        list_clipper_begin(ctx, 100000, 0, first, last);
        for (int i = first; i < last; i++) {
            char b[32];
            std::snprintf(b, sizeof b, "Row %d", i + 1);
            layout_row(ctx, 1, nullptr, 0, 0);
            if (list_item(ctx, b, big_sel_ == i).clicked) big_sel_ = i;
        }
        list_clipper_end(ctx);
        end_child(ctx);
    }
}

void Gallery::panel_calendar(Context& ctx) {
    separator_text(ctx, "Calendar");
    calendar(ctx, "##cal", year_, month_, day_);
    separator_text(ctx, "Card");
    if (begin_card(ctx, "profile")) {
        const float cols[] = {40, -1};
        layout_row(ctx, 2, cols, 2, 40);
        avatar(ctx, "Ada Lovelace", ctx.theme().accent);
        heading(ctx, "Ada Lovelace");
        layout_row(ctx, 1, nullptr, 0, 0);
        label_wrapped(ctx, "Cards group related content and grow to fit whatever you put inside them.");
        layout_row(ctx, 2, nullptr, 0, 0);
        if (button(ctx, "Message").clicked) toast(ctx, "Message sent", Tone::Success);
        button(ctx, "Follow", ButtonStyle::Primary);
        end_card(ctx);
    }
}

void Gallery::panel_alerts(Context& ctx) {
    separator_text(ctx, "Alerts");
    layout_row(ctx, 1, nullptr, 0, 0);
    alert(ctx, Tone::Info, "A new version is available.");
    alert(ctx, Tone::Success, "Your changes were saved.");
    alert(ctx, Tone::Warning, "Storage is almost full. Remove some files to keep syncing.");
    alert(ctx, Tone::Error, "Could not reach the server.");
    separator_text(ctx, "Toasts");
    layout_row(ctx, 4, nullptr, 0, 0);
    if (button(ctx, "Info").clicked) toast(ctx, "Heads up", Tone::Info);
    if (button(ctx, "Success").clicked) toast(ctx, "Saved", Tone::Success);
    if (button(ctx, "Warning").clicked) toast(ctx, "Battery low", Tone::Warning);
    if (button(ctx, "Error").clicked) toast(ctx, "Upload failed", Tone::Error);
    separator_text(ctx, "Loading");
    row2(ctx, "Progress ring");
    progress_ring(ctx, slider_f_);
    row2(ctx, "Skeleton");
    skeleton(ctx, 0.9f);
    layout_row(ctx, 1, nullptr, 0, 0);
    skeleton(ctx, 1.0f);
    skeleton(ctx, 0.6f);
}

void Gallery::panel_overlays(Context& ctx) {
    separator_text(ctx, "Drawers and sheets");
    layout_row(ctx, 2, nullptr, 0, 0);
    if (button(ctx, "Bottom sheet").clicked) want_sheet_ = true;
    if (button(ctx, "Side drawer").clicked) want_side_ = true;
    separator_text(ctx, "Context menu");
    layout_row(ctx, 1, nullptr, 0, 52);
    button(ctx, "Right-click or long-press here", ButtonStyle::Ghost);
    if (context_menu_begin(ctx, "ctx")) {
        if (menu_item(ctx, "Copy", "Ctrl+C")) toast(ctx, "Copied");
        if (menu_item(ctx, "Duplicate")) toast(ctx, "Duplicated");
        if (menu_item(ctx, "Delete")) toast(ctx, "Deleted", Tone::Error);
        popup_end(ctx);
    }
    separator_text(ctx, "Disabled sections");
    layout_row(ctx, 1, nullptr, 0, 0);
    toggle(ctx, "Enable the form", form_on_);
    begin_disabled(ctx, !form_on_);
    row2(ctx, "Name");
    textbox(ctx, "##dn", name_buf_);
    layout_row(ctx, 2, nullptr, 0, 0);
    checkbox(ctx, "Subscribe", check_b_);
    button(ctx, "Send", ButtonStyle::Primary);
    end_disabled(ctx);
    separator_text(ctx, "Transitions");
    static const char* const steps[] = {"One", "Two", "Three"};
    static const char* const texts[] = {
        "transition_begin fades and slides its content in whenever the key changes.",
        "Popups, dialogs, drawers, toasts and tooltips animate on their own.",
        "Config::animations or Context::set_animations(false) turns all motion off."};
    layout_row(ctx, 1, nullptr, 0, 0);
    segmented(ctx, "##steps", tr_page_, steps, 3);
    transition_begin(ctx, "steps", tr_page_);
    layout_row(ctx, 1, nullptr, 0, 0);
    label_wrapped(ctx, texts[tr_page_]);
    transition_end(ctx);
}

void Gallery::panel_native(Context& ctx) {
    separator_text(ctx, "Native integration");
    layout_row(ctx, 1, nullptr, 0, 0);
    if (link(ctx, "Open libsdl.org in the browser").clicked && runner_)
        runner_->open_url("https://libsdl.org");
    layout_row(ctx, 2, nullptr, 0, 0);
    if (button(ctx, "Open file...").clicked && runner_) runner_->show_open_dialog();
    if (button(ctx, "Light / dark").clicked) {
        const bool to_dark = theme_.background.r > 128;
        theme_ = make_theme(fonts_, to_dark);
        apply_theme(ctx);
    }
    std::string path;
    if (runner_ && runner_->poll_dialog(path))
        std::snprintf(picked_, sizeof picked_, "%s", path.empty() ? "Cancelled" : path.c_str());
    layout_row(ctx, 1, nullptr, 0, 0);
    label_colored(ctx, picked_, ctx.theme().text_muted);
    bullet_text(ctx, "The theme follows the system light/dark setting");
    bullet_text(ctx, "The cursor changes over text and resize handles");
    bullet_text(ctx, "Touch: flick to scroll, long-press for menus");
    bullet_text(ctx, "Android back closes popups, then the app");
    bullet_text(ctx, "The app sleeps while nothing is moving");
}

void Gallery::panel_flow(Context& ctx) {
    separator_text(ctx, "Flow layout (wraps)");
    static const char* const tags[] = {"Design", "Code",   "Docs",   "Tests", "Mobile",
                                       "Games",  "Charts", "Layout", "Fonts", "Accessibility"};
    flow_begin(ctx);
    for (int i = 0; i < 10; i++)
        if (chip(ctx, tags[i], tags_[i]).clicked) tags_[i] = !tags_[i];
    button(ctx, "A button");
    link(ctx, "a link");
    flow_end(ctx);
    separator_text(ctx, "Responsive grid");
    const int n = layout_grid(ctx, 64, 56);
    const Icon icons[] = {Icon::Home,     Icon::Mail,     Icon::Phone, Icon::Image,
                          Icon::Bookmark, Icon::Pin,      Icon::Send,  Icon::Archive,
                          Icon::Upload,   Icon::Download, Icon::Link,  Icon::Filter};
    const char* ids[] = {"g0", "g1", "g2", "g3", "g4", "g5", "g6", "g7", "g8", "g9", "g10", "g11"};
    for (int i = 0; i < 12; i++) icon_button(ctx, ids[i], icons[i]);
    layout_row(ctx, 1, nullptr, 0, 0);
    char b[48];
    std::snprintf(b, sizeof b, "%d columns fit this width", n);
    label_colored(ctx, b, ctx.theme().text_muted);
}

void Gallery::panel_charts(Context& ctx) {
    separator_text(ctx, "Line and area");
    static const float sales[] = {12, 19, 15, 24, 28, 22, 31, 35, 30, 38, 42, 47};
    static const float costs[] = {10, 12, 14, 15, 17, 18, 19, 22, 24, 23, 26, 28};
    static const char* const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const ChartSeries s2[] = {{"Revenue", sales, 12}, {"Costs", costs, 12}};
    layout_row(ctx, 1, nullptr, 0, 0);
    chart(ctx, "##area", ChartType::Area, s2, 2, months, 190);
    separator_text(ctx, "Bars");
    static const float q1[] = {4, 7, 3, 8}, q2[] = {5, 6, 6, 9}, q3[] = {2, 3, 4, 2};
    static const char* const teams[] = {"North", "South", "East", "West"};
    const ChartSeries s3[] = {{"Q1", q1, 4}, {"Q2", q2, 4}, {"Q3", q3, 4}};
    chart(ctx, "##bars", ChartType::Bar, s3, 3, teams, 170);
    chart(ctx, "##stack", ChartType::StackedBar, s3, 3, teams, 170);
}

void Gallery::panel_charts2(Context& ctx) {
    separator_text(ctx, "Pie and donut");
    static const float share[] = {42, 26, 18, 9, 5};
    static const char* const os[] = {"Android", "iOS", "Windows", "macOS", "Linux"};
    layout_row(ctx, 1, nullptr, 0, 0);
    pie_chart(ctx, "##donut", share, os, 5, 150, true);
    pie_chart(ctx, "##pie", share, os, 5, 130, false);
    separator_text(ctx, "Scatter");
    static float pts[40];
    for (int i = 0; i < 40; i++) pts[i] = 20 + 15 * std::sin((float)i * 0.7f) + (float)((i * 37) % 11);
    const ChartSeries sc[] = {{"Samples", pts, 40}};
    chart(ctx, "##scatter", ChartType::Scatter, sc, 1, nullptr, 140);
    separator_text(ctx, "Sparklines");
    static const float a[] = {3, 5, 4, 7, 6, 9, 8, 11}, b[] = {9, 8, 8, 6, 7, 5, 4, 3},
                       c[] = {2, 6, 3, 7, 2, 8, 3, 9};
    const float cols[] = {90, -80, 70};
    const char* names[] = {"Users", "Errors", "Load"};
    const float* data[] = {a, b, c};
    const char* now[] = {"11.2k", "3", "9%"};
    const Theme& t = ctx.theme();
    const Color colors[] = {t.success, t.error, t.accent};
    for (int i = 0; i < 3; i++) {
        layout_row(ctx, 3, cols, 3, 0);
        label(ctx, names[i]);
        sparkline(ctx, data[i], 8, colors[i]);
        label_colored(ctx, now[i], t.text_muted);
    }
}

void Gallery::panel_tree_table(Context& ctx) {
    separator_text(ctx, "Tree table: drag the header edges, click to sort");
    struct Item {
        const char* name;
        const char* size;
        const char* kind;
        int depth, parent;
    };
    static const Item items[] = {
        {"assets", "", "Folder", 0, -1},        {"brushes.cfg", "4 KB", "Config", 1, 0},
        {"city.psd", "48 MB", "Artwork", 1, 0}, {"src", "", "Folder", 0, -1},
        {"main.cpp", "6 KB", "C++", 1, 3},      {"gallery.cpp", "38 KB", "C++", 1, 3},
        {"docs", "", "Folder", 0, -1},          {"widgets.md", "22 KB", "Markdown", 1, 6}};
    const int folder_of[] = {0, -1, -1, 1, -1, -1, 2, -1};
    if (table_begin(ctx, "files", tcols_, 3, 200, &tsort_)) {
        if (tsort_.changed) toast(ctx, tsort_.ascending ? "Sorted ascending" : "Sorted descending");
        int row = 0;
        for (int i = 0; i < 8; i++) {
            const Item& it = items[i];
            if (it.parent >= 0 && !tree_open_[folder_of[it.parent]]) continue;
            bool* open = folder_of[i] >= 0 ? &tree_open_[folder_of[i]] : nullptr;
            if (table_tree_row(ctx, row++, it.name, it.depth, open, tsel_ == i).clicked) tsel_ = i;
            label(ctx, it.size);
            label_colored(ctx, it.kind, ctx.theme().text_muted);
        }
        table_end(ctx);
    }
}

void Gallery::panel_pickers(Context& ctx) {
    separator_text(ctx, "Wheels");
    static const char* const sizes[] = {"XS", "S", "M", "L", "XL", "XXL"};
    static const char* const colors[] = {"Black", "White", "Pink", "Navy", "Olive"};
    layout_row(ctx, 2, nullptr, 0, 0);
    wheel_picker(ctx, "##size", wheel_a_, sizes, 6, 5);
    wheel_picker(ctx, "##color", wheel_b_, colors, 5, 5);
    layout_row(ctx, 1, nullptr, 0, 0);
    char b[64];
    std::snprintf(b, sizeof b, "%s, %s", sizes[wheel_a_], colors[wheel_b_]);
    label_colored(ctx, b, ctx.theme().text_muted);
    separator_text(ctx, "Time");
    row2(ctx, "24-hour");
    time_picker(ctx, "##t24", hour_, minute_, true);
    row2(ctx, "12-hour");
    time_picker(ctx, "##t12", hour_, minute_, false);
}

void Gallery::panel_pickers2(Context& ctx) {
    separator_text(ctx, "Searchable and multiple choice");
    static const char* const countries[] = {"Argentina", "Australia",   "Belgium", "Brazil",   "Canada",
                                            "Denmark",   "Finland",     "France",  "Germany",  "Japan",
                                            "Kenya",     "Netherlands", "Norway",  "Portugal", "Spain"};
    row2(ctx, "Country");
    combo_search(ctx, "##country", csearch_, countries, 15);
    static const char* const fruit[] = {"Apple", "Apricot", "Banana", "Blueberry", "Cherry", "Mango"};
    row2(ctx, "Fruit");
    multi_combo(ctx, "##fruit", fruits_, fruit, 6);
    row2(ctx, "Autocomplete");
    autocomplete(ctx, "##auto", fruit_buf_, fruit, 6, "Type a fruit");
    separator_text(ctx, "Number pad");
    row2(ctx, "Amount");
    textbox(ctx, "##amount", amount_buf_, "0", TextInputType::Decimal);
    layout_row(ctx, 1, nullptr, 0, 0);
    number_pad(ctx, "##pad", amount_buf_, true);
}

void Gallery::panel_richtext(Context& ctx) {
    separator_text(ctx, "Rich text");
    layout_row(ctx, 1, nullptr, 0, 0);
    if (!rich_text(ctx,
                   "Mix **bold**, *italic* and `code` with {#3dd68c}colored text{/}, icons :star: :heart: "
                   ":check: and [links](https://github.com/WavestormSoftware/Radon). Lines wrap at the "
                   "width of the cell.")
             .link.empty())
        toast(ctx, "Link clicked");
    separator_text(ctx, "Selectable text");
    selectable_text(ctx, "##sel", "Select me with the mouse, or a long press\non a touch screen, then copy.");
}

void Gallery::panel_markdown(Context& ctx) {
    separator_text(ctx, "Markdown");
    layout_row(ctx, 1, nullptr, 0, 0);
    const RichTextResponse r = markdown(ctx,
                                        "# Release notes\n"
                                        "Radon **0.3** adds touch gestures, pickers, charts, docking and "
                                        "*rich text*.\n\n"
                                        "## Highlights\n"
                                        "- Pull to refresh, swipe rows, edge swipes\n"
                                        "- Undo and redo in every text box\n"
                                        "- Drag and drop with `drag_source`\n\n"
                                        "1. Read the [widget guide](docs/widgets.md)\n"
                                        "2. Run the gallery\n\n"
                                        "> Every widget still allocates nothing per frame.\n\n"
                                        "```\nrd::markdown(ctx, text);\n```\n"
                                        "---\n");
    if (!r.link.empty()) toast(ctx, "Link clicked");
}

void Gallery::panel_gestures(Context& ctx) {
    separator_text(ctx, "Pull to refresh");
    if (begin_child(ctx, "feed", 170)) {
        pull_to_refresh(ctx, "ptr", refreshing_);
        if (refreshing_ && refresh_clock_ <= 0) refresh_clock_ = 1.2f;
        for (int i = 0; i < 8; i++) {
            char b[48];
            std::snprintf(b, sizeof b, "Post %d (refreshed %d times)", i + 1, refreshes_);
            layout_row(ctx, 1, nullptr, 0, 0);
            list_item(ctx, b, false, Icon::Image);
        }
        end_child(ctx);
    }
    layout_row(ctx, 1, nullptr, 0, 0);
    label_colored(ctx, "Drag the list down with a finger (or the mouse in touch mode).",
                  ctx.theme().text_muted);
    separator_text(ctx, "Carousel");
    if (carousel_begin(ctx, "cards", 150, 180)) {
        const char* titles[] = {"Sunrise", "City", "Forest", "Desert", "Ocean", "Night"};
        for (int i = 0; i < 6; i++) {
            layout_begin_column(ctx);
            char id[8];
            std::snprintf(id, sizeof id, "c%d", i);
            push_id(ctx, id);
            if (begin_card(ctx, "card")) {
                layout_row(ctx, 1, nullptr, 0, 0);
                heading(ctx, titles[i]);
                label_colored(ctx, "Swipe or scroll", ctx.theme().text_muted);
                end_card(ctx);
            }
            pop_id(ctx);
            layout_end_column(ctx);
        }
        carousel_end(ctx);
    }
    separator_text(ctx, "Pinch and swipe");
    const Gesture& g = ctx.gesture();
    if (g.pinch != 1) zoom2_ = std::fmin(std::fmax(zoom2_ * g.pinch, 0.5f), 3.0f);
    if (g.swipe != SwipeDir::None) {
        static const char* const dirs[] = {"", "left", "right", "up", "down"};
        std::snprintf(gesture_log_, sizeof gesture_log_, "Swiped %s at %.0f px/s", dirs[(int)g.swipe],
                      (double)std::hypot(g.velocity.x, g.velocity.y));
    }
    layout_row(ctx, 1, nullptr, 0, 0);
    Canvas cv = canvas_begin(ctx, {1000, 120}, ctx.theme().background);
    zoom2_ = std::fmin(std::fmax(zoom2_ * cv.pinch, 0.5f), 3.0f);
    pan_.x += cv.pan2.x;
    pan_.y += cv.pan2.y;
    const float r = 24 * zoom2_;
    cv.circle({cv.rect.w / 2 + pan_.x, 60 + pan_.y}, r, ctx.theme().accent);
    canvas_end(ctx);
    label_colored(ctx, gesture_log_, ctx.theme().text_muted);
}

void Gallery::panel_swipes(Context& ctx) {
    separator_text(ctx, "Swipe a row left");
    static const char* const mail[] = {"Invoice #2041",      "Lunch on Friday?", "Your build passed",
                                       "New login detected", "Weekly report",    "Design review notes"};
    const SwipeAction acts[] = {{"Archive", Icon::Archive, ctx.theme().warning},
                                {"Delete", Icon::Trash, ctx.theme().error}};
    for (int i = 0; i < inbox_n_; i++) {
        layout_row(ctx, 1, nullptr, 0, 0);
        push_id(ctx, mail[inbox_[i]]);
        int act = -1;
        if (swipe_item(ctx, mail[inbox_[i]], acts, 2, act).clicked) toast(ctx, mail[inbox_[i]]);
        pop_id(ctx);
        if (act >= 0) {
            toast(ctx, act == 0 ? "Archived" : "Deleted", act == 0 ? Tone::Info : Tone::Error);
            for (int k = i; k + 1 < inbox_n_; k++) inbox_[k] = inbox_[k + 1];
            inbox_n_--;
            break;
        }
    }
    layout_row(ctx, 1, nullptr, 0, 0);
    if (inbox_n_ < 6 && button(ctx, "Restore all").clicked) {
        for (int i = 0; i < 6; i++) inbox_[i] = i;
        inbox_n_ = 6;
    }
    separator_text(ctx, "Accordion");
    static const char* const qs[] = {"What is Radon?", "Does it allocate?", "Which platforms?"};
    static const char* const as[] = {
        "An immediate-mode UI library for C++17 with a draw list you render yourself.",
        "Every pool is allocated once from a Config; frames make no heap allocations.",
        "Windows, Linux, macOS, the web, Android and iOS through SDL3, or your own backend."};
    for (int i = 0; i < 3; i++) {
        push_id(ctx, qs[i]);
        if (accordion_begin(ctx, qs[i], acc_open_, i)) {
            layout_row(ctx, 1, nullptr, 0, 0);
            label_wrapped(ctx, as[i]);
            accordion_end(ctx);
        }
        pop_id(ctx);
    }
}

void Gallery::panel_dnd(Context& ctx) {
    separator_text(ctx, "Reorder (drag a row)");
    static const char* const steps[] = {"Sketch", "Ink", "Color", "Shade", "Export"};
    reorder_list(ctx, "steps", order_, steps, 5);
    separator_text(ctx, "Drag tags between boxes");
    static const char* const tags[] = {"red", "green", "blue", "cyan", "pink", "gold"};
    const float cols[] = {-1, -1};
    layout_row(ctx, 2, cols, 2, 0);
    for (int box = 0; box < 2; box++) {
        layout_begin_column(ctx);
        layout_row(ctx, 1, nullptr, 0, 0);
        label_colored(ctx, box == 0 ? "Inbox" : "Done", ctx.theme().text_muted);
        for (int i = 0; i < 6; i++) {
            if (bucket_[i] != box) continue;
            layout_row(ctx, 1, nullptr, 0, 0);
            chip(ctx, tags[i], box == 1);
            drag_source(ctx, "tag", &i, sizeof i, tags[i]);
        }
        layout_row(ctx, 1, nullptr, 0, 40);
        button(ctx, box == 0 ? "Drop here##0" : "Drop here##1", ButtonStyle::Ghost);
        int n = 0;
        if (const void* p = drop_target(ctx, "tag", &n)) {
            int i;
            std::memcpy(&i, p, sizeof i);
            if (i >= 0 && i < 6) bucket_[i] = box;
        }
        layout_end_column(ctx);
    }
    layout_row(ctx, 1, nullptr, 0, 0);
    label_colored(ctx, dragging(ctx) ? "Dragging..." : "Mouse: press and drag. Touch: long-press, then drag.",
                  ctx.theme().text_muted);
}

void Gallery::docking(Context& ctx, Rect area) {
    dock_space(ctx, "dock", area);
    if (begin_dock_panel(ctx, "Scene")) {
        layout_row(ctx, 1, nullptr, 0, 0);
        label_colored(ctx, "Drag a tab to float it; drag a title onto a highlighted zone to dock it.",
                      ctx.theme().text_muted);
        Canvas cv = canvas_begin(ctx, {2000, 300}, ctx.theme().background);
        for (int i = 0; i < 6; i++)
            cv.fill_rect({40.0f + 70.0f * (float)i, 40.0f + 20.0f * (float)(i % 3), 50, 50},
                         ctx.theme().chart[i], 8);
        canvas_end(ctx);
        end_dock_panel(ctx);
    }
    if (dock_assets_ && begin_dock_panel(ctx, "Assets", &dock_assets_, DockPlace::Left, 0.2f)) {
        layout_row(ctx, 1, nullptr, 0, 0);
        static const char* const files[] = {"city.psd", "cat.png", "sky.png", "brushes.cfg", "theme.json"};
        for (int i = 0; i < 5; i++) list_item(ctx, files[i], i == 1, Icon::Image);
        end_dock_panel(ctx);
    }
    if (dock_inspector_ && begin_dock_panel(ctx, "Inspector", &dock_inspector_, DockPlace::Right, 0.25f)) {
        panel_toggles(ctx);
        panel_sliders(ctx);
        end_dock_panel(ctx);
    }
    if (dock_stats_ && begin_dock_panel(ctx, "Stats", &dock_stats_, DockPlace::Right, 0.25f)) {
        panel_charts2(ctx);
        end_dock_panel(ctx);
    }
    if (dock_console_ && begin_dock_panel(ctx, "Console", &dock_console_, DockPlace::Bottom, 0.25f)) {
        layout_row(ctx, 1, nullptr, 0, 0);
        label_colored(ctx, "[info] gallery started", ctx.theme().text_muted);
        label_colored(ctx, "[info] dock layout restored", ctx.theme().text_muted);
        textbox(ctx, "##cmd", console_buf_, "Type a command");
        end_dock_panel(ctx);
    }
    if (!(dock_console_ && dock_inspector_ && dock_assets_ && dock_stats_)) {
        Rect r{area.x + area.w - 170, area.y + area.h - 46, 160, 38};
        if (begin_window(ctx, "reopen", r, WindowBorder)) {
            layout_row(ctx, 1, nullptr, 0, 0);
            if (button(ctx, "Reopen panels").clicked)
                dock_console_ = dock_inspector_ = dock_assets_ = dock_stats_ = true;
            end_window(ctx);
        }
    }
}

// Drawers live at the top level (outside any window), so they are opened through flags.
void Gallery::overlays(Context& ctx) {
    if (want_sheet_) open_modal(ctx, "sheet");
    if (want_side_) open_modal(ctx, "side");
    if (want_pages_) open_modal(ctx, "pages");
    want_sheet_ = want_side_ = want_pages_ = false;
    if (drawer_begin(ctx, "sheet", Side::Bottom, 250)) {
        layout_row(ctx, 1, nullptr, 0, 0);
        heading(ctx, "Share drawing");
        if (list_item(ctx, "Copy link", false, Icon::Copy).clicked)
            toast(ctx, "Link copied"), close_modal(ctx);
        if (list_item(ctx, "Save to device", false, Icon::Download).clicked) close_modal(ctx);
        if (list_item(ctx, "Send to...", false, Icon::Share).clicked) close_modal(ctx);
        if (button(ctx, "Cancel").clicked) close_modal(ctx);
        drawer_end(ctx);
    }
    if (drawer_begin(ctx, "side", Side::Left, 260)) {  // also opens with a swipe from the left edge
        layout_row(ctx, 1, nullptr, 0, 0);
        heading(ctx, "Radon");
        for (int i = 0; i < kPageCount; i++) {
            if (desktop_only(i) && compact_) continue;
            if (list_item(ctx, kPages[i], page_ == i, kPageIcons[i]).clicked) {
                page_ = i;
                close_modal(ctx);
            }
        }
        drawer_end(ctx);
    }
    if (drawer_begin(ctx, "pages", Side::Bottom, 380)) {
        layout_row(ctx, 1, nullptr, 0, 0);
        heading(ctx, "Pages");
        for (int i = 0; i < kPageCount; i++) {
            if (desktop_only(i)) continue;
            if (list_item(ctx, kPages[i], page_ == i, kPageIcons[i]).clicked) {
                page_ = i;
                close_modal(ctx);
            }
        }
        drawer_end(ctx);
    }
}

void Gallery::frame(Context& ctx, const Input& in, int w, int h) {
    if (refreshing_ && (refresh_clock_ -= 1.0f / 60) <= 0) {  // pretend to load something
        refreshing_ = false;
        refreshes_++;
        toast(ctx, "Refreshed", Tone::Success, 1.5f);
    }
    const bool compact = w < 760;
    compact_ = compact;
    const bool want_touch = compact || (runner_ && runner_->options().touch);
    if (!theme_init_ || want_touch != touch_) {  // phones get finger-sized rows
        if (!theme_init_) theme_ = make_theme(fonts_, runner_ ? runner_->dark() : true);
        theme_init_ = true;
        touch_ = want_touch;
        apply_theme(ctx);
    }
    const float sw = compact ? 0.0f : 210.0f;
    const Rect safe = in.safe_rect();
    const float bar = compact ? 64.0f : 0.0f;
    set_toast_offset(ctx, bar);

    if (page_ == kDashboard && !compact) {  // dashboard takes the whole window, with a small page switcher
        dashboard(ctx, in, w, h);
        Rect sw_r{(float)w - 190, 4, 180, 34};
        if (begin_window(ctx, "page-switch", sw_r, WindowNone)) {
            layout_row(ctx, 1, nullptr, 0, 0);
            combo(ctx, "##page", page_, kPages, kPageCount);
            end_window(ctx);
        }
        return;
    }
    if (desktop_only(page_) && compact) page_ = 0;
    if (compact && edge_swipe(ctx, Side::Left)) want_side_ = true;
    if (page_ == kDocking) {
        Rect nav{8, 8, sw - 8, (float)h - 16};
        if (begin_window(ctx, "Radon", nav, WindowBorder | WindowTitleBar)) {
            for (int i = 0; i < kPageCount; i++) {
                layout_row(ctx, 1, nullptr, 0, 0);
                if (list_item(ctx, kPages[i], page_ == i, kPageIcons[i]).clicked) page_ = i;
            }
            end_window(ctx);
        }
        docking(ctx, {sw + 8, 8, (float)w - sw - 16, (float)h - 16});
        overlays(ctx);
        return;
    }

    if (compact) {  // bottom navigation, extended under the home indicator
        Rect nav{0, safe.y + safe.h - bar, (float)w, bar + in.safe_area.h};
        if (begin_window(ctx, "nav", nav, WindowNone)) {
            static const int pages[] = {0, 1, 11, 8};
            static const Icon icons[] = {Icon::Star, Icon::Pencil, Icon::Phone, Icon::Grid, Icon::Menu};
            static const char* const names[] = {"Widgets", "Inputs", "Gestures", "Charts", "More"};
            int sel = 4;
            for (int i = 0; i < 4; i++)
                if (pages[i] == page_) sel = i;
            layout_row(ctx, 1, nullptr, 0, bar - 2 * ctx.theme().padding);
            if (nav_bar(ctx, "##nav", sel, icons, names, 5)) {
                if (sel == 4)
                    want_pages_ = true;
                else
                    page_ = pages[sel];
            }
            end_window(ctx);
        }
    } else {
        Rect nav{8, 8, sw - 8, (float)h - 16};
        if (begin_window(ctx, "Radon", nav, WindowBorder | WindowTitleBar)) {
            for (int i = 0; i < kPageCount; i++) {
                layout_row(ctx, 1, nullptr, 0, 0);
                if (list_item(ctx, kPages[i], page_ == i, kPageIcons[i]).clicked) page_ = i;
            }
            spacing(ctx, 12);
            layout_row(ctx, 1, nullptr, 0, 0);
            label_colored(ctx, "Radon 0.3.0", ctx.theme().text_muted);
            label_colored(ctx, "Tab: move focus", ctx.theme().text_muted);
            label_colored(ctx, "Enter/Space: activate", ctx.theme().text_muted);
            end_window(ctx);
        }
    }

    using Panel = void (Gallery::*)(Context&);
    static const Panel p0[] = {&Gallery::panel_buttons, &Gallery::panel_toggles, &Gallery::panel_sliders,
                               &Gallery::panel_feedback};
    static const Panel p1[] = {&Gallery::panel_text, &Gallery::panel_choices, &Gallery::panel_colors,
                               &Gallery::panel_inputs2, &Gallery::panel_tabs};
    static const Panel p2[] = {&Gallery::panel_rows, &Gallery::panel_columns, &Gallery::panel_regions,
                               &Gallery::panel_flow};
    static const Panel p3[] = {&Gallery::panel_menus, &Gallery::panel_lists, &Gallery::panel_tree};
    static const Panel p4[] = {&Gallery::panel_theme, &Gallery::panel_theme_preview};
    static const Panel p6[] = {&Gallery::panel_table, &Gallery::panel_calendar, &Gallery::panel_clipper};
    static const Panel p7[] = {&Gallery::panel_alerts, &Gallery::panel_overlays, &Gallery::panel_native};
    static const Panel p8[] = {&Gallery::panel_charts, &Gallery::panel_charts2, &Gallery::panel_tree_table};
    static const Panel p9[] = {&Gallery::panel_pickers, &Gallery::panel_pickers2};
    static const Panel p10[] = {&Gallery::panel_richtext, &Gallery::panel_markdown};
    static const Panel p11[] = {&Gallery::panel_gestures, &Gallery::panel_swipes, &Gallery::panel_dnd};
    static const Panel* const all[] = {p0, p1, p2, p3, p4, p4, p6, p7, p8, p9, p10, p11, p11};
    static const int counts[] = {4, 5, 4, 3, 2, 2, 3, 3, 3, 2, 2, 3, 3};
    const Panel* panels = all[page_];
    const int count = counts[page_];

    const float m = compact ? 6.0f : 8.0f;
    const float ax = compact ? safe.x + m : sw + m, ay = (compact ? safe.y : 0) + m;
    const float aw = (compact ? safe.x + safe.w : (float)w) - ax - m;
    const float ah = (compact ? safe.y + safe.h - bar : (float)h) - ay - m;
    int ncols = (int)(aw / 440);
    ncols = ncols < 1 ? 1 : (ncols > 3 ? 3 : ncols);
    if (count < ncols) ncols = count;
    const float gap = 8, cw = (aw - gap * (float)(ncols - 1)) / (float)ncols;
    for (int c = 0; c < ncols; c++) {
        cols_[c] = {ax + (float)c * (cw + gap), ay, cw, ah};
        char title[32];
        std::snprintf(title, sizeof title, "p%d-c%d", page_, c);
        if (begin_window(ctx, title, cols_[c], WindowBorder | WindowScrollable)) {
            if (compact && c == 0 && pull_to_refresh(ctx, "refresh", refreshing_)) refresh_clock_ = 1.2f;
            transition_begin(ctx, "page", page_);
            for (int i = c; i < count; i += ncols) {
                char pid[32];
                std::snprintf(pid, sizeof pid, "panel%d", i);
                push_id(ctx, pid);
                (this->*panels[i])(ctx);
                pop_id(ctx);
                layout_row(ctx, 1, nullptr, 0, 0);
                spacing(ctx, 10);
            }
            transition_end(ctx);
            end_window(ctx);
        }
    }
    overlays(ctx);
}

}  // namespace demo
