// Copyright (c) 2026 Wavestorm Software
// Toasts, drawers, context menus, disabled sections and transitions.
#include "../internal.hpp"

namespace rd {
using namespace detail;

void toast(Context& ctx, std::string_view text, Tone tone, float seconds) {
    State& s = state(ctx);
    Toast* slot = &s.toasts[0];
    for (Toast& t : s.toasts) {
        if (t.duration == 0) {
            slot = &t;
            break;
        }
        if (t.age > slot->age) slot = &t;  // all busy: replace the oldest
    }
    const size_t n = utf8_fit(text, sizeof(slot->text) - 1);
    if (n) std::memcpy(slot->text, text.data(), n);
    slot->text[n] = 0;
    slot->tone = tone;
    slot->age = 0;
    slot->duration = seconds < 0.5f ? 0.5f : seconds;
}

void set_toast_offset(Context& ctx, float bottom_px) {
    state(ctx).toast_offset = bottom_px;
}

namespace detail {

void draw_toasts(State& s) {
    const Theme& t = s.cfg.theme;
    const Vec2 sc = s.in.screen_size;
    float y = sc.y - s.in.safe_area.h - s.toast_offset - 16;
    for (Toast& tt : s.toasts) {
        if (tt.duration == 0) continue;
        tt.age += s.dt;
        if (tt.age >= tt.duration) {
            tt.duration = 0;
            continue;
        }
        s.animating = true;
        if (sc.x <= 0) continue;  // no screen size: nowhere to put it
        float a = clamp01(std::fmin(tt.age / 0.2f, (tt.duration - tt.age) / 0.3f));
        if (!s.cfg.animations) a = 1;
        const float e = 1 - (1 - a) * (1 - a) * (1 - a);
        const Vec2 sz = text_size(s, t.font_body, tt.text);
        const float dot = 8, h = std::fmax(t.cell_height, sz.y + t.padding);
        const float w = std::fmin(sz.x + 3 * t.padding + dot, sc.x - 24);
        const Rect r{(sc.x - w) / 2, y - h + (1 - e) * 12, w, h};
        s.dl.set_layer(kTooltipLayer);
        s.dl.break_command();
        s.dl.set_clip({-1e6f, -1e6f, 2e6f, 2e6f});
        s.dl.set_alpha(e);
        if (t.shadows) s.dl.shadow(r, t.shadow, h / 2, 10, {0, 4});
        s.dl.rect(r, t.surface_active, h / 2);
        s.dl.rect(r, t.border, h / 2, t.border_width);
        s.dl.rect({r.x + t.padding, r.y + (h - dot) / 2, dot, dot}, tone_color(t, tt.tone), dot / 2);
        const Rect tr{r.x + 2 * t.padding + dot, r.y, w - 3 * t.padding - dot, h};
        s.dl.set_clip(tr);
        draw_text_centered(s, t.font_body, tr, tt.text, t.text, false);
        y -= (h + 8) * e;
    }
    s.dl.set_alpha(1);
}

}  // namespace detail

bool drawer_begin(Context& ctx, std::string_view id, Side side, float size) {
    State& s = state(ctx);
    const ID mid = hash_id(top_id(s), id);
    if (s.modal_id != mid) return false;
    s.modal_seen = true;
    const float k = anim_to(s, hash_u32(mid, 91), 1, 0.22f, 0);
    const Vec2 sc = s.in.screen_size;
    const Rect sa = s.in.safe_area;
    const float rad =
        s.cfg.theme.window_radius;  // the rounded edge on the screen border is pushed off screen
    const float extent = side == Side::Bottom ? std::fmin(size + sa.h, sc.y - sa.y)
                                              : std::fmin(size + (side == Side::Left ? sa.x : sa.w), sc.x);

    // Swipe toward the edge to dismiss: the panel follows the finger, a long or fast swipe closes it.
    float scroll = 0;
    for (int i = 0; i < s.cfg.max_windows; i++)
        if (s.wins[i].used && s.wins[i].id == mid) scroll = s.wins[i].scroll;
    const float dx = s.in.pointer.x - s.press_pos.x, dy = s.in.pointer.y - s.press_pos.y;
    const float toward = side == Side::Bottom ? dy : (side == Side::Left ? -dx : dx);
    const bool grab = s.active == hash_u32(mid, 93);  // the bottom sheet handle (mouse and touch)
    if (s.in.pointer_down && s.sheet_lock == 0 && s.press_window == mid && s.in.touch && s.touch_moved &&
        !s.active_captures) {
        const bool along =
            side == Side::Bottom ? std::fabs(dy) > std::fabs(dx) : std::fabs(dx) > std::fabs(dy);
        s.sheet_lock = along && toward > 0 && (side != Side::Bottom || scroll <= 0.5f) ? 1 : 2;
    }
    if (grab) s.sheet_lock = 1;
    const bool dragging = s.sheet_lock == 1 && s.in.pointer_down;
    if (dragging) {
        s.sheet_drag = std::fmax(0.0f, toward);
        s.drag_used = true;
    } else if (s.sheet_lock == 1 && s.in.pointer_released) {
        const Vec2 v = s.gesture.velocity;
        const float vt = side == Side::Bottom ? v.y : (side == Side::Left ? -v.x : v.x);
        s.sheet_lock = 2;
        if (s.sheet_drag > extent / 3 || vt > 500) {
            s.modal_id = 0;
            s.sheet_drag = 0;
            haptic(s, Haptic::Light);
            return false;
        }
    } else if (s.sheet_drag > 0) {  // spring back
        s.sheet_drag *= s.cfg.animations ? std::exp(-s.dt * 14) : 0.0f;
        if (s.sheet_drag < 0.5f) s.sheet_drag = 0;
        s.animating = true;
    }
    const float off = s.sheet_drag;
    const float shown = k * clamp01(1 - off / std::fmax(1.0f, extent));
    Rect r;
    if (side == Side::Bottom) {
        r = {0, sc.y - extent * k + off, sc.x, extent + rad};
    } else {
        r = side == Side::Left ? Rect{-extent * (1 - k) - rad - off, 0, extent + rad, sc.y}
                               : Rect{sc.x - extent * k + off, 0, extent + rad, sc.y};
    }
    if (s.in.pointer_pressed && !r.contains(s.in.pointer) && s.popup_id == 0 &&
        s.frame > s.modal_open_frame) {
        s.modal_id = 0;  // a tap on the dimmed area dismisses
        return false;
    }
    const uint32_t layer = s.cur_layer;
    s.dl.set_layer(kOverlayLayer);
    s.dl.break_command();
    s.dl.set_clip({-1e6f, -1e6f, 2e6f, 2e6f});
    s.dl.rect({0, 0, sc.x, sc.y}, {0, 0, 0, (uint8_t)(150 * shown)});
    s.dl.set_layer(layer);
    s.dl.break_command();
    if (!begin_window_impl(ctx, mid, "", r, WindowScrollable, true)) return false;
    Layout& l = top_layout(s);  // keep content clear of the off-screen edge, notches and system bars
    if (side == Side::Bottom) {
        const Theme& t = s.cfg.theme;
        const Rect hr{r.x, r.y, r.w, 22};  // grab handle
        const ID hid = hash_u32(mid, 93);
        update_control(ctx, hid, hr, false, true);
        if (s.hover == hid || s.active == hid) s.cursor = Cursor::ResizeNS;
        s.dl.rect({r.x + r.w / 2 - 18, r.y + 8, 36, 4},
                  lerp(t.border, t.text_muted, animate(s, hid, s.hover == hid || grab, false)), 2);
        l.body.y += 12;
        l.body.h -= rad + sa.h + 12;
    } else {
        const float lead = side == Side::Left ? rad + sa.x : 0;
        l.body.x += lead;
        l.body.w -= rad + (side == Side::Left ? sa.x : sa.w);
        l.body.y += sa.y;
        l.body.h -= sa.y + sa.h;
    }
    return true;
}

void drawer_end(Context& ctx) {
    end_window(ctx);
}

bool secondary_clicked(Context& ctx) {
    const State& s = state(ctx);
    const Vec2 p = s.in.pointer;
    return s.secondary && s.disabled == 0 && s.hover_window == s.cur_window && s.last_rect.contains(p) &&
           (s.clip_n == 0 || s.clips[s.clip_n - 1].contains(p));
}

bool context_menu_begin(Context& ctx, std::string_view id) {
    State& s = state(ctx);
    const ID pid = hash_id(top_id(s), id);
    if (secondary_clicked(ctx)) {
        open_popup_id(s, pid);
        s.popup_anchor = {s.in.pointer.x, s.in.pointer.y, 0, 0};
    }
    return popup_begin_id(ctx, pid, 180);
}

void begin_disabled(Context& ctx, bool disabled) {
    State& s = state(ctx);
    if (!push_alpha(s, disabled ? 0.45f : 1.0f)) return;
    s.disabled_stack[s.alpha_n - 1] = disabled;
    if (disabled) s.disabled++;
}

void end_disabled(Context& ctx) {
    State& s = state(ctx);
    if (s.alpha_n == 0) {
        s.error = true;
        return;
    }
    if (s.disabled_stack[s.alpha_n - 1]) s.disabled--;
    pop_alpha(s);
}

float animate(Context& ctx, std::string_view id, float target, float duration, float from) {
    State& s = state(ctx);
    return anim_to(s, hash_id(top_id(s), id), target, duration, from);
}

void transition_begin(Context& ctx, std::string_view id, int key) {
    State& s = state(ctx);
    // One animation slot per key: a new key has no slot yet, so it starts at 0 and plays again.
    const float v = anim_to(s, hash_u32(hash_id(top_id(s), id), (uint32_t)key), 1, 0.2f, 0);
    if (s.tr_n >= kMaxAlpha || !push_alpha(s, v)) {
        s.error = true;
        return;
    }
    s.disabled_stack[s.alpha_n - 1] = false;
    const float px = (1 - v) * 14;
    s.tr_shift[s.tr_n++] = px;
    if (s.layout_n > 0) top_layout(s).body.x += px;
}

void transition_end(Context& ctx) {
    State& s = state(ctx);
    if (s.tr_n == 0) {
        s.error = true;
        return;
    }
    if (s.layout_n > 0) top_layout(s).body.x -= s.tr_shift[s.tr_n - 1];
    s.tr_n--;
    pop_alpha(s);
}

}  // namespace rd
