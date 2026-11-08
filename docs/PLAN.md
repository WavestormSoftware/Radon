# Radon: Implementation Plan

A small, portable, immediate-mode UI library for C++, inspired by rxi/microui.
Owner: Wavestorm Software. Repository: `WavestormSoftware/Radon` (public, MIT).

Target: **v0.1.0 ships all five platforms**: Windows, Linux, Web (Emscripten/WebGL2), Android, and iOS.

This document is the single source of truth for implementation. Build milestone by milestone in the order given (M0 to M8). Each milestone has acceptance criteria. Do not start a milestone until the previous one passes its criteria.

---

## 1. Goals and non-goals

### Goals
- Small, dependency-free core. The core library has no SDL, OpenGL, or platform headers.
- Immediate-mode API: widgets are called every frame and return interaction results. No retained widget tree.
- No heap allocation per frame in the default configuration. All pools are sized at creation time from a `Config` struct.
- The core emits a draw list (vertices, indices, clipped commands). The host or backend renders it. The core never draws.
- Modern look: rounded corners, borders, shadows, smooth hover transitions, theming from one style struct.
- Targets (all in v0.1): Windows, Linux, Web (WebGL2 via Emscripten), Android, iOS. One SDL3 platform layer and one OpenGL ES 3 renderer serve all of them.
- Pointer-based input (mouse and touch share one path), logical pixels with a DPI scale factor, and no assumption of a physical keyboard.

### Non-goals (v0.1)
- Native OS widgets.
- Docking and multi-viewport windows.
- Full text shaping (HarfBuzz is a later addition).
- Accessibility tree.
- Retained-mode scene graphs.

---

## 2. Identity

- **Name:** Radon (chemical element Rn). Fits the Wavestorm naming (Carbon, Argon, Phosphor).
- **C++ namespace:** `rd`
- **Include root:** `#include <radon/radon.hpp>`
- **CMake targets:** `radon::radon` (core), `radon::sdl3_gl` (backend)
- **Copyright header:** `Copyright (c) 2026 Wavestorm Software`
- **License:** MIT, with the license file in the repo root.
- **Option prefix:** `RD_` (for example, `RD_BUILD_TESTS`).

---

## 3. Technical decisions

| Area | Decision | Reason |
|---|---|---|
| Language | C++17. Exceptions and RTTI not required | Compiles with MSVC, GCC, Clang, Emscripten, and Android NDK |
| Core dependencies | None | Keeps the core small and portable |
| Windowing and input (backend) | SDL3, fetched with CMake `FetchContent` | One API for Windows, Linux, Web, Android, and iOS |
| Rendering | OpenGL ES 3.0 subset, shared by desktop GL 3.3 core, WebGL2, GLES3 on mobile | One shader source and one backend for all targets |
| Shape rendering | Signed-distance-field (SDF) fragment shader for rounded rectangles, borders, shadows | Smooth edges at any scale with few vertices |
| Font rasterization | stb_truetype, vendored | Single file, public domain |
| Atlas packing | stb_rect_pack, vendored | Single file, public domain |
| Font input | Fonts loaded from memory buffers | Works on web and mobile without filesystem access |
| Text encoding | UTF-8 everywhere | Standard on all targets |
| Memory | All pools allocated in `Context` creation from a `Config` struct | Predictable memory, required for microui-style behavior |
| Overflow policy | When a pool fills: clamp, set `ctx.error()`, skip the work. Never crash or abort in release builds | The UI keeps running and the host can report the problem |
| Threading | One `Context` per thread. Not thread-safe | Simple, matches microui |
| Testing | doctest, vendored single header | Lightweight, works on all targets |
| Build | CMake 3.20+ with presets | Standard across all target platforms |
| Mobile project templates | SDL3's official Android (Gradle) and iOS (Xcode) templates | Maintained by the SDL project |

---

## 4. Repository layout

