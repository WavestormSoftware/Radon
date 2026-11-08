// Copyright (c) 2026 Wavestorm Software
// SDL3 platform layer and OpenGL ES 3 / GL 3.3 renderer for Radon.
#pragma once
#include <radon/radon.hpp>

namespace rd::sdl3 {

struct PlatformConfig {
    const char* title = "Radon";
    int width = 960, height = 600;
    bool resizable = true;
    bool vsync = true;
};

struct Platform;
/// Initializes SDL, creates the window and the GL (desktop) or GLES3 (web/mobile) context.
/// Returns null on failure; SDL_GetError() has the reason.
Platform* create_platform(const PlatformConfig& cfg);
void destroy_platform(Platform* p);
/// Pumps events and fills `input` for the next frame. Returns false when the app should quit.
bool poll(Platform* p, Input& input, float& dt_seconds);
/// Call once per frame after Context::end_frame: soft keyboard, clipboard wiring, swap.
void finish_frame(Platform* p, Context& ctx);
/// Applies DPI scale and clipboard callbacks to a context. Call once after creating it.
void attach(Platform* p, Context& ctx);
void framebuffer_size(const Platform* p, int& width, int& height);
float dpi_scale(const Platform* p);
bool is_gles(const Platform* p);
void request_quit(Platform* p);

}  // namespace rd::sdl3

namespace rd::gl {

struct RendererConfig {
    bool gles = false;  ///< true for GLES3 / WebGL2 contexts (selects the shader header)
};

struct Renderer;
/// Loads GL entry points, compiles the shaders and creates the atlas texture. Needs a current context.
Renderer* create_renderer(const RendererConfig& cfg);
/// Draws the context's draw list into the current framebuffer.
void render(Renderer* r, const Context& ctx, int fb_width, int fb_height);
void destroy_renderer(Renderer* r);
/// Creates an RGBA8 texture for rd::image. Returns 0 on failure; ids are usable as TextureId.
TextureId create_texture(Renderer* r, int width, int height, const uint8_t* rgba);
/// Clears the framebuffer. Call before render().
void clear(Renderer* r, Color c);
/// Reads the framebuffer as top-down RGBA (for screenshots). `out` has width*height*4 bytes.
bool read_pixels(Renderer* r, int width, int height, uint8_t* out);

}  // namespace rd::gl
