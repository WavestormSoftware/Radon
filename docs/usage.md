# Radon usage

All functions live in namespace `rd`; include `<radon/radon.hpp>`. One `Context` per thread; nothing is
thread-safe.

## Frame loop

```cpp
rd::Context ui;                    // allocates every pool once, from rd::Config
rd::Input input;                   // the backend fills this each frame
ui.begin_frame(input, dt_seconds);
/* widgets */
ui.end_frame();
const rd::DrawList& dl = ui.draw_list();   // render it; the core never draws
```

**Call order matters.** As in microui, a widget's result for a frame depends on the widgets called
before it. Hover is computed from the *previous* frame's window rectangles, so a new window reacts to
the pointer from its second frame. Never call `end_window` unless `begin_window` returned true.

## Context

| Call | Meaning |
|---|---|
| `Context(const Config&)` | Allocates all pools. Nothing allocates after this (see "Memory"). |
| `begin_frame(input, dt)` / `end_frame()` | Frame lifecycle. `end_frame` sorts the draw list by window z-order. |
| `draw_list()` | Vertices, indices and clipped commands for the backend. |
| `error()` | True if any pool overflowed this frame. The frame still completes. |
| `warning()` | True if the glyph atlas filled; it is cleared and refills next frame. |
| `set_theme(theme)` / `theme()` | Replace or read the theme. |
| `set_dpi_scale(s)` / `dpi_scale()` | Physical pixels per logical pixel. Changing it clears the atlas. |
| `load_font(data, size, pixel_size)` | Loads TrueType from memory (the buffer must outlive the context). Returns -1 on failure. |
| `set_default_font(f)` | Font used when a theme font is -1. The first loaded font is the default. |
| `set_clipboard_callbacks(get, set, user)` | `get(user, out, cap)` returns bytes copied; `set(user, text, len)`. |
| `atlas_pixels()`, `atlas_size()`, `atlas_version()` | 8-bit glyph atlas for the backend; re-upload when the version changes. |
| `wants_text_input()` | True while a textbox has focus: show the soft keyboard. |
| `pointer_over_ui()` | True if the pointer is over a window or popup; the host should ignore that click. |

## Config

`max_windows`, `max_id_stack`, `max_layout_stack`, `max_draw_commands`, `max_vertices`,
`max_indices`, `max_textboxes`, `max_fonts`, `max_glyphs`, `max_anims`, `atlas_size`, `dpi_scale`,
`animations`, `theme`. When a pool is full the work is skipped, `error()` turns true, and the frame keeps
running.

## Input

`pointer` (logical px), `pointer_down/pressed/released`, `touch` (the pointer is a finger: no hover,
drag scrolls), `wheel` (lines, positive = scroll up), `keys` (`KeyEvent{key, shift, ctrl, alt}`, 16
per frame), `text` (UTF-8 bytes, 256 per frame), `safe_area` (insets: x=left, y=top, w=right, h=bottom).
Keys: Enter, Backspace, Delete, Left/Right/Up/Down, Home, End, Tab, Escape, Space, A, C, V, X.

## Response

`hovered`, `focused`, `pressed` (became active this frame), `clicked` (released over the same widget),
`changed` (a value widget changed its value).

## IDs

An ID is the 32-bit FNV-1a hash of the ID-stack top plus the label. Text after `##` is hashed but not
shown: `"Delete##row3"`. `push_id(ctx, s)` / `pop_id(ctx)` scope IDs (for example, per list row).
`hash_id(seed, s)` exposes the hash.

## Windows and layout

```cpp
bool begin_window(ctx, title, Rect& rect, flags = WindowDefault);   // rect is updated by move/resize
void end_window(ctx);
void open_window(ctx, title);        // reopen a window closed with its close button
```
Flags: `WindowMovable`, `WindowResizable` (minimum 80x60), `WindowClosable`, `WindowScrollable`,
`WindowTitleBar`, `WindowBorder`; `WindowDefault` = movable | resizable | title bar | border. Clicking a
window raises it. Scrolling works with the wheel, touch drag, and the scrollbar thumb.

