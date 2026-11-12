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

/// Per-corner radii (top-left, top-right, bottom-right, bottom-left). A single float converts to all four.
struct Corners {
    float tl = 0, tr = 0, br = 0, bl = 0;
    Corners() = default;
    Corners(float r) : tl(r), tr(r), br(r), bl(r) {}
    Corners(float tl_, float tr_, float br_, float bl_) : tl(tl_), tr(tr_), br(br_), bl(bl_) {}
    static Corners top(float r) { return {r, r, 0, 0}; }
    static Corners bottom(float r) { return {0, 0, r, r}; }
    float max() const {
        float m = tl;
        for (float v : {tr, br, bl}) m = v > m ? v : m;
        return m;
    }
};

/// A fill: a flat color or a two-color gradient. A `Color` converts to a solid Paint, so anywhere a
/// Paint is expected you can pass a plain color.
struct Paint {
    enum Kind : uint8_t { Solid, Linear, Radial };
    Kind kind = Solid;
    Color a{0, 0, 0, 0}, b{0, 0, 0, 0};  ///< solid: a. gradients: a at the start / center, b at the end / edge
    float angle = 90;                     ///< linear: degrees, 0 = left to right, 90 = top to bottom
    Vec2 center{0.5f, 0.5f};              ///< radial: center as a fraction of the shape
    float radius = 0.5f;                  ///< radial: radius as a fraction of the shape's larger side
    Paint() = default;
    Paint(Color c) : a(c), b(c) {}
    static Paint linear(Color from, Color to, float angle_deg = 90) {
        Paint p;
        p.kind = Linear, p.a = from, p.b = to, p.angle = angle_deg;
        return p;
    }
    static Paint radial(Color inner, Color outer, Vec2 center = {0.5f, 0.5f}, float radius = 0.5f) {
        Paint p;
        p.kind = Radial, p.a = inner, p.b = outer, p.center = center, p.radius = radius;
        return p;
    }
    /// True for the default Paint; themes use it to mean "fall back to the plain color".
    bool empty() const { return kind == Solid && a.a == 0; }
};

struct Shadow {
    Color color{0, 0, 0, 0};  ///< alpha 0 = no shadow
    float blur = 0;
    Vec2 offset;
    float spread = 0;  ///< grows the shadow shape on every side
};

/// Everything needed to draw a modern box in one call: see `rd::box`.
struct Style {
    Paint fill;
    Paint border_paint;
    float border = 0;  ///< border width, 0 = none
    Corners radius;
    Shadow shadow;
    float opacity = 1;
    float backdrop_blur = 0;  ///< > 0 frosts what is behind the box (blur radius in px, up to ~32); the fill tints it
};

