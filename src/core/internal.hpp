// Copyright (c) 2026 Wavestorm Software
// Internal state shared by the core sources. Not a public header.
#pragma once
#include <radon/radon.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>
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

struct Layout {
    Rect body;
    Vec2 pos;
    Vec2 max;
    float widths[kMaxCols];
    int items = 1;
    int index = 1;
    float row_h = 0;
    float next_row = 0;
};

struct WindowState {
    ID id = 0;
    Rect rect;
    int z = 0;
    float scroll = 0;
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
};

struct TextEdit {
    ID id = 0;
    int cursor = 0, anchor = 0;
    float scroll = 0;
    uint32_t last_frame = 0;
};

struct ChildState {
    ID id = 0;
    float scroll = 0, content_h = 0;
    uint32_t last_frame = 0;
};

struct ChildFrame {
    int slot = 0, layout_index = 0, clip_base = 0;
    Rect rect;
    float pad = 0;
};

struct PickerState {
    ID id = 0;
    float h = 0, s = 0, v = 0;
    uint32_t last_rgb = 0, last_frame = 0;
};

struct Anim {
    ID id = 0;
    float hover = 0, press = 0;
    uint32_t last_frame = 0;
};

struct FontSlot {
    const uint8_t* data = nullptr;
    stbtt_fontinfo info;
    float pixel_size = 0, scale = 0;  // scale: font units -> logical pixels
    float ascent = 0, descent = 0, line_gap = 0;
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
    bool wheel_used = false;
    float time = 0, drag_acc = 0;
    PickerState pickers[8];
    Theme theme_stack[kMaxThemeStack];
    int theme_n = 0;
    ID modal_id = 0;
    uint32_t modal_open_frame = 0;
    bool modal_seen = false, popup_is_menu = false, in_menu = false;
    Rect menu_bar;
    float menu_x = 0;

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
Rect intersect(const Rect& a, const Rect& b);

// layout.cpp
Layout& top_layout(State& s);
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
Response selectable(Context& ctx, std::string_view text, bool selected);
void open_popup_id(State& s, ID id);
bool popup_begin_id(Context& ctx, ID id, float min_w);

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
