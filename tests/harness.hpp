// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <ostream>  // MSVC: doctest streams string_view and needs the full ostream
#include <doctest.h>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include <radon/radon.hpp>
#include "internal.hpp"

namespace th {

/// Loads a TTF for text tests: $RD_TEST_FONT, examples/assets/font.ttf, then common system fonts.
inline const std::vector<uint8_t>& font_bytes() {
    static std::vector<uint8_t> bytes;
    static bool tried = false;
    if (tried) return bytes;
    tried = true;
    std::vector<std::string> paths;
    if (const char* e = std::getenv("RD_TEST_FONT")) paths.push_back(e);
    paths.push_back(std::string(RD_SOURCE_DIR) + "/examples/assets/font.ttf");
    for (const char* p : {"/usr/share/fonts/noto/NotoSans-Regular.ttf", "/usr/share/fonts/TTF/DejaVuSans.ttf",
                          "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "C:/Windows/Fonts/arial.ttf",
                          "/System/Library/Fonts/Supplemental/Arial.ttf"})
        paths.push_back(p);
    for (auto& p : paths) {
        if (FILE* f = std::fopen(p.c_str(), "rb")) {
            std::fseek(f, 0, SEEK_END);
            long n = std::ftell(f);
            std::fseek(f, 0, SEEK_SET);
            bytes.resize((size_t)n);
            size_t got = std::fread(bytes.data(), 1, bytes.size(), f);
            std::fclose(f);
            if (got == bytes.size() && n > 0) return bytes;
            bytes.clear();
        }
    }
    if (std::getenv("RD_REQUIRE_TEST_FONT")) {  // CI sets this so text tests cannot be skipped silently
        std::fprintf(stderr, "no test font found; set RD_TEST_FONT to a .ttf file\n");
        std::abort();
    }
    return bytes;
}

inline rd::detail::State& state(rd::Context& c) {
    return rd::detail::state(c);
}
/// Scroll offset of the root-level window called `title`.
inline float window_scroll(rd::Context& c, const char* title) {
    auto& s = state(c);
    const rd::ID id = rd::hash_id(0, title);
    for (int i = 0; i < s.cfg.max_windows; i++)
        if (s.wins[i].used && s.wins[i].id == id) return s.wins[i].scroll;
    return 0;
}

struct Harness {
    rd::Context ctx;
    rd::Input in;
    explicit Harness(const rd::Config& cfg = {}) : ctx(cfg) { in.pointer = {-100, -100}; }

    void frame(const std::function<void()>& ui) {
        ctx.begin_frame(in, 1.0f / 60);
        ui();
        ctx.end_frame();
        in.pointer_pressed = in.pointer_released = false;
        in.keys.clear();
        in.text.clear();
        in.wheel = 0;
    }
    void move(float x, float y) { in.pointer = {x, y}; }
    void press(const std::function<void()>& ui) {
        in.pointer_down = in.pointer_pressed = true;
        frame(ui);
    }
    void release(const std::function<void()>& ui) {
        in.pointer_down = false;
        in.pointer_released = true;
        frame(ui);
    }
    /// Moves, waits one frame (hover needs the previous frame's windows), presses and releases.
    void click(float x, float y, const std::function<void()>& ui) {
        move(x, y);
        frame(ui);
        press(ui);
        release(ui);
    }
    void key(int k, bool shift = false, bool ctrl = false) { in.keys.push({k, shift, ctrl, false}); }
    void type(const char* s) {
        for (; *s; s++) in.text.push(*s);
    }
};

}  // namespace th
