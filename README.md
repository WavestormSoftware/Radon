# Radon

A small, fast, **immediate-mode UI library for C++17**. Call widgets every frame, get results back, and
render the draw list it produces with any renderer. No retained widget tree, no heap allocation per frame,
no dependencies in the core.

![Radon gallery](docs/screenshots/gallery-widgets.png)

- **40+ widgets**: buttons (4 styles), checkbox, radio, switch, sliders, drag fields, steppers, text boxes
  with selection and clipboard, combo, tabs, list box, tree, menu bar, popups, modal dialogs, color
  picker, plots, spinner, badges, 22 vector icons, scrolling child regions.
- **Windows**: move, resize, close, scroll, z-order, shadows; mouse **and touch** share one input path.
- **Theming**: one `Theme` struct drives every color and metric. Dark (Wavestorm), light and midnight
  presets, `Theme::with_accent()`, and scoped overrides with `push_theme()`.
- **Portable**: Windows, Linux, Web (WebGL2), Android, iOS through one SDL3 + OpenGL ES 3 backend. The core
  includes no platform headers; write your own backend in a few hundred lines.
- **Predictable**: all memory is allocated up front from a `Config`. A full pool never crashes: the work is
  skipped and `ctx.error()` turns true. Verified by an allocation test (zero heap allocations per frame).

## Quick start

```cpp
#include <radon/radon.hpp>

rd::Context ui;                                  // allocates every pool once
// ui.load_font(ttf_bytes, ttf_size, 16);        // fonts come from memory

void frame(const rd::Input& input, float dt) {
    ui.begin_frame(input, dt);
    static rd::Rect win{20, 20, 320, 200};
    if (rd::begin_window(ui, "Settings", win)) {
        static bool sound = true;
        static float volume = 0.5f;
        const float cols[] = {90, -1};           // 90 px label column, then fill
        rd::layout_row(ui, 2, cols, 2, 0);
        rd::label(ui, "Volume");
        rd::slider(ui, "##volume", volume, 0.0f, 1.0f);
        rd::layout_row(ui, 1, nullptr, 0, 0);
        rd::checkbox(ui, "Sound", sound);
        if (rd::button(ui, "Apply", rd::ButtonStyle::Primary).clicked) { /* ... */ }
        rd::end_window(ui);
    }
    ui.end_frame();
    // render ui.draw_list(): see backends/sdl3_gl, or docs/backends.md to write your own
}
```

## Try it

```sh
cmake --preset linux-clang-release            # or linux-gcc-debug
cmake --build --preset linux-clang-release
./build/linux-clang-release/examples/desktop_demo/radon_desktop_demo      # the gallery
./build/linux-clang-release/examples/radon_ex_03_todo_list                 # a small example
```

The demos need a TrueType font: put one at `examples/assets/font.ttf` or run with `--font <path>` /
`RD_FONT=<path>`. Pages, themes and screenshots: `--page 0..5`, `--light`, `--screenshot out.png`.

## Documentation

| | |
|---|---|
| [Getting started](docs/getting-started.md) | Embed Radon in your app, step by step |
| [Widget guide](docs/widgets.md) | Every widget with code and pictures |
| [Layout](docs/layout.md) | Rows, columns, regions, indentation |
| [Theming](docs/theming.md) | Colors, metrics, presets, scoped overrides |
| [Examples](docs/examples.md) | Ten small apps and the gallery |
| [API reference](docs/usage.md) | Every public function |
| [Backends](docs/backends.md) | Using the SDL3 backend or writing your own renderer |
| [Building](docs/building.md) | Linux, Windows, Web, Android, iOS |
| [Status](docs/STATUS.md) | What has been built and tested on which platform |

## License

MIT. See [LICENSE](LICENSE). Vendored: stb_truetype and stb_rect_pack (public domain / MIT), doctest
(MIT) in `src/third_party/`.
