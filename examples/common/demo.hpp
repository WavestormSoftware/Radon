// Copyright (c) 2026 Wavestorm Software
// The demo UI shared by the desktop, web and mobile entry points.
#pragma once
#include <radon/radon.hpp>
#include <string>
#include <vector>

namespace demo {

/// Reads a whole file; empty on failure.
std::vector<uint8_t> read_file(const std::string& path);

/// Looks for the demo font: `explicit_path`, $RD_FONT, ./examples/assets/font.ttf, <exe>/assets/font.ttf.
/// Prints a clear error and returns an empty vector when none is found.
std::vector<uint8_t> find_font(const char* explicit_path);

class App {
public:
    /// `font` must outlive the context. An empty font runs the demo without text.
    void init(rd::Context& ctx, const std::vector<uint8_t>& font, rd::TextureId checker);
    void frame(rd::Context& ctx, const rd::Input& in, int logical_w, int logical_h);
    bool dark() const { return dark_; }

private:
    void apply_theme(rd::Context& ctx);

    rd::FontHandle font_ = -1, small_ = -1;
    bool dark_ = true, laid_out_ = false, check_ = true, open_text_ = true, open_misc_ = true;
    float volume_ = 0.35f, speed_ = 2.0f, progress_ = 0.0f;
    int counter_ = 0, combo_ = 0, last_w_ = 0, last_h_ = 0;
    rd::TextureId checker_ = 0;
    rd::Rect main_{24, 24, 380, 520}, stats_{430, 24, 240, 170};
    char name_[48] = "Radon";
    rd::TextBuffer name_buf_{name_, sizeof name_, 5};
    char status_[64] = "";
};

}  // namespace demo
