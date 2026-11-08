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

    /// Glyph atlas for the backend: 8-bit coverage, atlas_size x atlas_size. The version changes
    /// whenever the pixels change.
    const uint8_t* atlas_pixels() const;
    int atlas_size() const;
    uint32_t atlas_version() const;

    /// True while a text box has focus (show the soft keyboard).
    bool wants_text_input() const;
    /// True if the pointer is over a window or popup (the host should not use the click).
    bool pointer_over_ui() const;

private:
    friend detail::State& detail::state(Context&);
    friend const detail::State& detail::state(const Context&);
    std::unique_ptr<detail::State> s_;
};

}  // namespace rd
