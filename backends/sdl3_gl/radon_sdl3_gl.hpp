// Copyright (c) 2026 Wavestorm Software
// SDL3 platform layer and OpenGL ES 3 / GL 3.3 renderer for Radon.
#pragma once
#include <radon/radon.hpp>

#include <string>

namespace rd::sdl3 {

struct PlatformConfig {
    const char* title = "Radon";
    int width = 960, height = 600;
    bool resizable = true;
    bool vsync = true;
    /// Sleep until the next input event while the UI is idle (Context::animating() is false) instead
    /// of redrawing every frame. Saves battery. Ignored on the web, where the browser owns the loop.
    bool wait_events = false;
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
/// Physical pixels per logical pixel: the OS display scale (Windows/Linux scaling, Android density,
/// Retina). Radon lays out in logical pixels, so UI has the same physical size on every screen.
float dpi_scale(const Platform* p);
/// Window size in logical pixels (framebuffer size / dpi_scale).
void logical_size(const Platform* p, int& width, int& height);
/// True if the OS is set to a dark theme (also when the OS does not say).
bool system_dark(const Platform* p);
/// True once after the OS switched between light and dark.
bool system_theme_changed(Platform* p);
/// Opens a URL in the system browser.
bool open_url(const char* url);

enum class FileDialog { Open, Save, Folder };
/// Shows the native file dialog (Windows, macOS, Linux portals/zenity; Android partly; not iOS or
/// the web). It does not block: fetch the result with poll_file_dialog on a later frame.
void show_file_dialog(Platform* p, FileDialog kind, const char* default_location = nullptr);
/// Returns true once when a dialog has finished. `path` is empty if the user cancelled.
bool poll_file_dialog(Platform* p, std::string& path);
bool is_gles(const Platform* p);

/// Gamepad bits for GamepadState::buttons.
enum GamepadButton : uint32_t {
    PadSouth = 1u << 0,
    PadEast = 1u << 1,
    PadWest = 1u << 2,
    PadNorth = 1u << 3,
    PadLeftShoulder = 1u << 4,
    PadRightShoulder = 1u << 5,
    PadBack = 1u << 6,
    PadStart = 1u << 7,
    PadLeftStick = 1u << 8,
    PadRightStick = 1u << 9,
    PadUp = 1u << 10,
    PadDown = 1u << 11,
    PadLeft = 1u << 12,
    PadRight = 1u << 13,
};
struct GamepadState {
    bool connected = false;
    Vec2 left, right;                           ///< sticks, -1..1, y positive = down
    float left_trigger = 0, right_trigger = 0;  ///< 0..1
    uint32_t buttons = 0;                       ///< GamepadButton bits
};
/// State of the index-th connected gamepad for your game. Returns false when there is none.
/// Gamepads also drive the UI: D-pad = arrows, A = Space, B = Escape, Start = Enter, bumpers = Tab.
bool gamepad_state(const Platform* p, int index, GamepadState& out);

/// Audio: short WAV sound effects mixed on the default output device. Needs no Platform.
/// Every call returns false/null when no audio device exists (headless CI), so it is safe to leave in.
struct Sound;
Sound* load_sound(const char* wav_path);
Sound* load_sound_memory(const uint8_t* wav, size_t size);
/// Starts playback on a free voice (8 at once; extra plays are dropped). volume 0..1.
bool play_sound(Sound* s, float volume = 1.0f);
void destroy_sound(Sound* s);
void audio_shutdown();  ///< Called by destroy_platform; call it yourself if you use audio without a Platform.
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
/// Wraps a GL texture you created (RGBA, GL_TEXTURE_2D) for rd::image / rd::viewport. No copy; you keep
/// ownership and must keep it alive while used. Returns 0 on failure.
TextureId adopt_texture(Renderer* r, unsigned gl_texture);
/// Re-uploads pixels of a texture made by create_texture (same or new size).
bool update_texture(Renderer* r, TextureId id, int width, int height, const uint8_t* rgba);
/// Frees a texture (owned ones are deleted; adopted ones are only forgotten). The id is not reused.
void destroy_texture(Renderer* r, TextureId id);
/// Offscreen target for rendering a game/3D view into a texture. Draw into it between begin_target and
/// end_target with your own GL, then show it with rd::viewport(ctx, id, size, /*flip_y=*/true).
TextureId create_render_target(Renderer* r, int width, int height, bool depth = false);
/// Reallocates the target (contents are lost). No-op when the size is unchanged.
bool resize_render_target(Renderer* r, TextureId id, int width, int height);
/// Binds the target, sets the viewport and clears it. Saves the previous framebuffer and viewport.
/// Other GL state (program, depth test, blend) is yours to set and restore.
bool begin_target(Renderer* r, TextureId id, Color clear_color);
/// Restores the framebuffer and viewport that begin_target saved.
void end_target(Renderer* r);
/// Clears the framebuffer. Call before render().
void clear(Renderer* r, Color c);
/// Reads the framebuffer as top-down RGBA (for screenshots). `out` has width*height*4 bytes.
bool read_pixels(Renderer* r, int width, int height, uint8_t* out);

}  // namespace rd::gl
