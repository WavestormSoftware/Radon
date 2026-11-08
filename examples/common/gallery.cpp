// Copyright (c) 2026 Wavestorm Software
#include "gallery.hpp"

#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

namespace demo {
using namespace rd;

namespace {

const char* const kPages[] = {"Widgets", "Inputs", "Layout", "Lists and menus", "Theming", "Dashboard"};
constexpr int kPageCount = 6;
const Icon kPageIcons[] = {Icon::Star, Icon::Pencil, Icon::Menu, Icon::Folder, Icon::Settings, Icon::Eye};

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
    ctx.set_theme(theme_);
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
    const float b[] = {-100, 100};
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
    const float w[] = {-1, -1};
    layout_row(ctx, 2, w, 2, 0);
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
    if (!theme_init_) {
        theme_ = ctx.theme();
        theme_init_ = true;
    }
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

void Gallery::frame(Context& ctx, const Input& in, int w, int h) {
    const bool compact = w < 760;
    const float sw = compact ? 0.0f : 210.0f;
    const float top = compact ? 58.0f : 0.0f;

    if (page_ == 5 && !compact) {  // dashboard takes the whole window, with a small page switcher
        dashboard(ctx, in, w, h);
        Rect sw_r{(float)w - 190, 4, 180, 34};
        if (begin_window(ctx, "page-switch", sw_r, WindowNone)) {
            layout_row(ctx, 1, nullptr, 0, 0);
            combo(ctx, "##page", page_, kPages, kPageCount);
            end_window(ctx);
        }
        return;
    }
    if (page_ == 5) page_ = 0;

    Rect nav = compact ? Rect{in.safe_area.x + 6, in.safe_area.y + 6,
                              (float)w - in.safe_area.x - in.safe_area.w - 12, 48}
                       : Rect{8, 8, sw - 8, (float)h - 16};
    if (begin_window(ctx, "Radon", nav, compact ? WindowBorder : (WindowBorder | WindowTitleBar))) {
        if (compact) {
            layout_row(ctx, 1, nullptr, 0, 0);
            combo(ctx, "##page", page_, kPages, kPageCount);
        } else {
            for (int i = 0; i < kPageCount; i++) {
                layout_row(ctx, 1, nullptr, 0, 0);
                if (list_item(ctx, kPages[i], page_ == i, kPageIcons[i]).clicked) page_ = i;
            }
            spacing(ctx, 12);
            layout_row(ctx, 1, nullptr, 0, 0);
            label_colored(ctx, "Radon 0.1.0", ctx.theme().text_muted);
            label_colored(ctx, "Tab: move focus", ctx.theme().text_muted);
            label_colored(ctx, "Enter/Space: activate", ctx.theme().text_muted);
        }
        end_window(ctx);
    }

    using Panel = void (Gallery::*)(Context&);
    std::vector<Panel> panels;
    switch (page_) {
        case 0:
            panels = {&Gallery::panel_buttons, &Gallery::panel_toggles, &Gallery::panel_sliders,
                      &Gallery::panel_feedback};
            break;
        case 1:
            panels = {&Gallery::panel_text, &Gallery::panel_colors, &Gallery::panel_tabs};
            break;
        case 2:
            panels = {&Gallery::panel_rows, &Gallery::panel_columns, &Gallery::panel_regions};
            break;
        case 3:
            panels = {&Gallery::panel_menus, &Gallery::panel_lists, &Gallery::panel_tree};
            break;
        default:
            panels = {&Gallery::panel_theme, &Gallery::panel_theme_preview};
            break;
    }
    const float ax = sw + (compact ? in.safe_area.x + 6 : 8), ay = top + (compact ? in.safe_area.y + 6 : 8);
    const float aw = (float)w - ax - (compact ? in.safe_area.w + 6 : 8),
                ah = (float)h - ay - (compact ? in.safe_area.h + 6 : 8);
    int ncols = (int)(aw / 440);
    ncols = ncols < 1 ? 1 : (ncols > 3 ? 3 : ncols);
    if ((int)panels.size() < ncols) ncols = (int)panels.size();
    const float gap = 8, cw = (aw - gap * (float)(ncols - 1)) / (float)ncols;
    for (int c = 0; c < ncols; c++) {
        cols_[c] = {ax + (float)c * (cw + gap), ay, cw, ah};
        char title[32];
        std::snprintf(title, sizeof title, "p%d-c%d", page_, c);
        if (begin_window(ctx, title, cols_[c], WindowBorder | WindowScrollable)) {
            for (size_t i = (size_t)c; i < panels.size(); i += (size_t)ncols) {
                char pid[32];
                std::snprintf(pid, sizeof pid, "panel%zu", i);
                push_id(ctx, pid);
                (this->*panels[i])(ctx);
                pop_id(ctx);
                layout_row(ctx, 1, nullptr, 0, 0);
                spacing(ctx, 10);
            }
            end_window(ctx);
        }
    }
}

}  // namespace demo