/// Interpolation for animating styles: `rd::blend(base, hover, rd::animate(ui, "card", hovered))`.
inline Color blend(Color a, Color b, float t) {
    auto m = [t](uint8_t x, uint8_t y) { return (uint8_t)((float)x + ((float)y - (float)x) * t + 0.5f); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}
inline float blend(float a, float b, float t) {
    return a + (b - a) * t;
}
inline Corners blend(const Corners& a, const Corners& b, float t) {
    return {blend(a.tl, b.tl, t), blend(a.tr, b.tr, t), blend(a.br, b.br, t), blend(a.bl, b.bl, t)};
}
/// A solid Paint blends smoothly into a gradient (its end color equals its start color).
inline Paint blend(const Paint& a, const Paint& b, float t) {
    Paint p;
    p.kind = a.kind == Paint::Solid ? b.kind : a.kind;
    p.a = blend(a.a, b.a, t);
    p.b = blend(a.b, b.b, t);
    p.angle = blend(a.angle, b.angle, t);
    p.center = {blend(a.center.x, b.center.x, t), blend(a.center.y, b.center.y, t)};
    p.radius = blend(a.radius, b.radius, t);
    return p;
}
inline Style blend(const Style& a, const Style& b, float t) {
    Style s;
    s.fill = blend(a.fill, b.fill, t);
    s.border_paint = blend(a.border_paint, b.border_paint, t);
    s.border = blend(a.border, b.border, t);
    s.radius = blend(a.radius, b.radius, t);
    s.shadow = {blend(a.shadow.color, b.shadow.color, t), blend(a.shadow.blur, b.shadow.blur, t),
                {blend(a.shadow.offset.x, b.shadow.offset.x, t), blend(a.shadow.offset.y, b.shadow.offset.y, t)},
                blend(a.shadow.spread, b.shadow.spread, t)};
    s.opacity = blend(a.opacity, b.opacity, t);
    s.backdrop_blur = blend(a.backdrop_blur, b.backdrop_blur, t);
    return s;
}

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
    KeyZ,  ///< Ctrl+Z undo, Ctrl+Shift+Z redo
    KeyY,  ///< Ctrl+Y redo
    KeyPageUp,
    KeyPageDown,
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

/// One finger on a touch screen.
struct Touch {
    uint64_t id = 0;  ///< stable while the finger stays down
    Vec2 pos{};       ///< logical pixels
};
constexpr int kMaxTouches = 10;

/// What a screen reader asks a widget to do (see Context::a11y_nodes).
enum class A11yAction { None, Activate, Focus, Increment, Decrement };

/// Filled by the platform backend once per frame.
struct Input {
    Vec2 pointer{};                  ///< logical pixels, top-left origin; the first finger on touch screens
    bool pointer_down = false;       ///< held this frame
    bool pointer_pressed = false;    ///< went down this frame
    bool pointer_released = false;   ///< went up this frame
    bool secondary_pressed = false;  ///< right mouse button went down this frame (context menus)
    bool touch = false;              ///< pointer comes from a finger (no hover, scroll by drag)
    float wheel = 0;                 ///< vertical scroll, logical lines (positive = content moves down)
    KeyEvents keys;
    TextEvents text;
    /// Every finger currently down (multi-touch gestures); `pointer` follows touches[0]. Empty on desktop.
    Touch touches[kMaxTouches];
    int touch_count = 0;
    /// Logical px at the bottom of the screen covered by the soft keyboard (0 when hidden).
    float keyboard_inset = 0;
    /// A screen reader request for widget `a11y_target` (an A11yNode::id). Cleared by the backend each frame.
    A11yAction a11y_action = A11yAction::None;
    uint32_t a11y_target = 0;
    Vec2 screen_size{};  ///< window size in logical px (used to center modals)
    bool window_maximized = false;  ///< the OS window is maximized (the titlebar shows Restore, corners go square)
    bool window_focused = true;     ///< the OS window has keyboard focus (the titlebar dims when not)
    Rect safe_area{};    ///< mobile insets in logical px: x=left, y=top, w=right, h=bottom; zero on desktop
    /// The screen minus the safe-area insets: where UI can be placed without hitting notches or bars.
    Rect safe_rect() const {
        return {safe_area.x, safe_area.y, screen_size.x - safe_area.x - safe_area.w,
                screen_size.y - safe_area.y - safe_area.h};
    }
};

/// Mouse cursor the UI asks for this frame (see Context::cursor). Backends map it to a system cursor.
enum class Cursor { Arrow, Text, Hand, ResizeEW, ResizeNS, ResizeNWSE, ResizeNESW };

/// What the custom titlebar asks the host to do with the OS window (see Context::window_command).
enum class WindowCommand { None, Minimize, Maximize, Restore, Close };

/// Where the OS window may be dragged by (see Context::window_chrome): the caption minus the interactive
/// controls inside it. Backends feed this to their native hit test.
struct WindowChrome {
    Rect caption;
    Rect no_drag[16];
    int no_drag_count = 0;
};

/// Semantic color of alerts and toasts.
enum class Tone { Info, Success, Warning, Error };

/// Which soft keyboard a focused text field wants (see Context::text_input_type).
enum class TextInputType { Text, Number, Decimal, Email, Url, Password };

/// Haptic feedback the UI asks for this frame (see Context::haptic). Ordered by strength.
enum class Haptic { None, Selection, Light, Medium };

/// Direction of a swipe gesture.
enum class SwipeDir { None, Left, Right, Up, Down };

/// Multi-touch gesture state for this frame (Context::gesture).
struct Gesture {
    int fingers = 0;   ///< fingers down now
    Vec2 center{};     ///< centroid of the fingers (or the pointer), logical px
    float pinch = 1;   ///< scale change this frame from two fingers spreading (>1) or closing (<1)
    float rotate = 0;  ///< rotation this frame from two fingers, radians (clockwise positive)
    Vec2 pan{};        ///< movement of the two-finger centroid this frame
    SwipeDir swipe = SwipeDir::None;  ///< set on the frame a fast one-finger flick ends
    Vec2 velocity{};                  ///< pointer velocity, logical px per second
};

/// Role of a node in the accessibility tree.
enum class A11yRole {
    Unknown,
    Window,
    Button,
    Checkbox,
    Radio,
    Switch,
    Slider,
    TextField,
    ComboBox,
    ListItem,
    Tab,
    Link,
    Text,
    Heading,
    Image,
    ProgressBar,
    TreeItem,
    MenuItem,
    Table,
    Row
};

/// One accessible element, recorded each frame when Config::a11y is on.
struct A11yNode {
    enum Flags : uint32_t {
        Focusable = 1,
        Focused = 2,
        Checked = 4,
        Selected = 8,
        Disabled = 16,
        Expanded = 32,
        Collapsed = 64
    };
    uint32_t id = 0;      ///< widget ID; pass back as Input::a11y_target
    uint32_t parent = 0;  ///< ID of the window it is in (0 = none)
    A11yRole role = A11yRole::Unknown;
    Rect rect;            ///< logical px
    char label[48] = {};  ///< UTF-8, may be cut
    char value[32] = {};  ///< current value as text ("0.50", "on", the text of a field)
    uint32_t flags = 0;
};

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
