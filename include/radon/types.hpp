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
