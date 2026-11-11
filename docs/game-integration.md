# Game and engine integration

Radon does not own your game loop, window or renderer. It integrates in two directions:

| You want | Use |
|---|---|
| A game or 3D view inside a Radon panel | [`rd::viewport`](#a-game-view-inside-a-panel) |
| 2D game drawing (shapes, sprites) inside a panel | [`rd::Canvas`](#2d-canvas) |
| Radon UI drawn by your engine | [the C API or the draw list](#radon-inside-your-engine) |
| Gamepads and sound with the SDL3 backend | [Gamepad](#gamepad) and [Audio](#audio) |

## A game view inside a panel

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

- `viewport` reports `hovered`, `focused`, `pressed`, `down`, `released`, `wheel` and `local`. A press inside gives focus; a drag that started inside keeps reporting after the pointer leaves.
- `begin_target` restores only the framebuffer and viewport. Set and restore your own state (program, depth test, blend).
- `resize_render_target` reallocates the target (contents are lost). Call it when the panel size changes.
- Already have a GL texture from an engine? `adopt_texture(renderer, gl_id)` wraps it without copying; you keep ownership.
- `update_texture` re-uploads pixels of a texture from `create_texture` (software-rendered views).
- `destroy_texture` frees a texture. Its id can be reused by the next texture you create.

Example: `examples/snippets/game_viewport.cpp`.

## 2D canvas

```cpp
rd::Canvas c = rd::canvas_begin(ui, {360, 200}, /*background=*/{20, 24, 36, 255});
c.fill_rect({x, y, 24, 24}, {255, 160, 0, 255}, /*radius=*/4);
c.circle({180, 100}, 14, {80, 200, 255, 255});
c.sprite(atlas, {x, y, 32, 32}, /*uv=*/{0, 0, 0.25f, 0.25f});   // a sprite-sheet cell
if (c.response.pressed) player = c.local;
rd::canvas_end(ui);
```

Coordinates are relative to the canvas and everything is clipped to it. Shapes are `fill_rect`, `stroke_rect`, `circle`, `line`, `triangle` and `sprite`. Example: `examples/snippets/game_canvas.cpp`.

## Gamepad

SDL3 backend only. `rd::sdl3::gamepad_state(platform, index, state)` fills sticks (dead zone applied, default 0.15), triggers and button bits (`PadSouth`, `PadUp`, ...). `gamepad_count` tells how many are connected. Pads also drive the UI: D-pad = arrows, A = Space, B = Escape, Start = Enter, bumpers = Tab.

## Audio

SDL3 backend only. `load_sound(path)` / `load_sound_memory(data, size)` load WAV, `play_sound(sound, volume)` mixes up to 8 voices on the default device, `stop_all_sounds()` silences them. Every call returns false when no audio device exists, so headless runs are safe. There is no music streaming or looping.

## Radon inside your engine

The core never touches a platform or GPU. Feed it input, call widgets, then render the draw list yourself.

From C or any FFI (C#, Rust, Lua, ...) use `include/radon/radon_c.h`. It is built into the core when `RD_BUILD_C_API=ON` (default when Radon is the top-level project):

```c
rd_context* ui = rd_create();
rd_begin_frame(ui, &input, dt);
if (rd_begin_window(ui, "Hello", &rect)) {
    rd_layout_row(ui, 1, NULL, 0, 0);
    if (rd_button(ui, "Click")) start_game();
    rd_end_window(ui);
}
rd_end_frame(ui);
int nv, ni, nc;
const rd_vertex* v = rd_vertices(ui, &nv);   // then rd_indices, rd_commands, rd_atlas
```

Wrapped in C: windows (with flags), layout row, label, wrapped label, buttons (with styles), checkbox, slider, drag float, textbox, progress bar, collapsing header, combo, image, viewport and the 2D canvas. Other widgets need a wrapper in `src/core/radon_c.cpp` (a few lines each). For a shared library define `RD_C_BUILD_SHARED` when building and `RD_C_SHARED` when consuming on Windows; `RD_API` handles the export. `rd_version()` returns `RD_C_API_VERSION`.

`rd_vertex` and `rd_draw_command` have the same layout as the C++ structs. The vertex layout and shader contract are in [backends.md](backends.md); `backends/sdl3_gl/shaders.cpp` is the reference shader. Textures other than the glyph atlas (id 0) use your own ids: pass them to `rd_viewport`, `rd_image` or `rd_canvas_sprite`, and bind them yourself when a command's `texture` is non-zero.

### raylib backend

`backends/raylib` draws the draw list through rlgl and reads raylib input. `examples/raylib/raylib_demo.cpp` shows a raylib 3D scene (render texture) inside a Radon panel. Build with `-DRD_BACKEND_RAYLIB=ON` (fetches raylib 5.5). It is also the template for other engines: the shader is the same, and the draw list is expanded to a triangle list because raylib's index buffers are 16-bit.

### Other engines

Unity, Unreal, Godot, bgfx, Vulkan, Metal and DirectX are not shipped as backends or plugins. Link the C API (or the C++ core), write the renderer against the contract in [backends.md](backends.md), and use the raylib backend as a worked example.
