// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <radon/context.hpp>

namespace rd {

enum WindowFlags : uint32_t {
    WindowNone = 0,
    WindowMovable = 1 << 0,
    WindowResizable = 1 << 1,
    WindowClosable = 1 << 2,
    WindowScrollable = 1 << 3,
    WindowTitleBar = 1 << 4,
    WindowBorder = 1 << 5,
    WindowDefault = WindowMovable | WindowResizable | WindowTitleBar | WindowBorder,
};

/// Begins a window. Returns false if it is closed or the pool is full; call end_window only when true.
bool begin_window(Context& ctx, std::string_view title, Rect& rect, uint32_t flags = WindowDefault);
void end_window(Context& ctx);
/// Reopens a window closed with its close button.
void open_window(Context& ctx, std::string_view title);

Response button(Context& ctx, std::string_view label);
Response checkbox(Context& ctx, std::string_view label, bool& value);
Response slider(Context& ctx, std::string_view label, float& value, float min, float max,
                const char* format = "%.2f");
Response drag_float(Context& ctx, std::string_view label, float& value, float speed = 1.0f);
/// Enter reports `clicked` (submit). Escape drops focus.
Response textbox(Context& ctx, std::string_view label, TextBuffer& buffer);
Response label(Context& ctx, std::string_view text);
Response label_wrapped(Context& ctx, std::string_view text);
Response progress_bar(Context& ctx, float fraction);
Response image(Context& ctx, TextureId tex, Vec2 size, Rect uv = {0, 0, 1, 1});
bool collapsing_header(Context& ctx, std::string_view label, bool& open);
bool combo(Context& ctx, std::string_view label, int& selected, const char* const* items, int count);
void tooltip(Context& ctx, std::string_view text);  ///< shown while the last widget is hovered
/// One popup can be open at a time. It anchors below the last widget.
bool popup_begin(Context& ctx, std::string_view id);
void popup_end(Context& ctx);
void open_popup(Context& ctx, std::string_view id);
void push_id(Context& ctx, std::string_view id);
void pop_id(Context& ctx);

}  // namespace rd
