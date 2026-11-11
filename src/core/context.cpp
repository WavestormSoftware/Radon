// Copyright (c) 2026 Wavestorm Software
#include "internal.hpp"

namespace rd {
namespace detail {

State::State(const Config& c)
    : cfg(c),
      id_stack(new ID[c.max_id_stack]),
      wins(new WindowState[c.max_windows]),
      layouts(new Layout[c.max_layout_stack]),
      edits(new TextEdit[c.max_textboxes]),
      anims(new Anim[c.max_anims]),
      children(new ChildState[c.max_children]),
      fonts(new FontSlot[c.max_fonts]),
      atlas(new uint8_t[(size_t)c.atlas_size * c.atlas_size]),
      nodes(new stbrp_node[c.atlas_size]),
      glyphs(new Glyph[c.max_glyphs]),
      arena_mem(new uint8_t[kArenaSize]),
      dl(c.max_draw_commands, c.max_vertices, c.max_indices) {
    tip[0] = 0;
    arena.mem = arena_mem.get();
    reset_atlas(*this);
}

Color lerp(Color a, Color b, float t) {
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    auto m = [t](uint8_t x, uint8_t y) { return (uint8_t)(x + (y - x) * t + 0.5f); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}

Color with_alpha(Color c, float a) {
    c.a = (uint8_t)(c.a * (a < 0 ? 0 : (a > 1 ? 1 : a)) + 0.5f);
    return c;
}

Rect intersect(const Rect& a, const Rect& b) {
    float x0 = std::fmax(a.x, b.x), y0 = std::fmax(a.y, b.y);
    float x1 = std::fmin(a.x + a.w, b.x + b.w), y1 = std::fmin(a.y + a.h, b.y + b.h);
    return {x0, y0, std::fmax(0.0f, x1 - x0), std::fmax(0.0f, y1 - y0)};
}

void push_clip(State& s, const Rect& r, bool absolute) {
    if (s.clip_n >= kMaxClip) {
        s.error = true;
        return;
    }
    Rect c = (absolute || s.clip_n == 0) ? r : intersect(s.clips[s.clip_n - 1], r);
    s.clips[s.clip_n++] = c;
    s.dl.set_clip(c);
}

void pop_clip(State& s) {
    if (s.clip_n > 0) s.clip_n--;
    s.dl.set_clip(s.clip_n ? s.clips[s.clip_n - 1] : Rect{-1e6f, -1e6f, 2e6f, 2e6f});
}

bool key_pressed(const State& s, int key, bool* shift, bool* ctrl) {
    for (int i = 0; i < s.in.keys.count; i++) {
        const KeyEvent& k = s.in.keys.items[i];
        if (k.key == key) {
            if (shift) *shift = k.shift;
            if (ctrl) *ctrl = k.ctrl;
            return true;
        }
    }
    return false;
}

Anim* anim_slot(State& s, ID id, bool& fresh) {
    Anim* oldest = &s.anims[0];
    for (int i = 0; i < s.cfg.max_anims; i++) {
        Anim& a = s.anims[i];
        if (a.id == id) {
            fresh = a.last_frame + 1 < s.frame;  // not shown last frame: restart
            return &a;
        }
        if (a.last_frame < oldest->last_frame) oldest = &a;
    }
    if (oldest->last_frame + 1 >= s.frame && oldest->id != 0) {
        s.error = true;  // every slot is in use this frame; skip the blend
        return nullptr;
    }
    *oldest = Anim{};
    oldest->id = id;
    fresh = true;
    return oldest;
}

float animate(State& s, ID id, bool hover, bool press, float* press_out) {
    float th = hover ? 1.0f : 0.0f, tp = press ? 1.0f : 0.0f;
    bool fresh = false;
    Anim* slot = s.cfg.animations ? anim_slot(s, id, fresh) : nullptr;
    if (!slot) {
        if (press_out) *press_out = tp;
        return th;
    }
    if (fresh) slot->hover = th, slot->press = tp;
    if (slot->last_frame != s.frame) {
        float k = std::fmin(1.0f, s.dt / 0.12f);
        slot->hover += (th - slot->hover) * k;
        slot->press += (tp - slot->press) * k;
        if (std::fabs(th - slot->hover) < 0.004f) slot->hover = th;
        if (std::fabs(tp - slot->press) < 0.004f) slot->press = tp;
        slot->last_frame = s.frame;
    }
    if (slot->hover != th || slot->press != tp) s.animating = true;
    if (press_out) *press_out = slot->press;
    return slot->hover;
}

float anim_to(State& s, ID id, float target, float duration, float from) {
    bool fresh = false;
    Anim* a = (s.cfg.animations && duration > 0) ? anim_slot(s, id, fresh) : nullptr;
    if (!a) return target;
    if (fresh) a->hover = from;
    if (a->last_frame != s.frame) {
        a->hover += (target - a->hover) * (1.0f - std::exp(-s.dt * 5.0f / duration));
        if (std::fabs(target - a->hover) < 0.004f) a->hover = target;
        a->last_frame = s.frame;
    }
    if (a->hover != target) s.animating = true;
    return a->hover;
}

bool push_alpha(State& s, float mul) {
    if (s.alpha_n >= kMaxAlpha) {
        s.error = true;
        return false;
    }
    s.alpha_stack[s.alpha_n++] = s.alpha;
    s.alpha *= mul;
    s.dl.set_alpha(s.alpha);
    return true;
}

void pop_alpha(State& s) {
    if (s.alpha_n > 0) s.alpha = s.alpha_stack[--s.alpha_n];
    s.dl.set_alpha(s.alpha);
}

void kinetic(State& s, float& scroll, float& vel, bool dragging, float maxs) {
    if (s.in.pointer_pressed) vel = 0;  // a new touch stops the glide
    if (dragging) {
        scroll -= s.pointer_delta.y;
        if (s.dt > 0) vel = 0.6f * vel + 0.4f * (-s.pointer_delta.y / s.dt);
    } else if (s.cfg.animations && !s.in.pointer_down && std::fabs(vel) > 8.0f) {
        scroll += vel * s.dt;
        vel *= std::exp(-4.0f * s.dt);
        if (scroll <= 0 || scroll >= maxs) vel = 0;
        s.animating = true;
    } else if (!s.in.pointer_down) {
        vel = 0;
    }
}

Response update_control(Context& ctx, ID id, const Rect& r, bool focusable, bool captures) {
    State& s = state(ctx);
    Response rs;
    if (s.disabled > 0) {  // disabled sections draw but never react
        s.last_id = id;
        s.last_rect = r;
        s.last_hovered = false;
        return rs;
    }
    const Vec2 p = s.in.pointer;
    const bool geo = r.contains(p) && (s.clip_n == 0 || s.clips[s.clip_n - 1].contains(p));
    const bool win_ok = s.hover_window == s.cur_window || s.active == id;
    const bool over = geo && win_ok;

    if (over && !s.in.touch && (s.active == 0 || s.active == id)) s.hover = id;
    if (s.in.pointer_pressed && over && s.active == 0) {
        s.active = id;
        s.active_captures = captures;
        s.press_claimed = true;
        rs.pressed = true;
        if (focusable) {
            s.focus = id;
            s.focus_window = s.cur_window;
            s.kbd_nav = false;
        }
    }
    if (s.active == id && !rs.pressed) {
        if (s.in.pointer_released || !s.in.pointer_down) {
            const float dx = p.x - s.press_pos.x, dy = p.y - s.press_pos.y;
            const bool near = s.in.touch && std::sqrt(dx * dx + dy * dy) <= kTouchSlop;
            rs.clicked = (over || near) && !s.touch_moved;
            s.active = 0;
            s.active_captures = false;
        }
    } else if (rs.pressed && s.in.pointer_released) {
        // press and release in the same frame (a quick tap)
        rs.clicked = true;
        s.active = 0;
        s.active_captures = false;
    }

    if (focusable && (s.focus == 0 || s.focus_window == s.cur_window)) {
        if (s.first_focusable == 0) s.first_focusable = id;
        if (s.tab_dir > 0) {
            if (s.focus == 0 || s.last_focusable == s.focus) {
                s.focus = id;
                s.focus_window = s.cur_window;
                s.tab_dir = 0;
            }
        } else if (s.tab_dir < 0) {
            if (s.focus == id && s.last_focusable != 0) {
                s.focus = s.last_focusable;
                s.tab_dir = 0;
            }
        }
        s.last_focusable = id;
    }
    if (s.focus == id) s.updated_focus = true;

    rs.hovered = s.hover == id;
    rs.focused = s.focus == id;
    s.last_id = id;
    s.last_rect = r;
    s.last_hovered = rs.hovered;
    return rs;
}

}  // namespace detail

Context::Context(const Config& cfg) : s_(new detail::State(cfg)) {}
Context::~Context() = default;

namespace detail {
State& state(Context& c) {
    return *c.s_;
}
const State& state(const Context& c) {
    return *c.s_;
}
}  // namespace detail

void Context::begin_frame(const Input& input, float dt) {
    using namespace detail;
    State& s = *s_;
    s.in = input;
    s.dt = dt;
    s.frame++;
    s.blink += dt;
    s.time += dt;
    s.wheel_used = s.drag_used = false;
    s.animating = false;
    s.cursor = Cursor::Arrow;
    s.text_rect = {};
    s.alpha = 1;
    s.alpha_n = s.disabled = s.tr_n = 0;
    s.clipper_rest = 0;
    s.cstack_n = 0;
    s.in_menu = false;
    s.error = false;
    s.warning = false;
    s.pointer_delta = input.pointer_pressed
                          ? Vec2{}
                          : Vec2{input.pointer.x - s.prev_pointer.x, input.pointer.y - s.prev_pointer.y};
    s.prev_pointer = input.pointer;
    s.dl.clear();
    s.id_n = 0;
    s.layout_n = 0;
    s.skipped_cols = 0;
    s.clip_n = 0;
    s.wstack_n = 0;
    s.cur_window = 0;
    s.tip_set = false;
    if (s.atlas_full) {
        reset_atlas(s);
        s.atlas_full = false;
    }
    begin_frame_windows(s);

    if (input.pointer_pressed) {
        s.press_pos = input.pointer;
        s.touch_moved = false;
        s.press_window = s.hover_window;
    }
    if (input.touch && input.pointer_down) {
        const float dx = input.pointer.x - s.press_pos.x, dy = input.pointer.y - s.press_pos.y;
        if (std::sqrt(dx * dx + dy * dy) > kTouchSlop) s.touch_moved = true;
    }
    // A right click, or a finger held still for half a second, is a "secondary click" (context menus).
    s.secondary = input.secondary_pressed;
    if (input.pointer_pressed) s.press_time = 0, s.long_fired = false;
    if (input.touch && input.pointer_down && !s.touch_moved && !s.long_fired) {
        s.press_time += dt;
        s.animating = true;  // keep frames coming while the hold timer runs
        if (s.press_time >= 0.5f) {
            s.secondary = s.long_fired = true;
            s.touch_moved = true;  // the release must not also click
        }
    }
    s.hover = 0;
    s.updated_focus = false;
    s.press_claimed = false;
    s.first_focusable = s.last_focusable = 0;
    s.tab_dir = 0;
    bool shift = false;
    if (key_pressed(s, KeyTab, &shift)) {
        s.tab_dir = shift ? -1 : 1;
        s.kbd_nav = true;
    }
    if (key_pressed(s, KeyEscape)) {
        if (s.popup_id)
            s.popup_id = 0;
        else if (s.modal_id)
            s.modal_id = 0;
    }
    s.modal_seen = false;
    if ((input.pointer_pressed || input.secondary_pressed) && s.popup_id && s.hover_window != s.popup_id) {
        s.popup_suppress = s.popup_id;
        s.popup_id = 0;
    }
    s.popup_seen = false;
    s.wants_text = s.wants_text_next;
    s.wants_text_next = false;
}

void Context::end_frame() {
    using namespace detail;
    State& s = *s_;
    if (s.tab_dir != 0) {
        s.focus = s.tab_dir > 0 ? s.first_focusable : s.last_focusable;
        s.updated_focus = true;
    }
    if (!s.updated_focus) s.focus = 0;
    if (s.in.pointer_pressed && !s.press_claimed) s.focus = 0;
    if (!s.in.pointer_down) {
        s.active = 0;
        s.active_captures = false;
    }
    if (s.in.pointer_released || !s.in.pointer_down) s.popup_suppress = 0;
    if (s.popup_id && !s.popup_seen && s.frame > s.popup_open_frame + 1) s.popup_id = 0;
    if (s.modal_id && !s.modal_seen && s.frame > s.modal_open_frame + 1) s.modal_id = 0;
    if (s.layout_n != 0 || s.wstack_n != 0 || s.cstack_n != 0) s.error = true;  // unbalanced begin/end
    if (s.alpha_n != 0 || s.disabled != 0 || s.tr_n != 0) s.error = true;  // unbalanced disabled/transition
    s.alpha = 1;
    s.dl.set_alpha(1);
    if (s.in.pointer_down || s.in.pointer_released || s.in.wheel != 0 || s.in.keys.count || s.in.text.count)
        s.animating = true;
    while (s.theme_n > 0) {  // unbalanced push_theme
        s.cfg.theme = s.theme_stack[--s.theme_n];
        s.error = true;
    }

    if (s.tip_set) {
        const Theme& t = s.cfg.theme;
        Vec2 sz = text_size(s, t.font_body, s.tip);
        Rect r{s.tip_pos.x, s.tip_pos.y, sz.x + 2 * t.padding, sz.y + t.padding};
        s.dl.set_layer(kTooltipLayer);
        s.dl.break_command();
        s.dl.set_clip({-1e6f, -1e6f, 2e6f, 2e6f});
        // keep the tip on screen
        if (s.in.screen_size.x > 0) r.x = std::fmax(0.0f, std::fmin(r.x, s.in.screen_size.x - r.w));
        if (s.in.screen_size.y > 0 && r.y + r.h > s.in.screen_size.y) r.y = s.tip_pos.y - 18 - r.h - 6;
        s.dl.set_alpha(anim_to(s, 0x7171u, 1, 0.12f, 0));
        s.dl.shadow(r, t.shadow, t.corner_radius, 8, {0, 3});
        s.dl.rect(r, t.surface_active, t.corner_radius);
        s.dl.rect(r, t.border, t.corner_radius, t.border_width);
        draw_text(s, t.font_body, {r.x + t.padding, r.y + t.padding / 2}, s.tip, t.text);
        s.dl.set_alpha(1);
    }
    draw_toasts(s);
    end_frame_windows(s);
    s.dl.sort_by_layer();
    if (s.dl.overflow()) s.error = true;
}

const DrawList& Context::draw_list() const {
    return s_->dl;
}
bool Context::error() const {
    return s_->error;
}
bool Context::warning() const {
    return s_->warning;
}
void Context::set_theme(const Theme& t) {
    s_->cfg.theme = t;
}
const Theme& Context::theme() const {
    return s_->cfg.theme;
}
float Context::dpi_scale() const {
    return s_->cfg.dpi_scale;
}
void Context::set_default_font(FontHandle f) {
    s_->default_font = f;
}
void Context::set_clipboard_callbacks(ClipboardGet g, ClipboardSet st, void* user) {
    s_->clip_get = g;
    s_->clip_set = st;
    s_->clip_user = user;
}
const uint8_t* Context::atlas_pixels() const {
    return s_->atlas.get();
}
int Context::atlas_size() const {
    return s_->cfg.atlas_size;
}
uint32_t Context::atlas_version() const {
    return s_->atlas_ver;
}
bool Context::wants_text_input() const {
    return s_->wants_text;
}
Rect Context::text_input_rect() const {
    return s_->text_rect;
}
Cursor Context::cursor() const {
    return s_->cursor;
}
bool Context::animating() const {
    return s_->animating;
}
bool Context::can_dismiss() const {
    return s_->popup_id != 0 || s_->modal_id != 0 || s_->wants_text;
}
void Context::set_animations(bool enabled) {
    s_->cfg.animations = enabled;
}

void Context::set_dpi_scale(float scale) {
    if (scale <= 0 || scale == s_->cfg.dpi_scale) return;
    s_->cfg.dpi_scale = scale;
    detail::reset_atlas(*s_);
}

FontHandle Context::load_font(const uint8_t* data, size_t size, float pixel_size) {
    using namespace detail;
    State& s = *s_;
    if (!data || size < 12 || pixel_size <= 0 || s.font_n >= s.cfg.max_fonts) {
        s.error = true;
        return -1;
    }
    FontSlot& f = s.fonts[s.font_n];
    f = FontSlot{};
    f.data = data;
    f.info.userdata = &s.arena;
    int off = stbtt_GetFontOffsetForIndex(data, 0);
    if (off < 0 || !stbtt_InitFont(&f.info, data, off)) {
        s.error = true;
        return -1;
    }
    f.pixel_size = pixel_size;
    f.scale = stbtt_ScaleForPixelHeight(&f.info, pixel_size);
    int a, d, g;
    stbtt_GetFontVMetrics(&f.info, &a, &d, &g);
    f.ascent = a * f.scale;
    f.descent = d * f.scale;
    f.line_gap = g * f.scale;
    if (s.default_font < 0) s.default_font = s.font_n;
    return s.font_n++;
}

}  // namespace rd
