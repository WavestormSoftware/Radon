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
    /// Gives the gallery the native helpers (URLs, file dialog, system theme). Optional.
    void set_runner(Runner* r) { runner_ = r; }
    /// The OS switched between light and dark.
    void set_dark(rd::Context& ctx, bool dark) {
        theme_ = make_theme(fonts_, dark);
        apply_theme(ctx);
    }
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
    void panel_inputs2(rd::Context&);
    void panel_choices(rd::Context&);
    void panel_table(rd::Context&);
    void panel_clipper(rd::Context&);
    void panel_calendar(rd::Context&);
    void panel_alerts(rd::Context&);
    void panel_overlays(rd::Context&);
    void panel_native(rd::Context&);
    void overlays(rd::Context&);
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

    // inputs page, second half
    char pw_[32] = "hunter2", find_[32] = "",
         notes_[256] = "Text areas keep line breaks.\nPress Enter for more.";
    rd::TextBuffer pw_buf_{pw_, sizeof pw_, 7}, find_buf_{find_, sizeof find_, 0},
        notes_buf_{notes_, sizeof notes_, 51};
    int seg_ = 1, stars_ = 4, year_ = 2026, month_ = 11, day_ = 10;
    float range_lo_ = 25, range_hi_ = 70;
    bool chips_[4]{true, false, true, false};
    // data page
    int sort_col_ = 0, table_sel_ = -1, table_page_ = 0, big_sel_ = 3;
    bool sort_asc_ = true;
    // motion page
    bool want_sheet_ = false, want_side_ = false, want_pages_ = false, form_on_ = true, touch_ = false;
    int tr_page_ = 0;
    char picked_[160] = "No file chosen";
    Runner* runner_ = nullptr;

    rd::Color brush_color_{0, 0, 0, 255};
    char brush_search_[32] = "";
    rd::TextBuffer brush_search_buf_{brush_search_, sizeof brush_search_, 0};
};

}  // namespace demo
