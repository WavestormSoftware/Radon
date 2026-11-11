# API reference

All functions live in namespace `rd`; include `<radon/radon.hpp>`. One `Context` per thread; nothing is
thread-safe. For tutorials see [getting-started.md](getting-started.md) and [widgets.md](widgets.md).

## Context

| Call | Meaning |
|---|---|
| `Context(const Config&)` | Allocates every pool. Nothing allocates after this. |
| `begin_frame(input, dt)` / `end_frame()` | Frame lifecycle. `end_frame` sorts the draw list by window z-order. |
| `draw_list()` | Vertices, indices and clipped commands for the backend. |
| `error()` | True if any pool overflowed (or begin/end was unbalanced) this frame. The frame still completes. |
| `warning()` | True if the glyph atlas filled; it is cleared and refills next frame. |
| `set_theme(t)` / `theme()` | Replace or read the theme. |
| `set_dpi_scale(s)` / `dpi_scale()` | Physical pixels per logical pixel. Changing it clears the atlas. |
| `load_font(data, size, px)` | TrueType from memory (buffer must outlive the context). Returns -1 on failure. |
| `set_default_font(f)` | Font used when a theme font is -1 (the first loaded font by default). |
| `set_clipboard_callbacks(get, set, user)` | `int get(user, out, cap)` returns bytes copied; `void set(user, text, len)`. |
| `atlas_pixels()` `atlas_size()` `atlas_version()` | 8-bit glyph atlas for backends; re-upload when the version changes. |
| `wants_text_input()` | True while a text box has focus (show the soft keyboard). |
| `pointer_over_ui()` | True if the pointer is over a window or popup (do not also use the click). |
| `text_input_rect()` | The focused text box in logical px, for the IME / soft keyboard. |
| `cursor()` | `Cursor::{Arrow,Text,Hand,ResizeEW,ResizeNS,ResizeNWSE}` the last frame asked for. |
| `animating()` | True while a transition or input is in progress; when false the host may sleep. |
| `can_dismiss()` | True when Escape / Android back would close a popup, dialog, drawer or text focus. |
| `set_animations(on)` | Turn transitions and scroll momentum on or off at runtime. |

`hash_id(seed, string)` is the 32-bit FNV-1a hash used for IDs.

## Config

`max_windows` (32), `max_id_stack` (32), `max_layout_stack` (64), `max_draw_commands` (4096),
`max_vertices` (65536), `max_indices` (196608), `max_textboxes` (16), `max_children` (32), `max_fonts` (4),
`max_glyphs` (4096), `max_anims` (256), `atlas_size` (1024), `dpi_scale`, `animations`, `theme`.
A full pool skips the work, sets `error()` and keeps running.

## Input

| Field | |
|---|---|
| `pointer`, `pointer_down`, `pointer_pressed`, `pointer_released` | Logical pixels, top-left origin. |
| `secondary_pressed` | Right mouse button went down this frame (context menus). |
| `touch` | The pointer is a finger: no hover, drag scrolls with momentum, 8 px tap slop, long-press = secondary click. |
| `wheel` | Lines; positive scrolls up. |
| `keys` | `KeyEvent{key, shift, ctrl, alt}`, 16 per frame. Keys: Enter, Backspace, Delete, Left, Right, Up, Down, Home, End, Tab, Escape, Space, A, C, V, X. |
| `text` | UTF-8 bytes typed this frame (256 max). |
| `screen_size` | Window size in logical px; used to center modals. |
| `safe_area` | Mobile insets: x=left, y=top, w=right, h=bottom. Zero on desktop. `safe_rect()` is the screen minus the insets. |

`Response`: `hovered`, `focused`, `pressed` (became active), `clicked` (released over it), `changed`.

## IDs

`push_id(ctx, s)` / `pop_id(ctx)` scope IDs. Text after `##` in a label is hashed but not displayed.

## Windows

