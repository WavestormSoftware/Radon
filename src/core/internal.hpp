// Copyright (c) 2026 Wavestorm Software
// Internal state shared by the core sources. Not a public header.
#pragma once
#include <radon/radon.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>
// static: raylib exports its own stbrp_*; keep Radon's copy private so both link
#define STBRP_STATIC
#include <stb_rect_pack.h>
#include <stb_truetype.h>

namespace rd::detail {

constexpr int kMaxCols = 16;
constexpr int kMaxClip = 32;
constexpr int kMaxWinStack = 8;
constexpr float kTouchSlop = 8.0f;
constexpr uint32_t kOverlayLayer = 1u << 24;
constexpr uint32_t kTooltipLayer = kOverlayLayer + 4096;
constexpr int kMaxChildStack = 8;
constexpr int kMaxThemeStack = 8;
constexpr size_t kArenaSize = 1u << 20;
constexpr int kMaxAlpha = 8;
constexpr int kMaxToasts = 4;
constexpr int kMaxUndo = 32;
constexpr int kDndBytes = 256;
constexpr uint32_t kWindowDocked = 1u << 30;  // internal window flag: docked panel (z fixed at 0)

struct Layout {
    Rect body;
    Vec2 pos;
    Vec2 max;
    float widths[kMaxCols];
    int items = 1;
    int index = 1;
    float row_h = 0;
    float next_row = 0;
    int mode = 0;      // LayoutRows, LayoutFlow (wrap) or LayoutStrip (one endless row)
    float flow_w = 0;  // flow/strip: width of a cell from plain layout_next
};
enum { LayoutRows = 0, LayoutFlow = 1, LayoutStrip = 2 };

struct WindowState {
    ID id = 0;
    Rect rect;
    int z = 0;
    float scroll = 0, vel = 0;
    float content_h = 0;
    uint32_t last_frame = 0;
    bool open = true, used = false, overlay = false;
};

struct WindowFrame {
    int slot = 0;
    ID id = 0;
    ID prev_window = 0;
    uint32_t prev_layer = 0;
    uint32_t flags = 0;
    Rect content;
    int layout_index = 0, clip_base = 0;
    bool overlay = false;
    float prev_alpha = 1;
};

struct TextEdit {
    ID id = 0;
    int cursor = 0, anchor = 0;
    float scroll = 0, scroll_y = 0;
    uint32_t last_frame = 0;
    int word_lo = -1, word_hi = -1;  // word-wise drag after a double-click or long-press
    int handle = 0;                  // touch selection handle being dragged: 1 start, 2 end
    float kb = 0;                    // keyboard inset when the field last checked it
};

/// Snapshots of one text buffer (undo or redo) in a fixed byte arena; the oldest drop off.
struct UndoStack {
    char* mem = nullptr;
    int cap = 0, used = 0, n = 0;
    struct Entry {
        int off, len, cursor;
    } e[kMaxUndo];
    void clear() { used = n = 0; }
    void push(const char* p, int len, int cursor) {
        if (len > cap) return clear();
        while (n > 0 && (used + len > cap || n == kMaxUndo)) {  // drop the oldest
            const int d = e[0].len;
            std::memmove(mem, mem + d, (size_t)(used - d));
            used -= d;
            for (int i = 1; i < n; i++) e[i - 1] = {e[i].off - d, e[i].len, e[i].cursor};
            n--;
        }
        std::memcpy(mem + used, p, (size_t)len);
        e[n++] = {used, len, cursor};
        used += len;
    }
    void pop() {
        if (n > 0) used = e[--n].off;
    }
};

struct ChildState {
    ID id = 0;
    float scroll = 0, content_h = 0;
    uint32_t last_frame = 0;
    float vel = 0;
    int lock = 0;      // carousel drag direction: 0 undecided, 1 horizontal, 2 vertical (not ours)
    float target = 0;  // carousel: where the strip snaps to
};

enum { ChildScroll = 0, ChildCard = 1, ChildAccordion = 2, ChildStrip = 3 };
struct ChildFrame {
    int slot = 0, layout_index = 0, clip_base = 0;
    Rect rect;
    float pad = 0;
    int kind = ChildScroll;
    float extra = 0;  // carousel: item width
};

struct DockNode {
    ID dock = 0;                          // the dock_space it belongs to
    bool used = false, vertical = false;  // split: children side by side, or stacked when vertical
    int parent = -1, child[2] = {-1, -1};
    float ratio = 0.5f;
    ID selected = 0;  // leaf: the visible tab
    int main = -1;    // root only: the leaf that "Center" panels go to
    Rect rect, body;  // body = below the tab strip
};

struct DockPanel {
    ID id = 0, dock = 0;
    int node = -1;  // leaf node, or -1 when floating
    Rect float_rect;
    char title[48];
    uint32_t last_frame = 0, order = 0;
    bool closable = false, close_clicked = false;
};

struct PickerState {
    ID id = 0;
    float h = 0, s = 0, v = 0;
    uint32_t last_rgb = 0, last_frame = 0;
};

struct Anim {
    ID id = 0;
    float hover = 0, press = 0;  // anim_to keeps its value in `hover`
    uint32_t last_frame = 0;
    int key = 0;  // transition_begin: the key the content was last shown for
};

struct Toast {
    char text[96];
    Tone tone = Tone::Info;
    float age = 0, duration = 0;  // duration 0 = free slot
};

struct FontSlot {
    const uint8_t* data = nullptr;
    stbtt_fontinfo info;
    float pixel_size = 0, scale = 0;  // scale: font units -> logical pixels
    float ascent = 0, descent = 0, line_gap = 0;
    FontHandle fallback = -1;  // where glyphs missing from this font come from
};

struct Glyph {
    FontHandle font = -1;
    uint32_t cp = 0;
    bool used = false;
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;                 // atlas uv of the bitmap
    float xoff = 0, yoff = 0, w = 0, h = 0, advance = 0;  // logical px
};

struct Arena {
    uint8_t* mem = nullptr;
    size_t used = 0;
};

struct State {
    explicit State(const Config& c);