```
Radon/
  CMakeLists.txt
  CMakePresets.json
  LICENSE
  README.md
  CHANGELOG.md
  docs/
    PLAN.md               # this file
    STATUS.md             # per-target build and run status (see section 9)
    usage.md
    decisions/            # one ADR per significant decision (0001-*.md)
  include/radon/
    radon.hpp             # single public include
    types.hpp             # Vec2, Rect, Color, Input, Response, fixed-capacity event queues
    config.hpp            # Config (pool sizes, DPI, atlas size)
    theme.hpp             # Theme (colors, radii, spacing, font sizes)
    context.hpp           # Context: frame lifecycle, IDs, input state
    layout.hpp            # Layout stack and cell allocation
    widgets.hpp           # Widget functions (free functions taking Context&)
    draw_list.hpp         # DrawVertex, DrawCommand, DrawList
    text.hpp              # Font handles, measurement, wrapping
  src/core/
    context.cpp
    ids.cpp               # FNV-1a hashing, ID stack
    input.cpp             # hover, focus, active transitions
    layout.cpp
    window.cpp            # windows, containers, z-order, scrolling
    widgets/              # one .cpp per widget family
    draw_list.cpp         # SDF shape emission, clip stack
    text.cpp              # glyph cache, wrapping, measurement
    theme.cpp             # default dark and light themes
  src/third_party/        # stb_truetype.h, stb_rect_pack.h, doctest.h, with LICENSE files
  backends/sdl3_gl/
    platform_sdl3.cpp     # SDL events to rd::Input, DPI, clipboard, text input, safe area
    renderer_gl.cpp       # draw list to GL / GLES3 / WebGL2
    shaders.cpp           # GLSL sources as string literals
  examples/
    desktop_demo/         # SDL3 + GL demo, all widgets
    web_demo/             # Emscripten build of the same demo, plus an HTML shell
    mobile_demo/          # Android (Gradle) and iOS (Xcode) project templates around the same demo code
    assets/font.ttf       # demo font (owner supplies; see section 12)
  tests/
    test_ids.cpp
    test_input.cpp
    test_layout.cpp
    test_draw_list.cpp
    test_text.cpp
    test_widgets.cpp
    test_textbox.cpp
    test_alloc.cpp        # counting allocator, checks zero per-frame allocation
  scripts/
    check_core_includes.sh
  .github/workflows/ci.yml
```

Rules:
- `include/` and `src/core/` must not include any SDL, GL, or platform header. `scripts/check_core_includes.sh` enforces this and runs in CI.
- Backends depend on the core. The core never depends on backends.

---

## 5. Public API

### 5.1 Types

```cpp
namespace rd {

struct Vec2 { float x = 0, y = 0; };
struct Rect { float x = 0, y = 0, w = 0, h = 0; };
struct Color { uint8_t r = 0, g = 0, b = 0, a = 255; };

// Fixed-capacity queues. No heap allocation.
struct KeyEvent { int key; bool shift, ctrl, alt; };         // Enter, Backspace, Delete, arrows, Home, End, Tab, Escape, A, C, V, X
template <typename T, int N> struct FixedQueue { T items[N]; int count = 0; bool push(const T&); };
using KeyEvents = FixedQueue<KeyEvent, 16>;
using TextEvents = FixedQueue<char, 256>;   // raw UTF-8 bytes

// Input is filled by the platform backend once per frame.
struct Input {
    Vec2 pointer{};                  // logical pixels, top-left origin
    bool pointer_down = false;       // held this frame
    bool pointer_pressed = false;    // went down this frame
    bool pointer_released = false;   // went up this frame
    float wheel = 0;                 // vertical scroll, in logical lines
    KeyEvents keys;
    TextEvents text;
    Rect safe_area{};                // mobile insets in logical pixels; zero on desktop
};

struct Response {
    bool hovered = false;
    bool focused = false;
    bool pressed = false;   // became active this frame
    bool clicked = false;   // released over the same widget this frame
    bool changed = false;   // value changed this frame (value widgets)
};

}  // namespace rd
```

### 5.2 Config

```cpp
struct Config {
    int max_windows = 32;
    int max_id_stack = 32;
    int max_layout_stack = 64;
    int max_draw_commands = 4096;
    int max_vertices = 65536;
    int max_indices = 196608;
    int max_textboxes = 16;         // edit state, keyed by ID
    int atlas_size = 1024;          // glyph atlas, square, power of two
    float dpi_scale = 1.0f;
    Theme theme = Theme::dark();
};
```

### 5.3 Context lifecycle

