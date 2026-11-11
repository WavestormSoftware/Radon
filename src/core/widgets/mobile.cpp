// Copyright (c) 2026 Wavestorm Software
// Touch patterns: pull to refresh, edge swipes and swipeable list rows.
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace {

void spinner_dots(State& s, Vec2 c, float rad, Color col, float fraction) {
    const int n = 10;
    const float head = std::fmod(s.time * 1.2f, 1.0f);
    for (int i = 0; i < n; i++) {
        const float f = (float)i / n, a = f * 6.283185f - 1.570796f;
        const float fade = fraction < 1 ? (f <= fraction ? 0.0f : 1.0f) : std::fmod(head - f + 1.0f, 1.0f);
        const float sz = rad * 0.32f;
        s.dl.rect({c.x + std::cos(a) * rad - sz / 2, c.y + std::sin(a) * rad - sz / 2, sz, sz},
                  with_alpha(col, 1.0f - fade * 0.85f), sz / 2);
    }
}

}  // namespace

bool pull_to_refresh(Context& ctx, std::string_view id, bool& refreshing) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect view;
    float* sc = scroll_target(s, &view);
    if (!sc || s.layout_n == 0) return false;
    bool fresh = false;
    Anim* a = anim_slot(s, hash_id(top_id(s), id), fresh);
    if (!a) return false;
    if (fresh) a->hover = 0, a->key = 0;
    a->last_frame = s.frame;
    const float threshold = 64, rest = 48;
    float& pull = a->hover;
    bool started = false;
    const bool drag = s.in.touch && s.in.pointer_down && s.touch_moved && s.in.touch_count < 2 &&
                      view.contains(s.press_pos) && !s.active_captures && !refreshing && *sc <= 0.5f;
    if (drag && (s.pointer_delta.y > 0 || pull > 0)) {
        const float before = pull;
        pull = std::fmin(std::fmax(0.0f, pull + s.pointer_delta.y * 0.5f), threshold * 2);
        if (before < threshold && pull >= threshold) haptic(s, Haptic::Light);
        s.drag_used = true;  // the container must not scroll while the list is pulled
        a->key = 1;
    } else if (!s.in.pointer_down) {
        if (a->key == 1 && pull >= threshold && !refreshing) {
            refreshing = started = true;
            haptic(s, Haptic::Medium);
        }
        a->key = 0;
        const float target = refreshing ? rest : 0;
        pull += (target - pull) * (s.cfg.animations ? 1 - std::exp(-s.dt * 12) : 1.0f);
        if (std::fabs(pull - target) < 0.5f) pull = target;
    }
    if (pull > 0.5f || refreshing) s.animating = true;
    if (pull <= 0.5f) return started;
    top_layout(s).body.y += pull;  // the content slides down under the indicator
    const float d = 30, k = clamp01(pull / threshold);
    const Vec2 c{view.x + view.w / 2, view.y + pull / 2 + 2};
    const Rect b{c.x - d / 2, c.y - d / 2, d, d};
    if (t.shadows) s.dl.shadow(b, t.shadow, d / 2, 8, {0, 2});
    s.dl.rect(b, t.surface_active, d / 2);
    s.dl.set_alpha(s.alpha * (refreshing ? 1.0f : 0.4f + 0.6f * k));
    spinner_dots(s, c, d * 0.3f, t.accent, refreshing ? 1.0f : k * 0.99f);
    s.dl.set_alpha(s.alpha);
    return started;
}

bool edge_swipe(Context& ctx, Side side) {
    State& s = state(ctx);
    if (!s.in.touch || !s.in.pointer_down || s.edge_fired || side == Side::Bottom) return false;
    const float zone = 24;
    const bool left = side == Side::Left;
    const bool at_edge = left ? s.press_pos.x <= s.in.safe_area.x + zone
                              : s.press_pos.x >= s.in.screen_size.x - s.in.safe_area.w - zone;
    const float dx = s.in.pointer.x - s.press_pos.x, dy = s.in.pointer.y - s.press_pos.y;
    if (!at_edge || (left ? dx : -dx) < 40 || std::fabs(dx) < 1.5f * std::fabs(dy)) return false;
    s.edge_fired = true;
    s.drag_used = true;
    haptic(s, Haptic::Light);
    return true;
}

