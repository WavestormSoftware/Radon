// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
using namespace detail;

Response slider(Context& ctx, std::string_view label, float& value, float lo, float hi,
                const char* format) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, true);
    if (hi > lo) {
        float nv = value;
        if (s.active == id && s.in.pointer_down) {
            float f = (s.in.pointer.x - r.x) / (r.w > 0 ? r.w : 1);
            f = f < 0 ? 0 : (f > 1 ? 1 : f);
            nv = lo + f * (hi - lo);
        }
        if (rs.focused) {
            const float step = (hi - lo) * 0.05f;
            if (key_pressed(s, KeyLeft)) nv -= step;
            if (key_pressed(s, KeyRight)) nv += step;
        }
        nv = nv < lo ? lo : (nv > hi ? hi : nv);
        if (nv != value) {
            value = nv;
            rs.changed = true;
        }
    }
    float f = hi > lo ? (value - lo) / (hi - lo) : 0;
    f = f < 0 ? 0 : (f > 1 ? 1 : f);
    s.dl.rect(r, t.background, t.corner_radius);
    s.dl.rect({r.x, r.y, r.w * f, r.h}, with_alpha(t.accent, rs.hovered || s.active == id ? 1.0f : 0.85f),
              t.corner_radius);
    draw_frame(s, r, Color{0, 0, 0, 0}, rs.focused);
    char buf[64];
    std::snprintf(buf, sizeof buf, format ? format : "%.2f", (double)value);
    draw_text_fit(s, r, buf, f > 0.5f ? t.accent_text : t.text, true);
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
