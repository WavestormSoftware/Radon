// Copyright (c) 2026 Wavestorm Software
// Live plots: line chart, histogram and a stepper for the signal frequency.
#include <SDL3/SDL_main.h>

#include <cmath>

#include "runner.hpp"

int main(int argc, char** argv) {
    static rd::Rect win{20, 20, 560, 420};
    static float samples[96] = {}, t = 0, freq = 1.0f;
    static bool running = true;
    demo::AppConfig cfg;
    cfg.title = "Live plots";
    cfg.width = 620, cfg.height = 480;
    cfg.frame = [](rd::Context& ui, const rd::Input&, int, int) {
        using namespace rd;
        if (running) {  // shift the buffer and append a new sample every frame
            t += 1.0f / 60;
            for (int i = 0; i + 1 < 96; i++) samples[i] = samples[i + 1];
            samples[95] = 0.5f + 0.35f * std::sin(t * 6.283f * freq) + 0.1f * std::sin(t * 31.0f);
        }
        if (begin_window(ui, "Signal", win)) {
            const float cols[] = {110, -1};
            layout_row(ui, 2, cols, 2, 0);
            label(ui, "Frequency");
            stepper(ui, "##freq", freq, 0.25f, 0.25f, 4.0f, "%.2f Hz");
            label(ui, "Running");
            toggle(ui, "##run", running);
            separator_text(ui, "Line");
            layout_row(ui, 1, nullptr, 0, 0);
            plot_lines(ui, samples, 96, 0, 1, 110);
            separator_text(ui, "Histogram (last 32)");
            layout_row(ui, 1, nullptr, 0, 0);
            plot_histogram(ui, samples + 64, 32, 0, 1, 90);
            separator_text(ui, "Status");
            layout_row(ui, 2, nullptr, 0, 0);
            if (running)
                spinner(ui);
            else
                label(ui, "paused");
            char b[32];
            std::snprintf(b, sizeof b, "latest %.2f", (double)samples[95]);
            label(ui, b);
            end_window(ui);
        }
    };
    return demo::run(argc, argv, cfg);
}
