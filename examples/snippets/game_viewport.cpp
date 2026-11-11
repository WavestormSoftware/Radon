// Copyright (c) 2026 Wavestorm Software
// An engine view inside a Radon panel: render into an offscreen target, show it with rd::viewport.
// The scene here is just a clear color that follows the mouse; put your own GL (or engine) calls
// between begin_target and end_target.
#include <SDL3/SDL_main.h>

#include <cmath>
#include <cstdio>

#include <radon_sdl3_gl.hpp>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect win{40, 40, 440, 380};
    static demo::Runner* run = nullptr;
    static rd::TextureId target = 0;
    static float mx = 0.5f, my = 0.5f, t = 0;
    static const int kW = 400, kH = 260;
    demo::AppConfig cfg;
    cfg.title = "Game viewport";
    cfg.width = 540, cfg.height = 460;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        rd::gl::Renderer* r = run->renderer();
        if (!target) target = rd::gl::create_render_target(r, kW, kH, true);
        t += 0.016f;
        rd::ViewportResponse v{};
        if (rd::begin_window(ui, "Viewport", win)) {
            v = rd::viewport(ui, target, {(float)kW, (float)kH}, true);
            if (v.down) {
                mx = v.local.x / v.rect.w;
                my = v.local.y / v.rect.h;
            }
            char text[96];
            std::snprintf(text, sizeof text, "%s  pointer %.0f, %.0f", v.focused ? "focused" : "click to focus",
                          v.local.x, v.local.y);
            rd::label(ui, text);
            rd::end_window(ui);
        }
        const auto c = [](float f) { return (uint8_t)(255.0f * (0.5f + 0.5f * f)); };
        rd::gl::begin_target(r, target, {c(mx), c(my), c(std::sin(t)), 255});
        rd::gl::end_target(r);
    };
    demo::Runner r;
    run = &r;
    if (!r.start(argc, argv, cfg)) return 1;
    while (r.step()) {
    }
    r.stop();
    return 0;
}
