// Copyright (c) 2026 Wavestorm Software
// The widget gallery: every Radon widget, a theme editor and a dashboard layout.
#pragma once
#include <radon/radon.hpp>

#include "runner.hpp"

namespace demo {

class Gallery {
public:
    void setup(const Fonts& fonts, int first_page) {
        fonts_ = fonts;
        page_ = first_page;
    }
    void set_checker(rd::TextureId t) { checker_ = t; }
    void frame(rd::Context& ctx, const rd::Input& in, int w, int h);
    int page() const { return page_; }

private:
    void panel_buttons(rd::Context&);
    void panel_toggles(rd::Context&);
    void panel_sliders(rd::Context&);
    void panel_feedback(rd::Context&);
    void panel_text(rd::Context&);
    void panel_colors(rd::Context&);
    void panel_tabs(rd::Context&);
    void panel_rows(rd::Context&);
    void panel_columns(rd::Context&);
    void panel_regions(rd::Context&);
    void panel_lists(rd::Context&);
    void panel_tree(rd::Context&);
    void panel_menus(rd::Context&);
    void panel_theme(rd::Context&);
    void panel_theme_preview(rd::Context&);
    void dashboard(rd::Context&, const rd::Input&, int w, int h);
    void apply_theme(rd::Context&);

    Fonts fonts_;
    int page_ = 0;
    rd::TextureId checker_ = 0;
    rd::Rect cols_[3]{}, dash_[8]{};

    // widgets page
    bool check_a_ = true, check_b_ = false, toggle_a_ = true, toggle_b_ = false;
    int radio_ = 1, slider_i_ = 40, drag_i_ = 7, step_i_ = 3, clicks_ = 0;
    float slider_f_ = 0.35f, drag_f_ = 2.0f, step_f_ = 0.5f;
    float plot_[48]{};
    int plot_head_ = 0;
    float plot_clock_ = 0;
    // inputs page
    char name_[48] = "Ada Lovelace", search_[48] = "", email_[64] = "";
    rd::TextBuffer name_buf_{name_, sizeof name_, 12}, search_buf_{search_, sizeof search_, 0},
        email_buf_{email_, sizeof email_, 0};
    int combo_ = 1, tab_ = 0;
    rd::Color color_{236, 72, 153, 255};
    rd::Color swatches_[6]{{236, 72, 153, 255}, {77, 163, 255, 255},  {61, 214, 140, 255},
                           {245, 181, 74, 255}, {160, 120, 255, 255}, {255, 93, 108, 255}};

    // lists page
    int list_sel_ = 2;
    bool open_[6]{true, true, false, false, false, false};
    bool show_hint_ = true;
    char log_[96] = "Choose a menu item";
    // theme page
    int preset_ = 0;
    rd::Theme theme_{};
    bool theme_init_ = false;
    // dashboard
    int tool_ = 2, dash_tab_ = 0, brush_ = 2, layer_sel_ = 4, brush_tab_ = 0, layers_tab_ = 0;
    bool vis_[7]{true, true, true, true, true, true, true}, lock_[7]{};
    float size_ = 24, opacity_ = 100, zoom_ = 0.56f;

    rd::Color brush_color_{0, 0, 0, 255};
    char brush_search_[32] = "";
    rd::TextBuffer brush_search_buf_{brush_search_, sizeof brush_search_, 0};
};

}  // namespace demo
