# Getting started

## 1. The mental model

Radon is **immediate mode**. You do not create widgets and keep them around. Every frame you describe the
UI by calling functions, and each call returns what happened:

```cpp
if (rd::button(ui, "Save").clicked) save();      // runs the frame the button is released
rd::checkbox(ui, "Dark mode", settings.dark);     // reads and writes your own variable
```

Your data stays yours. Radon keeps only small bits of interaction state (which widget is hovered, focused
or active, window positions, text cursors) keyed by widget **IDs**.

A frame looks like this:

```cpp
ui.begin_frame(input, dt_seconds);   // 1. give Radon the input
/* windows and widgets */            // 2. describe the UI
ui.end_frame();                      // 3. finish
render(ui.draw_list());              // 4. draw the result with any renderer
```

`rd::Input` is plain data (pointer, buttons, wheel, key events, text, safe area) that your platform layer
fills once per frame. The `radon::sdl3_gl` backend does that for you.

## 2. Add Radon to your project

With CMake:

```cmake
add_subdirectory(Radon)                      # or FetchContent
target_link_libraries(myapp PRIVATE radon::radon)        # the core only
target_link_libraries(myapp PRIVATE radon::sdl3_gl)      # core + SDL3 window/input/renderer
```

`radon::radon` needs only a C++17 compiler. `radon::sdl3_gl` needs SDL3 (an installed copy is used if
found, otherwise CMake fetches release 3.2.8).

## 3. A complete SDL3 app

```cpp
#include <SDL3/SDL_main.h>
#include <radon/radon.hpp>
#include "radon_sdl3_gl.hpp"

int main() {
    rd::sdl3::Platform* plat = rd::sdl3::create_platform({});          // window + GL context
    rd::gl::Renderer* rend = rd::gl::create_renderer({rd::sdl3::is_gles(plat)});

    rd::Context ui;
    rd::sdl3::attach(plat, ui);                                        // DPI + clipboard
    // load a TrueType file into memory (the buffer must outlive `ui`):
    //   rd::FontHandle f = ui.load_font(bytes.data(), bytes.size(), 16);
    //   rd::Theme t = rd::Theme::dark(); t.font_body = t.font_heading = f; ui.set_theme(t);

    rd::Input input;
    float dt = 0;
    while (rd::sdl3::poll(plat, input, dt)) {                          // false when the window closes
        int fw, fh;
        rd::sdl3::framebuffer_size(plat, fw, fh);
        input.screen_size = {fw / ui.dpi_scale(), fh / ui.dpi_scale()}; // only needed for modals

        ui.begin_frame(input, dt);
        static rd::Rect win{40, 40, 300, 160};
        if (rd::begin_window(ui, "Hello", win)) {
            rd::layout_row(ui, 1, nullptr, 0, 0);
            rd::label(ui, "Hello, Radon");
            rd::end_window(ui);
        }
        ui.end_frame();

        rd::gl::clear(rend, ui.theme().background);
        rd::gl::render(rend, ui, fw, fh);
        rd::sdl3::finish_frame(plat, ui);                              // soft keyboard + swap
    }
    rd::gl::destroy_renderer(rend);
    rd::sdl3::destroy_platform(plat);
}
```

`examples/snippets/01_hello.cpp` is the same thing using the example runner (about 20 lines).

## 4. The rules that matter

1. **Call `end_window` only if `begin_window` returned true** (same for `begin_child`, `popup_begin`,
   `begin_menu`, `begin_menu_bar`, `modal_begin`).
2. **Every widget takes a cell from the current layout.** Start rows with `layout_row`; see
   [layout.md](layout.md). The default row is one fill-width column of the theme's cell height.
3. **IDs come from labels.** Two widgets with the same label in the same scope clash. Use `"Delete##row3"`
   (text after `##` is hashed but not shown) or `push_id`/`pop_id` around loops.
4. **Order matters.** A widget's result depends on widgets called before it in the frame. Hover is based on
   the previous frame's window rectangles, so a brand-new window reacts to the pointer from its second frame.
5. **Fonts are bytes.** `load_font` takes a memory buffer, so the same code runs on web and mobile where
   there is no file system. Without a font the UI still runs (no text).
6. **Pools are fixed.** If you create more windows, text boxes or draw commands than `rd::Config` allows,
   the extras are skipped and `ui.error()` is true for that frame. Raise the limits in `Config`.

## 5. Handling input yourself

If you do not use the SDL backend, fill `rd::Input` yourself each frame:

```cpp
rd::Input in;
in.pointer = {mouse_x, mouse_y};         // logical pixels, top-left origin
in.pointer_down = left_button_is_down;
in.pointer_pressed = left_button_went_down_this_frame;
in.pointer_released = left_button_went_up_this_frame;
in.wheel = wheel_lines;                  // positive = scroll up
in.keys.push({rd::KeyBackspace, shift, ctrl, alt});   // key presses that happened this frame
for (char c : utf8_text_typed) in.text.push(c);       // UTF-8 bytes of typed text
in.touch = last_event_was_a_finger;      // no hover; drags scroll; taps click
```

Ask the context what the UI wants: `ui.wants_text_input()` (show the soft keyboard) and
`ui.pointer_over_ui()` (do not also treat this click as a game click).

## 6. Next steps

- Browse every widget in the [widget guide](widgets.md) or run the gallery.
- Read [theming](theming.md) to restyle everything from one struct.
- Copy from [the examples](examples.md): a settings form, a to-do list, a theme editor, a dashboard.
