// Copyright (c) 2026 Wavestorm Software
// Showcase apps: four small products, each with its own theme and typefaces, built only from Radon
// widgets and Canvas drawing.
#pragma once
#include <radon/radon.hpp>

#include <string_view>

namespace showcase {

/// Loads examples/showcase/fonts/<name>.ttf at `px` (cached). Returns `fallback` when the file is missing.
rd::FontHandle font(rd::Context& ui, const char* name, float px, rd::FontHandle fallback);

/// 0xRRGGBB (+ alpha) to a Color.
constexpr rd::Color hex(uint32_t rgb, uint8_t a = 255) {
    return {(uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb, a};
}
rd::Color mix(rd::Color a, rd::Color b, float t);

/// A label in another font and color (the type scale of each app is built from this).
void text(rd::Context& ui, std::string_view s, rd::FontHandle f, rd::Color c);

/// Each app: `setup` loads its fonts and returns its theme; `frame` draws one frame at w x h.
struct App {
    rd::Theme (*setup)(rd::Context& ui, rd::FontHandle fallback, bool dark);
    void (*frame)(rd::Context& ui, const rd::Input& in, int w, int h);
    const char* name;
};
extern const App tilekit, halcyon, stride_today, stride_plan, liveops;

}  // namespace showcase
