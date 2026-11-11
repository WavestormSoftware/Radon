// Copyright (c) 2026 Wavestorm Software
// raylib backend for Radon: input from raylib, draw list rendered through rlgl. Reference adapter that
// shows the core works outside SDL3/OpenGL; for engines with a vertex-buffer API the code maps 1:1.
#pragma once
#include <radon/radon.hpp>

#include <raylib.h>

namespace rd::rl {

struct Renderer;
/// Compiles the shader. Call after InitWindow.
Renderer* create_renderer();
void destroy_renderer(Renderer* r);
/// Registers a raylib texture (for rd::image / rd::viewport) under `id` (1..63). Radon never frees it.
/// For a RenderTexture2D pass `rt.texture` and use `flip_y = true` in rd::viewport.
void set_texture(Renderer* r, TextureId id, Texture2D tex);
/// Fills `in` from raylib: mouse, wheel, keys, typed text, window size.
void fill_input(Input& in);
/// Applies the display scale (render size / screen size). Call once per frame before begin_frame.
void apply_dpi(Context& ctx);
/// Draws the context's draw list. Call between BeginDrawing and EndDrawing.
void render(Renderer* r, const Context& ctx);

}  // namespace rd::rl