| Call | |
|---|---|
| `begin_window(ctx, title, Rect&, flags)` / `end_window(ctx)` | Returns false if closed or the pool is full. |
| `open_window(ctx, title)` | Reopen after the close button. |
| Flags | `WindowMovable`, `WindowResizable` (min 80x60), `WindowClosable`, `WindowScrollable`, `WindowTitleBar`, `WindowBorder`; `WindowDefault` = movable, resizable, title bar, border. |

## Layout

| Call | |
|---|---|
| `layout_row(ctx, columns, widths, count, height)` | See [layout.md](layout.md). |
| `layout_next(ctx)` | Next cell as a `Rect`. |
| `layout_begin_column` / `layout_end_column` | Nested vertical layout. |
| `spacing(ctx, px)` `separator(ctx)` `separator_text(ctx, text)` | Gaps and rules. |
| `indent(ctx, px=-1)` `unindent(ctx, px=-1)` | Negative = `theme.indent`. |
| `begin_child(ctx, id, height, border=true)` / `end_child(ctx)` | Scrolling region taking a full row. |

## Widgets

Input: `button` (optionally `ButtonStyle::{Default,Primary,Ghost,Danger}`), `icon_button` (optionally
selected), `checkbox`, `radio_button`, `toggle`, `slider`, `slider_int`, `drag_float`, `drag_int`, `stepper`,
`stepper_int`, `textbox` (with hint), `combo`, `tab_bar`, `list_item`, `listbox`, `color_picker`,
`color_button`, `collapsing_header`, `tree_node`/`tree_pop`.

Display: `label`, `label_colored`, `label_wrapped`, `bullet_text`, `progress_bar`, `image`, `badge`,
`spinner`, `plot_lines`, `plot_histogram`, `draw_icon`, `tooltip`.

Overlays: `popup_begin`/`popup_end`/`open_popup`/`close_popup`, `begin_menu_bar`/`end_menu_bar`,
`begin_menu`/`end_menu`/`menu_item`, `open_modal`/`close_modal`/`modal_begin`/`modal_end`.

More input: `password_box`, `search_box`, `text_area`, `range_slider`, `segmented`, `chip`, `rating`,
`link`, `splitter`, `calendar`, `date_picker`, `pagination`, `nav_bar`, `breadcrumbs`.

More display: `heading`, `avatar`, `alert`, `progress_ring`, `skeleton`, `toast` / `set_toast_offset`.

Tables and lists: `table_header`, `table_row`, `list_clipper_begin` / `list_clipper_end`.

More containers: `begin_card` / `end_card`, `drawer_begin` / `drawer_end` (opened with `open_modal`),
`context_menu_begin`, `secondary_clicked`.

State and motion: `begin_disabled` / `end_disabled`, `transition_begin` / `transition_end`, `animate`.

Signatures are in `include/radon/widgets.hpp` with doc comments; usage for each is in
[widgets.md](widgets.md).

`TextBuffer{char* data; int capacity; int length}` is a non-owning view; `capacity` includes the NUL.

## Text

`measure_text(ctx, font, text)` returns `{width, line height}` in logical px.
`wrap_text(ctx, font, text, max_width, TextLine* out, max_lines)` breaks at spaces, then inside long words
at code point boundaries, and at `\n`. `utf8_decode(p, end, cp)` returns the byte length; bad input decodes to
U+FFFD and always advances.

## Theme

See [theming.md](theming.md): `Theme::dark()`, `light()`, `midnight()`, `with_accent()`, `touch()`,
`scaled()`, `push_theme` / `pop_theme`.

## DrawList

For backends: `vertices()`, `indices()`, `commands()` (with counts), `overflow()`. Emitters used by widgets
and available for new ones: `rect`, `shadow`, `glyph`, `image`, `line`, `triangle`, `gradient_rect`,
`set_clip`, `set_layer`, `set_alpha` (fades everything drawn afterwards). See [backends.md](backends.md).

## Memory

Everything is allocated in the constructor, including the glyph atlas and a 1 MB scratch arena that
stb_truetype uses while rasterizing glyphs. Frames after warm-up allocate nothing (`tests/test_alloc.cpp`
runs the whole widget set and counts allocations).
