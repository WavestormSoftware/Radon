// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace {

uint32_t pack(Color c) {
    return (uint32_t)c.r << 24 | (uint32_t)c.g << 16 | (uint32_t)c.b << 8;
}  // alpha excluded

void rgb_to_hsv(Color c, float& h, float& s, float& v) {
    float r = c.r / 255.0f, g = c.g / 255.0f, b = c.b / 255.0f;
    float mx = std::fmax(r, std::fmax(g, b)), mn = std::fmin(r, std::fmin(g, b)), d = mx - mn;
    v = mx;
    s = mx > 0 ? d / mx : 0;
    if (d <= 0) return;  // grey: keep the previous hue
    if (mx == r)
        h = std::fmod((g - b) / d, 6.0f);
    else if (mx == g)
        h = (b - r) / d + 2;
    else
        h = (r - g) / d + 4;
    h /= 6;
    if (h < 0) h += 1;
}

Color hsv_to_rgb(float h, float s, float v, uint8_t a) {
    h = (h - std::floor(h)) * 6;
    float c = v * s, x = c * (1 - std::fabs(std::fmod(h, 2.0f) - 1)), m = v - c, r = 0, g = 0, b = 0;
    if (h < 1)
        r = c, g = x;
    else if (h < 2)
        r = x, g = c;
    else if (h < 3)
        g = c, b = x;
    else if (h < 4)
        g = x, b = c;
    else if (h < 5)
        r = x, b = c;
    else
        r = c, b = x;
    auto q = [m](float f) { return (uint8_t)(clamp01(f + m) * 255.0f + 0.5f); };
    return {q(r), q(g), q(b), a};
}

PickerState* picker_for(State& s, ID id) {
    PickerState* oldest = &s.pickers[0];
    for (auto& p : s.pickers) {
        if (p.id == id) return &p;
        if (p.last_frame < oldest->last_frame) oldest = &p;
    }
    *oldest = PickerState{id, 0, 0, 0, 0xffffffffu, s.frame};
    return oldest;
}

}  // namespace

Response color_button(Context& ctx, std::string_view label, Color color) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    if (color.a < 255) {  // checker behind translucent colors
        s.dl.rect(r, {200, 200, 200, 255}, t.corner_radius);
        s.dl.rect({r.x, r.y, r.w / 2, r.h / 2}, {120, 120, 120, 255}, 0);
        s.dl.rect({r.x + r.w / 2, r.y + r.h / 2, r.w / 2, r.h / 2}, {120, 120, 120, 255}, 0);
    }
    s.dl.rect(r, color, t.corner_radius);
    bool ring = rs.focused && s.kbd_nav;
    s.dl.rect(r, ring ? t.focus_ring : (rs.hovered ? t.text_muted : t.border), t.corner_radius,
              ring ? t.focus_ring_width : t.border_width);
    return rs;
}