```cpp
class Context {
public:
    explicit Context(const Config& cfg = {});
    void begin_frame(const Input& input, float dt_seconds);
    void end_frame();
    const DrawList& draw_list() const;
    bool error() const;                       // true if a pool overflowed this frame
    void set_theme(const Theme& theme);
    FontHandle load_font(const uint8_t* data, size_t size, float pixel_size);
    void set_default_font(FontHandle font);
    void set_clipboard_callbacks(ClipboardGet get, ClipboardSet set);  // supplied by the backend
};
```

Use `const float* values, int count` or a small `Span<T>` for arrays. Do not use `std::span` (C++20).

### 5.4 Windows and layout

```cpp
enum WindowFlags : uint32_t {
    WindowNone       = 0,
    WindowMovable    = 1 << 0,
    WindowResizable  = 1 << 1,
    WindowClosable   = 1 << 2,
    WindowScrollable = 1 << 3,
    WindowTitleBar   = 1 << 4,
    WindowBorder     = 1 << 5,
    WindowDefault    = WindowMovable | WindowResizable | WindowTitleBar | WindowBorder,
};

bool begin_window(Context& ctx, std::string_view title, Rect& rect, uint32_t flags = WindowDefault);
void end_window(Context& ctx);

void layout_row(Context& ctx, int columns, const float* widths, int count, float height);
void layout_begin_column(Context& ctx);   // nested vertical layout
void layout_end_column(Context& ctx);
void spacing(Context& ctx, float px);
void separator(Context& ctx);
```

Widths: a positive value is pixels. A negative value means "fill remaining space" (as in microui).

### 5.5 Widgets

Each widget takes the context first. The ID comes from the label. `"Label##suffix"` makes labels unique without showing the suffix.

```cpp
Response button(Context& ctx, std::string_view label);
Response checkbox(Context& ctx, std::string_view label, bool& value);
Response slider(Context& ctx, std::string_view label, float& value, float min, float max, const char* format = "%.2f");
Response drag_float(Context& ctx, std::string_view label, float& value, float speed = 1.0f);
Response textbox(Context& ctx, std::string_view label, TextBuffer& buffer);
Response label(Context& ctx, std::string_view text);
Response label_wrapped(Context& ctx, std::string_view text);
Response progress_bar(Context& ctx, float fraction);
Response image(Context& ctx, TextureId tex, Vec2 size, Rect uv = {0, 0, 1, 1});
bool collapsing_header(Context& ctx, std::string_view label, bool& open);
bool combo(Context& ctx, std::string_view label, int& selected, const char* const* items, int count);
void tooltip(Context& ctx, std::string_view text);     // shown while the last widget is hovered
bool popup_begin(Context& ctx, std::string_view id);
void popup_end(Context& ctx);
void open_popup(Context& ctx, std::string_view id);
void push_id(Context& ctx, std::string_view id);
void pop_id(Context& ctx);
```

`TextBuffer` is a non-owning view: `struct TextBuffer { char* data; int capacity; int length; };`. The caller owns the memory.

### 5.6 Fonts and text

```cpp
Vec2 measure_text(const Context& ctx, FontHandle font, std::string_view text);
```

Font data is always passed in memory, never read from a path, so web and mobile targets work.

### 5.7 Example usage

```cpp
rd::Context ui;                  // created once
rd::Input input;                 // filled by the backend each frame

void frame(float dt) {
    ui.begin_frame(input, dt);

    static rd::Rect win{20, 20, 320, 240};
    if (rd::begin_window(ui, "Settings", win)) {
        static bool sound = true;
        static float volume = 0.5f;
        static char name[64] = "player";
        static rd::TextBuffer name_buf{name, sizeof(name), 6};

        const float cols[] = {90, -1};
        rd::layout_row(ui, 2, cols, 2, 0);
        rd::label(ui, "Name");
        rd::textbox(ui, "##name", name_buf);

        rd::layout_row(ui, 2, cols, 2, 0);
        rd::label(ui, "Volume");
        rd::slider(ui, "##volume", volume, 0.0f, 1.0f);

        if (rd::button(ui, "Apply").clicked) {
            // apply settings
        }
        rd::end_window(ui);
    }

    ui.end_frame();
    // backend renders ui.draw_list()
}
```

---

## 6. Core subsystems

### 6.1 IDs
- `ID = FNV-1a 32-bit hash` of the ID-stack top plus the label. Text after `##` is hashed but not displayed.
- `push_id` and `pop_id` manage nested scopes (for example, rows in a list).
- Debug builds (`RD_DEBUG_IDS`) record IDs each frame and report duplicates.

