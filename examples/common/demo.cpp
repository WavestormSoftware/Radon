// Copyright (c) 2026 Wavestorm Software
#include "demo.hpp"

#include <SDL3/SDL.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace demo {

std::vector<uint8_t> read_file(const std::string& path) {
    std::vector<uint8_t> out;
    if (FILE* f = std::fopen(path.c_str(), "rb")) {
        std::fseek(f, 0, SEEK_END);
        long n = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        if (n > 0) {
            out.resize((size_t)n);
            if (std::fread(out.data(), 1, out.size(), f) != out.size()) out.clear();
        }
        std::fclose(f);
    }
    return out;
}

std::vector<uint8_t> find_font(const char* explicit_path) {
    std::vector<std::string> tried;
    if (explicit_path) tried.push_back(explicit_path);
    if (const char* e = std::getenv("RD_FONT")) tried.push_back(e);
    tried.push_back("examples/assets/font.ttf");
    tried.push_back("assets/font.ttf");
    if (const char* base = SDL_GetBasePath()) tried.push_back(std::string(base) + "assets/font.ttf");
    for (const auto& p : tried) {
        auto f = read_file(p);
        if (!f.empty()) {
            std::printf("radon demo: font %s\n", p.c_str());
            return f;
        }
    }
    std::fprintf(stderr,
                 "radon demo: ERROR: no font found. Put a TrueType font at examples/assets/font.ttf, set RD_FONT, or pass "
                 "--font <path>. Running without text.\n");
    return {};
}

void App::apply_theme(rd::Context& ctx) {
    rd::Theme t = dark_ ? rd::Theme::dark() : rd::Theme::light();
    t.font_body = t.font_heading = font_;
    t.font_small = small_;
    ctx.set_theme(t);
}

void App::init(rd::Context& ctx, const std::vector<uint8_t>& font, rd::TextureId checker) {
    checker_ = checker;
    if (!font.empty()) {
        font_ = ctx.load_font(font.data(), font.size(), 16);
        small_ = ctx.load_font(font.data(), font.size(), 13);
        ctx.set_default_font(font_);
    }
    apply_theme(ctx);
}

void App::frame(rd::Context& ctx, const rd::Input& in, int w, int h) {
    using namespace rd;
    const float sl = in.safe_area.x, st = in.safe_area.y, sr = in.safe_area.w, sb = in.safe_area.h;
    if (!laid_out_ || w != last_w_ || h != last_h_) {
        if (w < 700) {  // phone-sized: one full-width window inside the safe area
            main_ = {sl + 8, st + 8, (float)w - sl - sr - 16, (float)h - st - sb - 16};
            stats_ = {-1000, -1000, 240, 170};
        } else {
            main_ = {24, 24, 380, std::fmin(540.0f, (float)h - 48)};
            stats_ = {430, 24, 250, 190};
        }
        laid_out_ = true;
        last_w_ = w;
        last_h_ = h;
    }

    if (begin_window(ctx, "Radon demo", main_, WindowDefault | WindowScrollable)) {
        const float cols[] = {110, -1};
        layout_row(ctx, 2, cols, 2, 0);
        label(ctx, "Name");
        textbox(ctx, "##name", name_buf_);

        layout_row(ctx, 1, nullptr, 0, 0);
        if (collapsing_header(ctx, "Controls", open_misc_)) {
            layout_row(ctx, 2, cols, 2, 0);
            label(ctx, "Button");
            if (button(ctx, "Click me").clicked) counter_++;
            tooltip(ctx, "Buttons report clicked, pressed and hovered");
            label(ctx, "Clicks");
            std::snprintf(status_, sizeof status_, "%d", counter_);
            label(ctx, status_);
            label(ctx, "Checkbox");
            checkbox(ctx, "Enabled", check_);
            label(ctx, "Slider");
            slider(ctx, "##volume", volume_, 0.0f, 1.0f);
            label(ctx, "Drag");
            drag_float(ctx, "##speed", speed_, 0.05f);
            label(ctx, "Progress");
            progress_bar(ctx, volume_);
            label(ctx, "Theme");
            static const char* const themes[] = {"Dark", "Light"};
            int sel = dark_ ? 0 : 1;
            if (combo(ctx, "##theme", sel, themes, 2)) {
                dark_ = sel == 0;
                apply_theme(ctx);
            }
            label(ctx, "Popup");
            if (button(ctx, "Open popup").clicked) open_popup(ctx, "demo_popup");
            if (popup_begin(ctx, "demo_popup")) {
                layout_row(ctx, 1, nullptr, 0, 0);
                label(ctx, "A popup");
                if (button(ctx, "Close").clicked) close_popup(ctx);
                popup_end(ctx);
            }
        }
        layout_row(ctx, 1, nullptr, 0, 0);
        if (collapsing_header(ctx, "Text and images", open_text_)) {
            layout_row(ctx, 1, nullptr, 0, 0);
            label_wrapped(ctx,
                          "Radon is an immediate-mode UI library. Widgets are called every frame and return what "
                          "happened. Tab moves focus, Enter and Space activate.");
            if (checker_) {
                layout_row(ctx, 1, nullptr, 0, 0);
                image(ctx, checker_, {96, 96});
            }
        }
        end_window(ctx);
    }

    if (begin_window(ctx, "Stats", stats_, WindowDefault | WindowClosable)) {
        const DrawList& dl = ctx.draw_list();
        char b[96];
        layout_row(ctx, 1, nullptr, 0, 0);
        std::snprintf(b, sizeof b, "vertices %d / commands %d", dl.vertex_count(), dl.command_count());
        label(ctx, b);
        std::snprintf(b, sizeof b, "scale %.2f  error %s", (double)ctx.dpi_scale(), ctx.error() ? "yes" : "no");
        label(ctx, b);
        std::snprintf(b, sizeof b, "pointer %.0f, %.0f%s", (double)in.pointer.x, (double)in.pointer.y,
                      in.touch ? " (touch)" : "");
        label(ctx, b);
        if (button(ctx, "Reset counter").clicked) counter_ = 0;
        end_window(ctx);
    }
}

}  // namespace demo
