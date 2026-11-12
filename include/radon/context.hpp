// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <memory>
#include <radon/config.hpp>
#include <radon/draw_list.hpp>
#include <radon/types.hpp>

namespace rd {

class Context;
namespace detail {
struct State;
State& state(Context&);
const State& state(const Context&);
}  // namespace detail

/// FNV-1a 32-bit hash of `s` continuing from `seed` (0 starts from the FNV offset basis).
ID hash_id(ID seed, std::string_view s);

class Context {
public:
    explicit Context(const Config& cfg = {});
    ~Context();
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    /// Starts a frame. Call once per frame before any widget.
    void begin_frame(const Input& input, float dt_seconds);
    /// Finishes the frame and finalizes the draw list.
    void end_frame();
    const DrawList& draw_list() const;
    /// True if a pool overflowed this frame.
    bool error() const;
    /// True if the glyph atlas filled and was cleared (glyphs reappear next frame).
    bool warning() const;
    void set_theme(const Theme& theme);
    const Theme& theme() const;
    void set_dpi_scale(float scale);
    float dpi_scale() const;

    /// Loads a TrueType font from memory. The buffer must outlive the context. Returns -1 on failure.
    FontHandle load_font(const uint8_t* data, size_t size, float pixel_size);
    void set_default_font(FontHandle font);
    void set_clipboard_callbacks(ClipboardGet get, ClipboardSet set, void* user = nullptr);

    /// Glyph atlas for the backend: 8-bit signed distance (0.5 = glyph edge), atlas_size x atlas_size. The version changes
    /// whenever the pixels change.
    const uint8_t* atlas_pixels() const;
    int atlas_size() const;
    uint32_t atlas_version() const;

    /// True while a text box has focus (show the soft keyboard).
    bool wants_text_input() const;
    /// True if the pointer is over a window or popup (the host should not use the click).
    bool pointer_over_ui() const;
    /// Where the focused text box is, in logical px (zero when none). Backends pass it to the IME /
    /// soft keyboard so the field stays visible.
    Rect text_input_rect() const;
    /// The mouse cursor the last frame asked for (text beam over text boxes, resize arrows, ...).
    Cursor cursor() const;
    /// The titlebar drag region of the last frame (empty caption when no titlebar was drawn).
    const WindowChrome& window_chrome() const;
    /// What the titlebar's minimize/maximize/close buttons asked for during the last frame.
    WindowCommand window_command() const;
    /// True if the last frame was still animating or handling input. When false the host may sleep
    /// until the next input event instead of redrawing (saves battery).
    bool animating() const;
    /// True when Escape (or the Android back button) would close something: a popup, a dialog or
    /// drawer, or the focused text box. When false, a back press should leave the app.
    bool can_dismiss() const;
    /// Turns transitions on or off at runtime (for "reduce motion" settings).
    void set_animations(bool enabled);

    /// Glyphs missing from `font` are taken from `fallback` (emoji, CJK, symbols). Chains up to 4 deep.
    void set_font_fallback(FontHandle font, FontHandle fallback);
    /// The keyboard the focused text field wants. Backends pass it when showing the soft keyboard.
    TextInputType text_input_type() const;
    /// The strongest haptic feedback a widget asked for during the last frame. Backends play it.
    Haptic haptic() const;
    /// Pinch, rotate, two-finger pan and swipe for the current frame.
    const Gesture& gesture() const;
    /// The accessibility tree of the last frame (empty unless Config::a11y). Valid until begin_frame.
    const A11yNode* a11y_nodes(int* count) const;

private:
    friend detail::State& detail::state(Context&);
    friend const detail::State& detail::state(const Context&);
    std::unique_ptr<detail::State> s_;
};

}  // namespace rd
