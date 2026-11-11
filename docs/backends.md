# Backends

The core never draws and never touches the platform. A **backend** has two jobs: feed `rd::Input` in, and
render `Context::draw_list()` out.

## The SDL3 + OpenGL backend (`radon::sdl3_gl`)

One implementation covers Windows, Linux, macOS, Web, Android and iOS. Header: `backends/sdl3_gl/radon_sdl3_gl.hpp`.

```cpp
namespace rd::sdl3 {
Platform* create_platform(const PlatformConfig&);    // SDL_Init, window, GL 3.3 core (desktop) or GLES 3
bool poll(Platform*, Input&, float& dt);              // events -> Input; false on quit
void attach(Platform*, Context&);                     // DPI scale + clipboard callbacks
void finish_frame(Platform*, Context&);               // soft keyboard + IME rect, cursor, swap buffers
void framebuffer_size(const Platform*, int& w, int& h);
void logical_size(const Platform*, int& w, int& h);   // framebuffer size / dpi_scale
bool system_dark(const Platform*);   bool system_theme_changed(Platform*);
bool open_url(const char*);
void show_file_dialog(Platform*, FileDialog, const char* where = nullptr);
bool poll_file_dialog(Platform*, std::string& path);
float dpi_scale(const Platform*);   bool is_gles(const Platform*);   void request_quit(Platform*);
}
namespace rd::gl {
Renderer* create_renderer(const RendererConfig&);     // loads GL, compiles shaders
void clear(Renderer*, Color);   void render(Renderer*, const Context&, int fb_w, int fb_h);
TextureId create_texture(Renderer*, int w, int h, const uint8_t* rgba);   // for rd::image
bool read_pixels(Renderer*, int w, int h, uint8_t* out);                  // screenshots
}
```

What `poll` does: maps mouse and **finger** events to one pointer (the first finger; touch is flagged so
Radon disables hover and scrolls by drag), keys to `rd::Key*`, text input to UTF-8 bytes, the wheel to
lines, and `SDL_GetWindowSafeArea` to `Input::safe_area`. Ctrl is Cmd on Apple platforms. `finish_frame`
calls `SDL_StartTextInput` while a text box is focused, which shows the soft keyboard on phones.

`poll` also maps the right mouse button to `secondary_pressed`, the Android back button to Escape (or
quit when `ctx.can_dismiss()` is false), blocks while the app is in the background, and with
`PlatformConfig::wait_events` sleeps while the UI is idle. All of this is described in
[platforms.md](platforms.md).

Logical pixels are physical pixels divided by `dpi_scale`, the OS display scale. Pointer positions and
the safe area are converted for you. Pass `fb_width/height` in physical pixels to `render`.

GL entry points are loaded through `SDL_GL_GetProcAddress`, so there is no GL loader dependency. On
Emscripten link with `-sGL_ENABLE_GET_PROC_ADDRESS=1` (the web demo does).

## Writing your own renderer

Read `ctx.draw_list()` after `end_frame`:

```cpp
const rd::DrawList& dl = ctx.draw_list();
dl.vertices(), dl.vertex_count();      // rd::DrawVertex: pos, uv, color, local, shape
dl.indices(),  dl.index_count();       // uint32 triangle list
dl.commands(), dl.command_count();     // rd::DrawCommand: texture, clip rect, index_offset, index_count
```

Draw each command in order with its scissor rectangle (logical px; multiply by the DPI scale):

```
for each command c:
    scissor(c.clip);  bind texture (0 = glyph atlas, else yours);
    draw_indexed(c.index_count, c.index_offset);
```

The atlas is `ctx.atlas_pixels()` (8-bit coverage, `atlas_size()` square). Re-upload it when
`ctx.atlas_version()` changes.

### Vertex shape data

`DrawVertex::shape = (half width, half height, radius, border)` and `local` is the position relative to the
shape center. `shape.z` selects how to shade:

| `shape.z` | Meaning |
|---|---|
| `>= 0` | Signed-distance rounded rectangle. `border > 0` means "draw only a ring of that width". `uv.x > 0` means a soft shadow with that blur. |
| `-1` (`ShapeGlyph`) | Text: `texture(atlas, uv).r` is coverage, multiplied into `color.a`. |
| `-2` (`ShapeImage`) | `texture(tex, uv) * color`. |
| `-3` (`ShapeSolid`) | `color`, interpolated across the triangle (lines, triangles, gradients). |

The reference fragment shader is `backends/sdl3_gl/shaders.cpp` (about 25 lines). Use premultiplied or
straight alpha blending with `SRC_ALPHA, ONE_MINUS_SRC_ALPHA` as the GL backend does.

### Platform side

Fill `rd::Input` each frame (see [getting-started.md](getting-started.md#5-handling-input-yourself)),
provide clipboard callbacks with `ctx.set_clipboard_callbacks(get, set, user)`, and set
`ctx.set_dpi_scale(scale)` when the display scale is not 1.

## Using the core without any backend

The tests drive the core headlessly with synthetic `Input` (see `tests/harness.hpp`). That is also a good
way to unit-test your own UI code.
