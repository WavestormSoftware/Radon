// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
using namespace detail;

Response slider(Context& ctx, std::string_view label, float& value, float lo, float hi, const char* format) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, true);
    // Track and round thumb on the left, value in a fixed column on the right (sized for lo and hi so
    // the track does not jump while dragging).
    char buf[64];
    const char* fmt = format ? format : "%.2f";
    std::snprintf(buf, sizeof buf, fmt, (double)lo);
    float vw = text_size(s, t.font_body, buf).x;
    std::snprintf(buf, sizeof buf, fmt, (double)hi);
    vw = std::fmax(vw, text_size(s, t.font_body, buf).x) + t.padding;
    if (vw > r.w * 0.4f) vw = 0;                   // too narrow for a value column: track only
    const float kr = std::fmin(r.h * 0.3f, 9.0f);  // thumb radius
    const float x0 = r.x + kr, w = std::fmax(1.0f, r.w - vw - 2 * kr);
    if (hi > lo) {
        float nv = value;
        if (s.active == id && s.in.pointer_down) {
            float f = (s.in.pointer.x - x0) / w;
            f = f < 0 ? 0 : (f > 1 ? 1 : f);
            nv = lo + f * (hi - lo);
        }
        if (rs.focused) {
            const float step = (hi - lo) * 0.05f;
            if (key_pressed(s, KeyLeft)) nv -= step;
            if (key_pressed(s, KeyRight)) nv += step;
            if (key_pressed(s, KeyPageDown)) nv -= step * 5;
            if (key_pressed(s, KeyPageUp)) nv += step * 5;
            if (key_pressed(s, KeyHome)) nv = lo;
            if (key_pressed(s, KeyEnd)) nv = hi;
        }
        nv = nv < lo ? lo : (nv > hi ? hi : nv);
        if (nv != value) {
            value = nv;
            rs.changed = true;
        }
    }
    std::snprintf(buf, sizeof buf, fmt, (double)value);
    a11y_node(s, A11yRole::Slider, label, buf);
    float f = hi > lo ? (value - lo) / (hi - lo) : 0;
    f = f < 0 ? 0 : (f > 1 ? 1 : f);
    const float cy = r.y + r.h / 2, th = std::fmax(4.0f, kr * 0.5f);
    s.dl.rect({r.x, cy - th / 2, w + 2 * kr, th}, t.surface_active, th / 2);
    s.dl.rect({r.x, cy - th / 2, kr + w * f, th}, t.accent, th / 2);
    const float grow = animate(s, hash_u32(id, 91), rs.hovered, s.active == id) * 2;  // thumb swells on hover
    const float kk = kr + grow;
    const Rect k{x0 + w * f - kk, cy - kk, 2 * kk, 2 * kk};
    if (t.shadows) s.dl.shadow(k, t.shadow, kk, 3, {0, 1});
    s.dl.rect(k, {255, 255, 255, 255}, kk);
    s.dl.rect(k, t.accent, kk, 2);
    focus_ring(s, r, rs, t.corner_radius);
    if (vw > 0) draw_text_fit(s, {r.x + r.w - vw, r.y, vw, r.h}, buf, t.text, false);
    return rs;
}

Response drag_float(Context& ctx, std::string_view label, float& value, float speed) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, true);
    if (s.active == id && s.in.pointer_down && s.pointer_delta.x != 0) {
        value += s.pointer_delta.x * speed;
        rs.changed = true;
    }
    if (rs.focused) {
        if (key_pressed(s, KeyLeft)) value -= speed, rs.changed = true;
        if (key_pressed(s, KeyRight)) value += speed, rs.changed = true;
    }
    draw_frame(s, r, control_color(s, id, rs), rs.focused);
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.2f", (double)value);
    draw_text_fit(s, r, buf, t.text, true);
    return rs;
}

}  // namespace rd
