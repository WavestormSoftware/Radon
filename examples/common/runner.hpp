// Copyright (c) 2026 Wavestorm Software
// A tiny application runner shared by every example: window, GL, fonts, frame loop, screenshots.
#pragma once
#include <radon/radon.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace demo {

/// Reads a whole file (also Android assets and iOS bundle resources); empty on failure.
std::vector<uint8_t> read_file(const std::string& path);
/// Finds the demo font: explicit path, $RD_FONT, examples/assets/font.ttf, next to the executable.
/// Prints a clear error and returns an empty vector when none is found.
std::vector<uint8_t> find_font(const char* explicit_path);

/// Fonts the runner loaded into the context. Handles are -1 when no font was found.
struct Fonts {
    rd::FontHandle body = -1, heading = -1, small = -1;
};
/// Wavestorm dark (or light) theme with the loaded fonts applied.
rd::Theme make_theme(const Fonts& fonts, bool dark = true);

struct Options {
    const char* font = nullptr;
    const char* screenshot = nullptr;  ///< --screenshot out.png: render --frames frames, save, exit
    int frames = 0;
    int page = 0;                          ///< --page n: first page of multi-page examples
    bool light = false;                    ///< --light
    int width = 0, height = 0;             ///< --size WxH
    float pointer_x = -1, pointer_y = -1;  ///< --pointer x,y: fixed pointer position (screenshots)
};

/// Called every frame between begin_frame and end_frame. Sizes are logical pixels.
using FrameFn = std::function<void(rd::Context&, const rd::Input&, int width, int height)>;
using InitFn = std::function<void(rd::Context&, const Fonts&, const Options&)>;

struct AppConfig {
    const char* title = "Radon";
    int width = 1100, height = 700;
    InitFn init;  ///< optional, after fonts are loaded
    FrameFn frame;
};

/// Window + GL + context + loop. Use run() on desktop, or start()/step()/stop() when something else
/// owns the loop (Emscripten, mobile).
class Runner {
public:
    Runner();
    ~Runner();
    bool start(int argc, char** argv, const AppConfig& cfg);
    /// One frame. Returns false when the app should exit.
    bool step();
    void stop();
    rd::Context& context();
    const Fonts& fonts() const;
    const Options& options() const;
    /// Registers an RGBA texture usable with rd::image. Valid after start().
    rd::TextureId create_texture(int w, int h, const uint8_t* rgba);

private:
    struct Impl;
    std::unique_ptr<Impl> m_;
    friend int run(int, char**, const AppConfig&);
};

/// Procedural RGBA test images for rd::image demos.
std::vector<uint8_t> checker_rgba(int size, rd::Color a, rd::Color b);
std::vector<uint8_t> art_rgba(int width, int height);

/// start + loop until quit + stop. Returns the process exit code.
int run(int argc, char** argv, const AppConfig& cfg);

}  // namespace demo