    Config cfg;
    Input in;
    Vec2 prev_pointer, pointer_delta;
    float dt = 0, blink = 0;
    uint32_t frame = 0;
    bool error = false, warning = false;

    std::unique_ptr<ID[]> id_stack;
    int id_n = 0;

    ID hover = 0, focus = 0, active = 0, hover_window = 0, focus_window = 0, cur_window = 0;
    int clicks = 0;  // 1, 2 (double) or 3 (triple) for the press this frame
    float last_press_time = -10;
    Vec2 last_press_pos;
    Haptic haptic = Haptic::None;
    TextInputType text_type = TextInputType::Text;
    Gesture gesture;
    Touch prev_touches[kMaxTouches];
    int prev_touch_count = 0;
    bool edge_fired = false;
    float sheet_drag = 0;  // drawer swipe-to-dismiss offset
    int sheet_lock = 0;    // 0 undecided, 1 dragging the sheet, 2 not ours
    // drag and drop
    bool dnd_active = false, dnd_delivered = false;
    ID dnd_source = 0;
    char dnd_type[32];
    uint8_t dnd_data[kDndBytes];
    int dnd_size = 0;
    char dnd_preview[64];
    // undo for the focused text box
    std::unique_ptr<char[]> undo_mem;
    UndoStack undo, redo;
    ID undo_id = 0;
    bool undo_typing = false;
    // docking
    std::unique_ptr<DockNode[]> dock_nodes;
    std::unique_ptr<DockPanel[]> dock_panels;
    ID dock_cur = 0;
    int dock_cur_root = -1, dock_main = -1;
    uint32_t dock_order = 0;
    ID dock_dragging = 0;  // floating panel whose title is being dragged (last frame)
    int dock_target = -1, dock_zone = -1;
    // accessibility
    std::unique_ptr<A11yNode[]> a11y;
    int a11y_n = 0;
    // filter text of the open combo_search popup
    char search[64];
    int search_len = 0;
    ID press_window = 0;
    bool updated_focus = false, press_claimed = false, kbd_nav = false, active_captures = false,
         touch_moved = false;
    Vec2 press_pos;
    int tab_dir = 0;
    ID last_focusable = 0, first_focusable = 0;

    ID last_id = 0;
    Rect last_rect;
    bool last_hovered = false;

    std::unique_ptr<WindowState[]> wins;
    WindowFrame wstack[kMaxWinStack];
    int wstack_n = 0;
    int z_counter = 0;
    uint32_t cur_layer = 0;

    Rect clips[kMaxClip];
    int clip_n = 0;

    std::unique_ptr<Layout[]> layouts;
    int layout_n = 0, skipped_cols = 0;

    ID popup_id = 0, popup_suppress = 0;
    Rect popup_anchor;
    uint32_t popup_open_frame = 0;
    bool popup_seen = false;