### 6.2 Interaction state (microui-style)
Track three IDs across frames: `hover`, `focus`, and `active`.
- A widget is hovered if the pointer is inside its rect and no window above it covers that point.
- Pressing over a widget sets `active` to its ID. Releasing over the same widget sets `clicked`.
- `focus` is set on click or Tab. Keyboard events go only to the focused widget.
- Widget call order within a frame determines the result, as in microui. Document this in `docs/usage.md`.

### 6.3 Windows and containers
- Windows live in a fixed pool. Each has an ID, rect, z-order, scroll offset, and content size.
- Dragging the title bar moves the window. Dragging the bottom-right corner resizes it (minimum 80 by 60 logical pixels).
- Clicking a window raises it to the top of the z-order.
- Scrolling: content larger than the window shows a scrollbar. Mouse wheel and touch drag both scroll.
- Clipping: each window pushes a clip rect. Nested clip rects intersect.

### 6.4 Layout
- Row layout: columns with fixed or fill widths, as in microui.
- Cell height and spacing come from the theme.
- `layout_begin_column` pushes a vertical layout. Cells stack top to bottom.
- Each layout computes its content bounds, which feed the window's scroll range.

### 6.5 Draw list
- Vertex: `{ Vec2 pos; Vec2 uv; Color color; Vec2 local; Vec4 shape; }`. `local` and `shape` carry SDF data (half-size, corner radius, border width) so the fragment shader draws rounded, anti-aliased shapes.
- Command: `{ TextureId texture; Rect clip; uint32_t index_offset; uint32_t index_count; }`. Consecutive primitives with the same texture and clip merge into one command.
- Primitives: `rect` (rounded, bordered, shadowed), `text_run`, `image`, `line`, `triangle`. Circles are optional in v0.1.
- Cleared at `begin_frame`. Never grows past `Config` limits. On overflow: set the error flag and drop the primitive.

### 6.6 Text
- Glyph cache: one atlas texture (`atlas_size` square). Glyphs are rasterized on first use. When the atlas fills, clear it, rebuild on the next frame, and set a warning flag.
- Measurement: advance width and line height, with kerning from stb_truetype.
- Wrapping: break at spaces; break long words at code-point boundaries. Handle UTF-8 correctly.
- Text is drawn from the atlas as coverage (alpha) in the shader.

### 6.7 Theme
`Theme` holds:
- Colors: background, surface, surface_hover, surface_active, text, text_muted, accent, accent_text, border, shadow, error, focus_ring.
- Metrics: padding, spacing, corner_radius, border_width, scrollbar_width, title_height, cell_height.
- Typography: font handle and size for body, heading, and small text.

Built-in themes:
- `Theme::dark()`: background #0a0a0a (from wavestorm.net). Accent #4da3ff (placeholder until the brand palette is confirmed).
- `Theme::light()`: same structure, light values.

No widget may hard-code a color, font size, or spacing. Everything reads from the theme.

### 6.8 Text input and editing
- Insert characters from `input.text`. Delete with Backspace and Delete.
- Move the cursor with Left, Right, Home, End, and mouse or touch tap. Shift extends the selection.
- Ctrl+A selects all. Ctrl+C, Ctrl+X, and Ctrl+V call the clipboard callbacks supplied by the backend.
- Cursor blink period 500 ms, driven by `dt_seconds`.
- Edit state (cursor, selection, scroll offset) lives in a fixed pool keyed by ID (`max_textboxes`).

### 6.9 Animation
- Hover and press transitions interpolate color and size over about 120 ms per widget ID. Blend values live in a fixed pool keyed by ID.
- Animations are cosmetic. Disabling them in `Config` must not change any interaction result.

### 6.10 Keyboard navigation
- Tab and Shift+Tab move focus between focusable widgets in the current window.
- Enter activates a focused button. Space toggles a checkbox.

### 6.11 Touch behavior
- Touch down is a press. Touch up is a click if the finger stayed within a small slop distance (8 logical pixels).
- Touch movement inside a scrollable window scrolls it. Touch does not produce hover.
- Multi-touch is not required in v0.1. The first finger is the pointer.

---

## 7. Backends