```cpp
void layout_row(ctx, int columns, const float* widths, int count, float height);
Rect layout_next(ctx);               // the next cell, for custom drawing
void layout_begin_column(ctx);  void layout_end_column(ctx);
void spacing(ctx, float px);    void separator(ctx);
```
A positive width is pixels; a negative width fills the remaining space (-1 fills exactly, -2 leaves 1 px).
Cell `i` uses `widths[min(i, count-1)]`; a null `widths` splits the row evenly. Height 0 is the theme cell
height; negative fills the remaining height. Cells are separated by `theme.spacing`.

## Widgets

| Call | Notes |
|---|---|
| `button(ctx, label)` | `clicked`; Enter activates when focused. |
| `checkbox(ctx, label, bool&)` | `changed`; Space toggles when focused. |
| `slider(ctx, label, float&, min, max, fmt)` | `changed`; Left/Right step 5% when focused. |
| `drag_float(ctx, label, float&, speed)` | Drag horizontally; `changed`. |
| `textbox(ctx, label, TextBuffer&)` | Edits in place. `changed` on edit, `clicked` on Enter, Escape drops focus. |
| `label(ctx, text)` / `label_wrapped(ctx, text)` | Wrapped label grows its row to fit (up to 32 lines). |
| `progress_bar(ctx, fraction)` | 0..1. |
| `image(ctx, TextureId, size, uv)` | Texture 0 is the glyph atlas; create others through the backend. |
| `collapsing_header(ctx, label, bool& open)` | Returns the open state. |
| `combo(ctx, label, int& selected, items, count)` | The label is only the ID. Returns true when the selection changed. |
| `tooltip(ctx, text)` | Call right after a widget; shown while it is hovered (not on touch). |
| `popup_begin/popup_end/open_popup/close_popup` | One popup at a time, anchored under the last widget; closes on outside click or Escape. |

`TextBuffer{char* data; int capacity; int length}` is a non-owning view; `capacity` includes the NUL.
Selection, Shift+arrows, Home/End, Ctrl+A/C/X/V (through the clipboard callbacks), a 500 ms caret blink
and UTF-8-aware editing are built in. Tab / Shift+Tab move focus within the focused window.

## Text

`measure_text(ctx, font, text)` returns `{width, line height}` in logical px. `wrap_text(ctx, font, text,
max_width, TextLine* out, max_lines)` breaks at spaces, then inside long words at code point boundaries,
and always at `\n`. `utf8_decode` is exposed; bad input decodes to U+FFFD and always advances.

## Theme

`Theme::dark()` (background #0a0a0a, accent #4da3ff) and `Theme::light()`. Fields: colors (`background`,
`surface`, `surface_hover`, `surface_active`, `text`, `text_muted`, `accent`, `accent_text`, `border`,
`shadow`, `error`, `focus_ring`), metrics (`padding`, `spacing`, `corner_radius`, `border_width`,
`scrollbar_width`, `title_height`, `cell_height`) and fonts (`font_body`, `font_heading`, `font_small`;
a font handle already carries its pixel size). No widget hard-codes a color, size or spacing.

## Memory

Everything is allocated in the constructor, including the glyph atlas and a 1 MB scratch arena that
stb_truetype uses while rasterizing glyphs. Frames after warm-up allocate nothing (`tests/test_alloc.cpp`).

## Backend (`radon::sdl3_gl`)

`rd::sdl3::create_platform`, `poll`, `finish_frame`, `attach` (window, input, DPI, clipboard, soft
keyboard, safe area) and `rd::gl::create_renderer`, `clear`, `render`, `create_texture`, `read_pixels`.
See `examples/desktop_demo/main.cpp` for the whole loop. To use another renderer, read
`Context::draw_list()` and `atlas_pixels()` yourself; the vertex format is documented in `draw_list.hpp`.