    char tip[128];
    Vec2 tip_pos;
    bool tip_set = false;

    std::unique_ptr<TextEdit[]> edits;
    bool wants_text = false, wants_text_next = false;
    ClipboardGet clip_get = nullptr;
    ClipboardSet clip_set = nullptr;
    void* clip_user = nullptr;

    std::unique_ptr<Anim[]> anims;

    std::unique_ptr<ChildState[]> children;
    ChildFrame cstack[kMaxChildStack];
    int cstack_n = 0;
    bool wheel_used = false, drag_used = false;
    float time = 0, drag_acc = 0;
    PickerState pickers[8];
    Theme theme_stack[kMaxThemeStack];
    int theme_n = 0;
    ID modal_id = 0;
    uint32_t modal_open_frame = 0;
    bool modal_seen = false, popup_is_menu = false, in_menu = false;
    Rect menu_bar;
    float menu_x = 0;

    float alpha = 1;  // current fade (popup transitions, disabled sections)
    float alpha_stack[kMaxAlpha];
    bool disabled_stack[kMaxAlpha];
    int alpha_n = 0, disabled = 0;
    bool animating = false, secondary = false, long_fired = false;
    float press_time = 0;
    Cursor cursor = Cursor::Arrow;
    Rect text_rect;
    Toast toasts[kMaxToasts];
    float toast_offset = 0;
    float tr_shift[kMaxAlpha];  // transition_begin slide offsets
    int tr_n = 0;
    float clipper_rest = 0;  // list_clipper: space to reserve after the visible rows

    std::unique_ptr<FontSlot[]> fonts;
    int font_n = 0;
    FontHandle default_font = -1;
    std::unique_ptr<uint8_t[]> atlas;
    std::unique_ptr<stbrp_node[]> nodes;
    stbrp_context pack;
    std::unique_ptr<Glyph[]> glyphs;
    int glyph_n = 0;
    uint32_t atlas_ver = 1;
    bool atlas_full = false;
    std::unique_ptr<uint8_t[]> arena_mem;
    Arena arena;