### 7.1 Renderer
```cpp
namespace rd::gl {
struct Renderer;
Renderer* create_renderer(const RendererConfig& cfg);  // compiles shaders, creates the atlas texture
void render(Renderer* r, const Context& ctx, int fb_width, int fb_height);
void destroy_renderer(Renderer* r);
}
```
- Shaders: one vertex and one fragment shader. Use `#version 300 es` for WebGL2 and GLES3, and `#version 330 core` for desktop, selected by a preprocessor switch.
- Fragment shader: SDF rounded rectangle, border, and shadow in one path. Text and images use the same shader with a mode uniform.
- Stream the vertex buffer each frame. One draw call per command, scissored to its clip rect.
- Upload the atlas texture only when glyphs change.

### 7.2 SDL3 platform backend
- Create the window with the platform's GL or GLES context. Handle `SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED` and compute the DPI scale.
- Map SDL mouse and finger events to `Input` (section 6.11 applies to touch).
- Map SDL key events to `Input.keys`. Map SDL text events to `Input.text`.
- Call `SDL_StartTextInput` when a textbox gains focus and `SDL_StopTextInput` when it loses focus. This shows and hides the soft keyboard on mobile.
- Provide clipboard callbacks to the core.
- Fill `Input.safe_area` from SDL's safe-area query on iOS and Android. Zero elsewhere.

### 7.3 Web (Emscripten)
- Build with the Emscripten toolchain. Drive frames with `emscripten_set_main_loop`.
- Use WebGL2 through SDL3's Emscripten support.
- Output: one HTML file, one `.js`, and one `.wasm`. No server component.
- Embed the demo font in the binary using `--embed-file` or a generated byte array.

### 7.4 Mobile (Android and iOS)
- Android: SDL3's Android Gradle template. Build with the NDK, produce an unsigned APK.
- iOS: SDL3's Xcode template. Build for the simulator without signing.
- Respect safe-area insets (section 7.2). Pass them through `Input.safe_area`.
- Show the soft keyboard through `SDL_StartTextInput` when a textbox is focused.

---

## 8. Build, tooling, and CI

### CMake options
- `RD_BUILD_EXAMPLES` (default ON on desktop)
- `RD_BUILD_TESTS` (default ON)
- `RD_BACKEND_SDL3_GL` (default ON)
- `RD_WARNINGS_AS_ERRORS` (default ON in CI)

### Presets
- `linux-gcc-debug`, `linux-clang-release`, `windows-msvc-release`, `web-emscripten-release`, `android-ndk-release`, `ios-simulator-release`.

### CI (GitHub Actions, `.github/workflows/ci.yml`)
- `ubuntu-latest`, GCC and Clang: build, run tests, run `scripts/check_core_includes.sh`.
- `windows-latest`, MSVC: build and run tests.
- Emscripten job: build the web demo and check the output files exist.
- Android job (`ubuntu-latest`): build `mobile_demo` with the NDK and Gradle, produce an unsigned APK.
- iOS job (`macos-latest`): build `mobile_demo` for the simulator without signing.
- `clang-format --dry-run --Werror` on all sources.

### Versioning
Semantic versioning. v0.x is pre-stable. Tag `v0.1.0` after M8 passes.

---

## 9. Milestones

Each milestone ends with a commit. The commit message starts with `M<n>:`.

### M0: Scaffold
- Repo layout, LICENSE, README stub, CHANGELOG, CMake, presets, CI, `.clang-format`, `scripts/check_core_includes.sh`.
- Vendor doctest, stb_truetype, and stb_rect_pack with their license files.
- Copy this plan to `docs/PLAN.md` (already present if committed by the owner).
- **Done when:** an empty project builds on Linux and Windows and runs one passing test.

### M1: Core without rendering
- `Context`, `Config`, `Input`, `FixedQueue`, IDs and the ID stack, hover/focus/active logic, layout rows, the draw list with rectangles, and windows with move and z-order.
- **Done when:**
  - Unit tests cover ID hashing (including `##`), hover and click transitions, layout width math with fill cells, and draw-list limits.
  - `test_alloc` shows zero heap allocations during frames after warm-up.
  - `scripts/check_core_includes.sh` passes.

### M2: Text
- Font loading from memory, glyph atlas, measurement, UTF-8 decoding, and wrapping.
- **Done when:**
  - Tests cover UTF-8 decoding, wrapping at spaces and long words, and measurement of a known string.
  - A test rasterizes the demo font and checks atlas coordinates.

