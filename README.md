# Radon

A small, portable, immediate-mode UI library for C++17, inspired by
[rxi/microui](https://github.com/rxi/microui). The core has no dependencies and never draws: it
emits a draw list that a backend renders. One SDL3 + OpenGL ES 3 backend serves Windows, Linux, Web
(WebGL2), Android and iOS. MIT licensed.

## Quick start

```cpp
#include <radon/radon.hpp>

rd::Context ui;                       // all pools allocated here, once
rd::FontHandle font = ui.load_font(ttf_bytes, ttf_size, 16);   // fonts come from memory

void frame(const rd::Input& input, float dt) {
    ui.begin_frame(input, dt);
    static rd::Rect win{20, 20, 320, 200};
    if (rd::begin_window(ui, "Settings", win)) {
        static bool sound = true;
        static float volume = 0.5f;
        const float cols[] = {90, -1};
        rd::layout_row(ui, 2, cols, 2, 0);
        rd::label(ui, "Volume");
        rd::slider(ui, "##volume", volume, 0.0f, 1.0f);
        rd::checkbox(ui, "Sound", sound);
        if (rd::button(ui, "Apply").clicked) { /* ... */ }
        rd::end_window(ui);
    }
    ui.end_frame();
    // render ui.draw_list(): see backends/sdl3_gl or write your own
}
```

With CMake: `target_link_libraries(app PRIVATE radon::radon)` (core only) or add `radon::sdl3_gl`
(window, input, renderer). Build and test: `cmake --preset linux-gcc-debug && cmake --build --preset
linux-gcc-debug && ctest --preset linux-gcc-debug`.

See [docs/usage.md](docs/usage.md) for the full API, [docs/STATUS.md](docs/STATUS.md) for what has been
verified per platform, and [docs/PLAN.md](docs/PLAN.md) for the design.