bool color_picker(Context& ctx, std::string_view label, Color& color, bool alpha) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    PickerState* ps = picker_for(s, id);
    ps->last_frame = s.frame;
    if (ps->last_rgb != pack(color)) rgb_to_hsv(color, ps->h, ps->s, ps->v);  // changed from outside

    layout_row(ctx, 1, nullptr, 0, 140);
    Rect r = layout_next(ctx);
    const float bar = 18, gap = 8;
    const int bars = alpha ? 2 : 1;
    Rect sv{r.x, r.y, r.w - bars * (bar + gap), r.h};
    Rect hue{sv.x + sv.w + gap, r.y, bar, r.h};
    Rect alp{hue.x + bar + gap, r.y, bar, r.h};

    bool changed = false;
    const Rect saved_rect = s.last_rect;
    ID svid = hash_u32(id, 1), hid = hash_u32(id, 2), aid = hash_u32(id, 3);
    update_control(ctx, svid, sv, false, true);
    update_control(ctx, hid, hue, false, true);
    if (alpha) update_control(ctx, aid, alp, false, true);
    s.last_rect = saved_rect;
    const Vec2 p = s.in.pointer;
    uint8_t a = color.a;
    if (s.active == svid && s.in.pointer_down) {
        ps->s = clamp01((p.x - sv.x) / sv.w);
        ps->v = 1 - clamp01((p.y - sv.y) / sv.h);
    } else if (s.active == hid && s.in.pointer_down) {
        ps->h = clamp01((p.y - hue.y) / hue.h);
    } else if (alpha && s.active == aid && s.in.pointer_down) {
        a = (uint8_t)(255.0f * (1 - clamp01((p.y - alp.y) / alp.h)) + 0.5f);
    }
    Color nc = hsv_to_rgb(ps->h, ps->s, ps->v, a);
    // Dragging only: keep the exact color the user passed in until they touch a control.
    if (s.active == svid || s.active == hid || (alpha && s.active == aid)) {
        changed = nc.r != color.r || nc.g != color.g || nc.b != color.b || nc.a != color.a;
        color = nc;
    }
    ps->last_rgb = pack(color);

    // saturation / value square
    const Color hue_c = hsv_to_rgb(ps->h, 1, 1, 255);
    const Color white{255, 255, 255, 255}, black0{0, 0, 0, 0}, black{0, 0, 0, 255};
    s.dl.gradient_rect(sv, white, hue_c, hue_c, white);
    s.dl.gradient_rect(sv, black0, black0, black, black);
    s.dl.rect(sv, t.border, 0, t.border_width);
    const Vec2 mk{sv.x + sv.w * ps->s, sv.y + sv.h * (1 - ps->v)};
    s.dl.rect({mk.x - 6, mk.y - 6, 12, 12}, black, 6, 3);
    s.dl.rect({mk.x - 5, mk.y - 5, 10, 10}, white, 5, 1.5f);
    // hue bar
    for (int i = 0; i < 6; i++) {
        float y0 = hue.y + hue.h * (float)i / 6, y1 = hue.y + hue.h * (float)(i + 1) / 6;
        s.dl.gradient_rect({hue.x, y0, hue.w, y1 - y0}, hsv_to_rgb((float)i / 6, 1, 1, 255),
                           hsv_to_rgb((float)i / 6, 1, 1, 255), hsv_to_rgb((float)(i + 1) / 6, 1, 1, 255),
                           hsv_to_rgb((float)(i + 1) / 6, 1, 1, 255));
    }
    s.dl.rect(hue, t.border, 0, t.border_width);
    float hy = hue.y + hue.h * ps->h;
    s.dl.rect({hue.x - 2, hy - 3, hue.w + 4, 6}, white, 3, 1.5f);
    if (alpha) {
        s.dl.rect(alp, {200, 200, 200, 255}, 0);
        for (int i = 0; i < 7; i++)
            s.dl.rect({alp.x + ((i & 1) ? bar / 2 : 0), alp.y + alp.h * (float)i / 7, bar / 2, alp.h / 7},
                      {120, 120, 120, 255}, 0);
        Color solid = color;
        solid.a = 255;
        Color clear = solid;
        clear.a = 0;
        s.dl.gradient_rect(alp, solid, solid, clear, clear);
        s.dl.rect(alp, t.border, 0, t.border_width);
        float ay = alp.y + alp.h * (1 - color.a / 255.0f);
        s.dl.rect({alp.x - 2, ay - 3, alp.w + 4, 6}, white, 3, 1.5f);
    }

    // preview + hex
    const float cols[] = {40, -1};
    layout_row(ctx, 2, cols, 2, 0);
    color_button(ctx, "##preview", color);
    char hex[16];
    if (alpha)
        std::snprintf(hex, sizeof hex, "#%02X%02X%02X%02X", color.r, color.g, color.b, color.a);
    else
        std::snprintf(hex, sizeof hex, "#%02X%02X%02X", color.r, color.g, color.b);
    rd::label(ctx, hex);
    return changed;
}

}  // namespace rd