### M3: Desktop backend and first demo
- SDL3 platform backend and GL renderer.
- `examples/desktop_demo`: window with a label, button, checkbox, slider, and progress bar.
- **Done when:** the demo runs on Linux and Windows; button clicks and slider drags work. Screenshot saved to `docs/screenshots/m3.png`.

### M4: Widgets and windows
- Scrolling, resize, close button, collapsing header, combo, popup, tooltip, image, drag_float, dark and light themes, hover and press animations.
- **Done when:** every widget in section 5.5 works in the demo, and each has an interaction test in `test_widgets.cpp`.

### M5: Text input and keyboard navigation
- Textbox with selection, clipboard callbacks, cursor blink, Tab navigation.
- **Done when:** `test_textbox.cpp` covers insert, delete, selection, and clipboard through a mock; the demo works without a mouse.

### M6: Web
- `examples/web_demo` built with Emscripten and WebGL2 with an HTML shell.
- Touch path (section 6.11) is covered by unit tests in the core. Browser touch is checked manually.
- **Done when:** the web demo builds in CI, the output loads and renders in a headless browser if one is available, and the STATUS entry is updated.

### M7: Mobile
- Android Gradle project and iOS Xcode project from SDL3's templates, both running `mobile_demo`.
- Safe-area handling and soft keyboard for textboxes.
- **Done when:** both projects build in CI (unsigned). Device runs are recorded in `docs/STATUS.md` as manual checks.

### M8: Polish and release
- `docs/usage.md` covers all of section 5. README has a 20-line quick start. Screenshot gallery in `docs/screenshots/`.
- Final `docs/STATUS.md` covers every target.
- **Done when:** all five targets build in CI, the allocation test and core-includes check pass, and `v0.1.0` is tagged.

---

## 10. Testing strategy

- **Unit tests (doctest):** core logic, run on every CI target, no window or GL needed.
- **Interaction tests:** simulate frames with synthetic `Input`. Assert `Response` fields and draw-list counts.
- **Allocation test:** a counting allocator replaces global `new` and `malloc` in one test binary. Frames after warm-up allocate zero bytes.
- **Golden draw-list tests:** for a fixed layout, record vertex and command counts and compare with a checked-in file. Update intentionally.
- **Visual checks:** the desktop demo screenshots are compared by eye per milestone.
- **Fuzzing (later):** random input events fed to the core for 60 seconds per CI run. The core must never crash.

---

## 11. Ground rules for implementation

1. Do not add a dependency that is not listed in section 3 without asking the owner.
2. The core must not include SDL, GL, or other platform headers. Enforced in CI.
3. The default configuration must not allocate on the heap during a frame.
4. Every public function has a doc comment and appears in `docs/usage.md`.
5. Each significant technical decision gets an ADR in `docs/decisions/`.
6. Small commits, conventional prefixes (`feat:`, `fix:`, `test:`, `docs:`, `chore:`), and milestone prefixes (`M3: ...`).
7. Vendored third-party code keeps its license header and sits in `src/third_party/` with its LICENSE file.
8. On a full pool or atlas: never crash. Set the error flag, skip the work, keep the frame running.
9. No hard-coded colors, font sizes, or spacing in widgets. Read them from the theme.
10. Do not claim a target works unless it was built and run. Record anything unverified in `docs/STATUS.md`.

---

## 12. Decisions and open items

Decided for the first build:
- License: MIT.
- Accent color: #4da3ff placeholder.
- Dependencies: vendor doctest, stb_truetype, stb_rect_pack. Fetch SDL3 with `FetchContent`.
- Demo font: the owner supplies `examples/assets/font.ttf`. Choose a permissively licensed font (for example, one under the SIL Open Font License). Until it exists, the demo prints a clear error at startup and the build still succeeds.

Owner actions before or during the build:
1. Create `WavestormSoftware/Radon` on GitHub and attach it to the Claude Code session.
2. Commit this file to the repo as `docs/PLAN.md`.
3. Add the demo font to `examples/assets/font.ttf` (or accept the placeholder behavior).
4. Confirm the accent color.
5. Mobile signing (Apple Developer account, Android keystore) is needed for device and store builds. CI builds only unsigned artifacts.
