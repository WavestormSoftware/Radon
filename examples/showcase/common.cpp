// Copyright (c) 2026 Wavestorm Software
#include <SDL3/SDL.h>

#include <memory>
#include <string>
#include <vector>

#include "runner.hpp"
#include "showcase.hpp"

namespace showcase {

namespace {
struct Loaded {
    std::string key;
    rd::FontHandle handle;
};
// Font bytes must outlive the context, so they live here for the whole run.
std::vector<std::unique_ptr<std::vector<uint8_t>>> g_files;
std::vector<Loaded> g_fonts;

std::vector<uint8_t>* file(const char* name) {
    std::vector<std::string> tried = {std::string("examples/showcase/fonts/") + name + ".ttf",
                                      std::string("fonts/") + name + ".ttf"};
    if (const char* base = SDL_GetBasePath()) tried.push_back(std::string(base) + "fonts/" + name + ".ttf");
    for (const auto& p : tried) {
        auto bytes = demo::read_file(p);
        if (!bytes.empty()) {
            g_files.push_back(std::make_unique<std::vector<uint8_t>>(std::move(bytes)));
            return g_files.back().get();
        }
    }
    return nullptr;
}
}  // namespace

rd::FontHandle font(rd::Context& ui, const char* name, float px, rd::FontHandle fallback) {
    const std::string key = std::string(name) + "@" + std::to_string(px);
    for (const auto& f : g_fonts)
        if (f.key == key) return f.handle;
    static std::vector<std::pair<std::string, std::vector<uint8_t>*>> files;
    std::vector<uint8_t>* bytes = nullptr;
    for (auto& f : files)
        if (f.first == name) bytes = f.second;
    if (!bytes && (bytes = file(name))) files.emplace_back(name, bytes);
    rd::FontHandle h = bytes ? ui.load_font(bytes->data(), bytes->size(), px) : -1;
    if (h < 0) h = fallback;
    g_fonts.push_back({key, h});
    return h;
}

rd::Color mix(rd::Color a, rd::Color b, float t) {
    auto m = [t](uint8_t x, uint8_t y) { return (uint8_t)((float)x + ((float)y - (float)x) * t + 0.5f); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}

void text(rd::Context& ui, std::string_view s, rd::FontHandle f, rd::Color c) {
    rd::Theme t = ui.theme();
    t.font_body = f;
    t.text = c;
    rd::push_theme(ui, t);
    rd::label(ui, s);
    rd::pop_theme(ui);
}

}  // namespace showcase