Response swipe_item(Context& ctx, std::string_view label, const SwipeAction* actions, int count,
                    int& triggered, bool selected) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    triggered = -1;
    const ID id = get_id(s, label);
    const Rect r = layout_next(ctx);
    if (!actions || count < 0) count = 0;
    bool fresh = false;
    Anim* a = anim_slot(s, hash_u32(id, 0x5717u), fresh);
    if (a && fresh) a->hover = a->press = 0, a->key = 0;
    if (a) a->last_frame = s.frame;
    float off = a ? a->hover : 0;  // how far the row has slid left
    const float aw = r.h * 1.6f, total = aw * (float)count;
    enum { Open = 1, Horizontal = 2, Vertical = 4 };
    int key = a ? a->key : 0;

    // The revealed actions take presses before the row does.
    for (int i = 0; i < count && off > 1; i++) {
        const float w = off / (float)count;
        const Rect ar{r.x + r.w - off + w * (float)i, r.y, w, r.h};
        if (update_control(ctx, hash_u32(id, 0xAC00u + (uint32_t)i), ar, false, false).clicked) {
            triggered = i;
            key &= ~Open;
            haptic(s, Haptic::Light);
        }
    }
    Response rs = update_control(ctx, id, r, true, false);
    a11y_node(s, A11yRole::ListItem, label, {}, selected ? A11yNode::Selected : 0u);
    if (rs.pressed) {
        key &= Open;
        if (a) a->press = off;
    }
    const float dx = s.in.pointer.x - s.press_pos.x, dy = s.in.pointer.y - s.press_pos.y;
    if (s.active == id && s.in.pointer_down && count > 0) {
        if (!(key & (Horizontal | Vertical)) && dx * dx + dy * dy > kTouchSlop * kTouchSlop)
            key |= std::fabs(dx) > std::fabs(dy) ? Horizontal : Vertical;
        if (key & Horizontal) {
            off = std::fmin(std::fmax(0.0f, (a ? a->press : 0) - dx), total + 24);
            s.drag_used = true;
            s.touch_moved = true;  // no click on release
        }
    } else if (key & Horizontal) {  // released after a horizontal drag: snap open or shut
        const float v = s.gesture.velocity.x;
        const bool open = v < -300 || (v <= 300 && off > total / 2);
        key = open ? Open : 0;
        if (open) haptic(s, Haptic::Selection);
    }
    if (rs.clicked && (key & Open)) rs.clicked = false, key &= ~Open;  // a tap closes an open row first
    if (s.in.pointer_pressed && !r.contains(s.in.pointer)) key &= ~Open;
    if (!(s.active == id && (key & Horizontal))) {
        const float target = (key & Open) ? total : 0;
        off += (target - off) * (s.cfg.animations ? 1 - std::exp(-s.dt * 16) : 1.0f);
        if (std::fabs(off - target) < 0.5f) off = target;
        if (off != target) s.animating = true;
    }
    if (a) a->hover = off, a->key = key;

    push_clip(s, r);
    for (int i = 0; i < count && off > 0.5f; i++) {
        const float w = off / (float)count;
        const Rect ar{r.x + r.w - off + w * (float)i, r.y, w + 1, r.h};
        s.dl.rect(ar, actions[i].color);
        const float is = std::fmin(r.h * 0.42f, 22.0f);
        const bool lbl = actions[i].label && w > aw * 0.8f && r.h >= 40;
        const Rect ir{ar.x + (ar.w - is) / 2, ar.y + (lbl ? r.h * 0.18f : (r.h - is) / 2), is, is};
        draw_icon(ctx, actions[i].icon, ir, {255, 255, 255, 255});
        if (lbl)
            draw_text_centered(s, small_font(t), {ar.x, ir.y + is, ar.w, r.y + r.h - ir.y - is},
                               actions[i].label, {255, 255, 255, 255}, true);
    }
    const Rect row{r.x - off, r.y, r.w, r.h};
    const float h = animate(s, id, rs.hovered, false);
    s.dl.rect(row, lerp(t.surface, t.surface_hover, h), off > 0.5f ? 0 : t.corner_radius);
    if (selected) s.dl.rect(row, t.selection, t.corner_radius);
    focus_ring(s, row, rs, t.corner_radius);
    draw_text_fit(s, {row.x + t.padding, row.y, row.w - 2 * t.padding, row.h}, visible_label(label), t.text,
                  false);
    pop_clip(s);
    return rs;
}

}  // namespace rd