    DrawList dl;
};

// ids.cpp
ID hash_u32(ID seed, uint32_t v);
ID top_id(const State& s);
ID get_id(State& s, std::string_view label);
std::string_view visible_label(std::string_view label);

// context.cpp
Response update_control(Context& ctx, ID id, const Rect& r, bool focusable, bool captures);
bool key_pressed(const State& s, int key, bool* shift = nullptr, bool* ctrl = nullptr);
void push_clip(State& s, const Rect& r, bool absolute = false);
void pop_clip(State& s);
Color lerp(Color a, Color b, float t);
Color with_alpha(Color c, float a);
float animate(State& s, ID id, bool hover, bool press, float* press_out = nullptr);
/// Eases a per-ID value toward `target` (about `duration` seconds). A value that was not animated on
/// the previous frame restarts at `from`. Safe to call more than once per frame.
float anim_to(State& s, ID id, float target, float duration, float from);
Anim* anim_slot(State& s, ID id, bool& fresh);
/// Asks the backend for haptic feedback this frame (the strongest request wins).
inline void haptic(State& s, Haptic h) {
    if (h > s.haptic) s.haptic = h;
}
/// The innermost scroll container: a child region (child_slot >= 0) or the current window. Returns
/// the scroll offset to adjust and its visible rect, or null outside any window.
float* scroll_target(State& s, Rect* view);
bool push_alpha(State& s, float mul);
void pop_alpha(State& s);
/// Touch flick scrolling: tracks the drag velocity and keeps scrolling after release.
void kinetic(State& s, float& scroll, float& vel, bool dragging, float max_scroll);
Rect intersect(const Rect& a, const Rect& b);

// layout.cpp
Layout& top_layout(State& s);
/// The next cell with a known width: wraps in flow layouts, else same as layout_next.
Rect layout_next_w(Context& ctx, float w);
bool in_flow(const State& s);
bool push_layout(State& s, const Rect& body);
void extend_height(State& s, Rect& r, float needed);

// text.cpp
const FontSlot* font_of(const State& s, FontHandle f);
Vec2 text_size(const State& s, FontHandle f, std::string_view text);
void draw_text(State& s, FontHandle f, Vec2 top_left, std::string_view text, Color c);
void draw_text_centered(State& s, FontHandle f, const Rect& r, std::string_view text, Color c, bool center_x);
void reset_atlas(State& s);
float x_of_index(const State& s, FontHandle f, std::string_view text, int byte_index);
int index_at_x(const State& s, FontHandle f, std::string_view text, float x);

// window.cpp
void begin_frame_windows(State& s);
void end_frame_windows(State& s);
bool begin_window_impl(Context& ctx, ID id, std::string_view title, Rect& rect, uint32_t flags, bool overlay);

// widgets
void draw_toasts(State& s);
Response selectable(Context& ctx, std::string_view text, bool selected);
void open_popup_id(State& s, ID id);
bool popup_begin_id(Context& ctx, ID id, float min_w);
Response textbox_impl(Context& ctx, ID id, Rect r, TextBuffer& buf, const char* hint, int mode, float inset_l,
                      float inset_r);
enum {
    TextSingle = 0,
    TextPassword = 1,
    TextMulti = 2,
    TextReadOnly = 4,
    TextNumber = 8,
    TextDecimal = 16,
    TextEmail = 32,
    TextUrl = 64
};
ChildState* find_child(State& s, ID id);
TextEdit* find_edit(State& s, ID id);
bool activated(State& s, const Response& rs);
void focus_ring(State& s, const Rect& r, const Response& rs, float radius);
// a11y.cpp: describes the last widget (s.last_id / s.last_rect). No-op unless Config::a11y.
void a11y_node(State& s, A11yRole role, std::string_view label, std::string_view value = {},
               uint32_t flags = 0);
void a11y_control(State& s, ID id, const Rect& r, bool focusable);
/// A node for non-interactive content (text, headings, images).
void a11y_static(State& s, A11yRole role, const Rect& r, std::string_view label);
// chart.cpp
float nice_step(float raw);
void format_tick(char* out, size_t cap, float v, float step);
// richtext.cpp
FontHandle bold_font(const Theme& t);
// dnd.cpp
void draw_dnd_preview(State& s);
/// Largest length <= `cap` that does not cut a UTF-8 code point in half.
inline size_t utf8_fit(std::string_view t, size_t cap) {
    size_t n = t.size() < cap ? t.size() : cap;
    while (n > 0 && n < t.size() && (static_cast<unsigned char>(t[n]) & 0xC0) == 0x80) n--;
    return n;
}
/// Drops an incomplete code point at the end of `p[0..n)` (text that was cut at a byte limit).
inline size_t utf8_trim_tail(const char* p, size_t n) {
    size_t i = n;
    while (i > 0 && n - i < 3 && (static_cast<unsigned char>(p[i - 1]) & 0xC0) == 0x80) i--;
    if (i == 0) return n;
    const unsigned char c = static_cast<unsigned char>(p[i - 1]);
    const size_t need = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
    return need > n - (i - 1) ? i - 1 : n;
}
inline FontHandle small_font(const Theme& t) {
    return t.font_small >= 0 ? t.font_small : t.font_body;
}
inline float clamp01(float v) {
    return v < 0 ? 0 : (v > 1 ? 1 : v);
}
inline Color tone_color(const Theme& t, Tone tone) {
    return tone == Tone::Success
               ? t.success
               : (tone == Tone::Warning ? t.warning : (tone == Tone::Error ? t.error : t.accent));
}

}  // namespace rd::detail

namespace rd::detail {

/// Fill color for a pressable control, blended by the hover/press animation.
inline Color control_color(State& s, ID id, const Response& rs) {
    float press = 0;
    float h = animate(s, id, rs.hovered, s.active == id, &press);
    const Theme& t = s.cfg.theme;
    return lerp(t.surface_hover, t.surface_active, 0.5f * h + 0.5f * press);
}

inline void draw_frame(State& s, const Rect& r, Color fill, bool focused, bool always_ring = false) {
    const Theme& t = s.cfg.theme;
    s.dl.rect(r, fill, t.corner_radius);
    bool ring = focused && (s.kbd_nav || always_ring);
    s.dl.rect(r, ring ? t.focus_ring : t.border, t.corner_radius, ring ? t.focus_ring_width : t.border_width);
}

/// Draws text vertically centered in `r`, clipped to `r` only when it would overflow.
inline void draw_text_fit(State& s, const Rect& r, std::string_view text, Color c, bool center_x) {
    const Theme& t = s.cfg.theme;
    Vec2 sz = text_size(s, t.font_body, text);
    bool clip = sz.x > r.w;
    if (clip) push_clip(s, r);
    draw_text_centered(s, t.font_body, r, text, c, center_x && !clip);
    if (clip) pop_clip(s);
}

}  // namespace rd::detail
