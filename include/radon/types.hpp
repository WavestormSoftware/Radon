// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace rd {

using ID = uint32_t;
using FontHandle = int;      ///< Index into the context font table. -1 means "none".
using TextureId = uint32_t;  ///< 0 is the glyph atlas (default). Other values are backend textures.

struct Vec2 {
    float x = 0, y = 0;
};
struct Vec4 {
    float x = 0, y = 0, z = 0, w = 0;
};
struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(Vec2 p) const { return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h; }
};
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
};

enum Key : int {
    KeyNone = 0,
    KeyEnter,
    KeyBackspace,
    KeyDelete,
    KeyLeft,
    KeyRight,
    KeyUp,
    KeyDown,
    KeyHome,
    KeyEnd,
    KeyTab,
    KeyEscape,
    KeySpace,
    KeyA,
    KeyC,
    KeyV,
    KeyX,
};

struct KeyEvent {
    int key = KeyNone;
    bool shift = false, ctrl = false, alt = false;
};

/// Fixed-capacity queue. Pushing onto a full queue returns false and drops the item.
template <typename T, int N>
struct FixedQueue {
    T items[N]{};
    int count = 0;
    bool push(const T& v) {
        if (count >= N) return false;
        items[count++] = v;
        return true;
    }
    void clear() { count = 0; }
};
using KeyEvents = FixedQueue<KeyEvent, 16>;
using TextEvents = FixedQueue<char, 256>;  ///< raw UTF-8 bytes

/// Filled by the platform backend once per frame.
struct Input {
    Vec2 pointer{};                  ///< logical pixels, top-left origin
    bool pointer_down = false;       ///< held this frame
    bool pointer_pressed = false;    ///< went down this frame
    bool pointer_released = false;   ///< went up this frame
    bool secondary_pressed = false;  ///< right mouse button went down this frame (context menus)
    bool touch = false;              ///< pointer comes from a finger (no hover, scroll by drag)
    float wheel = 0;                 ///< vertical scroll, logical lines (positive = content moves down)
    KeyEvents keys;
    TextEvents text;
    Vec2 screen_size{};  ///< window size in logical px (used to center modals)
    Rect safe_area{};    ///< mobile insets in logical px: x=left, y=top, w=right, h=bottom; zero on desktop
    /// The screen minus the safe-area insets: where UI can be placed without hitting notches or bars.
    Rect safe_rect() const {
        return {safe_area.x, safe_area.y, screen_size.x - safe_area.x - safe_area.w,
                screen_size.y - safe_area.y - safe_area.h};
    }
};

/// Mouse cursor the UI asks for this frame (see Context::cursor). Backends map it to a system cursor.
enum class Cursor { Arrow, Text, Hand, ResizeEW, ResizeNS, ResizeNWSE };

/// Semantic color of alerts and toasts.
enum class Tone { Info, Success, Warning, Error };

struct Response {
    bool hovered = false;
    bool focused = false;
    bool pressed = false;  ///< became active this frame
    bool clicked = false;  ///< released over the same widget this frame
    bool changed = false;  ///< value changed this frame (value widgets)
};

/// Non-owning editable text. The caller owns `data`; `capacity` includes the NUL terminator.
struct TextBuffer {
    char* data = nullptr;
    int capacity = 0;
    int length = 0;
};

/// Clipboard hooks supplied by the backend. `get` copies up to `capacity` bytes and returns the count.
using ClipboardGet = int (*)(void* user, char* out, int capacity);
using ClipboardSet = void (*)(void* user, const char* text, int length);

}  // namespace rd
