# Game and engine integration

## Showing a game view inside Radon

Render your scene into an offscreen target, then show it with `rd::viewport`:

```cpp
TextureId view = rd::gl::create_render_target(renderer, 640, 360, /*depth=*/true);

// every frame, between begin_frame and end_frame:
auto v = rd::viewport(ui, view, {640, 360}, /*flip_y=*/true);
if (v.down) camera.drag(v.local);      // pointer relative to the view, also while dragging outside it
if (v.keys) handle_keys(*v.keys);      // only while the view has focus

rd::gl::begin_target(renderer, view, {20, 20, 24, 255});   // binds, sets viewport, clears
draw_my_scene();                                           // your GL / engine calls
rd::gl::end_target(renderer);                              // restores framebuffer and viewport
```

- `begin_target` only restores the framebuffer and viewport. Set and restore your own state (program, depth test, blend).
- `resize_render_target` reallocates the target (contents are lost). Call it when the panel size changes.
- Already have a GL texture from an engine? `adopt_texture(renderer, gl_id)` wraps it without copying; you keep ownership.
- `update_texture` re-uploads pixels of a texture from `create_texture` (software-rendered views).
- `viewport` reports `hovered`, `focused`, `pressed`, `down`, `released`, `wheel` and `local`. A press inside gives focus; a drag that started inside keeps reporting after the pointer leaves.

See `examples/snippets/game_viewport.cpp`.

## Radon inside another engine

The core never touches a platform or GPU. Feed it input, call widgets, then render the draw list yourself.

From C (or any FFI) use `include/radon/radon_c.h`, built into the core when `RD_BUILD_C_API=ON` (default when Radon is the top-level project):

```c
rd_context* ui = rd_create();
rd_begin_frame(ui, &input, dt);
if (rd_begin_window(ui, "Hello", &rect)) {
    rd_layout_row(ui, 1, NULL, 0, 0);
    if (rd_button(ui, "Click")) ...;
    rd_end_window(ui);
}
rd_end_frame(ui);
int nv, ni, nc;
const rd_vertex* v = rd_vertices(ui, &nv);          // then rd_indices, rd_commands, rd_atlas
```

`rd_vertex` and `rd_draw_command` have the same layout as the C++ structs. The vertex layout and shader contract are in [backends.md](backends.md); `backends/sdl3_gl/shaders.cpp` is the reference shader. Textures other than the glyph atlas (id 0) use your own ids: pass them to `rd_viewport` and bind them yourself when a command's `texture` is non-zero.

The canvas is C++ only. Wrapped in C: windows (with flags), layout row, label, wrapped label, buttons (with styles), checkbox, slider, drag float, textbox, progress bar, collapsing header, combo, image and viewport. Other widgets need a new wrapper in `src/core/radon_c.cpp` (one line each). For a shared library define `RD_C_BUILD_SHARED` when building and `RD_C_SHARED` when consuming on Windows; `RD_API` handles the export.

## 2D canvas, gamepad, audio

- **Canvas:** `rd::Canvas c = rd::canvas_begin(ui, {w, h}, background); c.fill_rect / stroke_rect / circle / line / triangle / sprite; rd::canvas_end(ui);`. Coordinates are relative to the canvas and clipped to it; `c.local` and `c.response` give pointer and click state. `sprite` takes a uv rect for sprite sheets. See `examples/snippets/game_canvas.cpp`.
- **Gamepad (SDL3 backend):** `rd::sdl3::gamepad_state(platform, index, state)` returns sticks, triggers and button bits for your game. Pads also drive the UI: D-pad = arrows, A = Space, B = Escape, Start = Enter, bumpers = Tab.
- **Audio (SDL3 backend):** `load_sound(path)` / `load_sound_memory(...)` for WAV, `play_sound(sound, volume)` mixes up to 8 voices on the default device. It returns false when no audio device exists, so headless runs are safe. No music streaming or looping yet.

## raylib

`backends/raylib` draws the draw list through rlgl and reads raylib input; `examples/raylib/raylib_demo.cpp` shows a raylib 3D scene (render texture) inside a Radon panel. Build with `-DRD_BACKEND_RAYLIB=ON` (fetches raylib 5.5). It is also the template for other engines: the shader is the same, and the draw list is expanded to a triangle list because raylib's index buffers are 16-bit.

## Other engines

Unity, Unreal, Godot, bgfx, Vulkan, Metal and DirectX are not shipped. The pieces they need exist: the C API for the core, the vertex and shader contract in [backends.md](backends.md), and the raylib backend as a worked example of a non-SDL renderer.
