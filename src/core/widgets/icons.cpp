// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
using namespace detail;

void draw_icon(Context& ctx, Icon icon, const Rect& r, Color c) {
    DrawList& d = state(ctx).dl;
    const float side = std::fmin(r.w, r.h);
    const float u = side * 0.3f;  // half extent of the glyph
    const Vec2 m{r.x + r.w / 2, r.y + r.h / 2};
    const float w = std::fmax(1.25f, side * 0.075f);  // stroke width
    auto L = [&](float x0, float y0, float x1, float y1) {
        d.line({m.x + x0 * u, m.y + y0 * u}, {m.x + x1 * u, m.y + y1 * u}, c, w);
    };
    auto ring = [&](float cx, float cy, float rad) {
        d.rect({m.x + (cx - rad) * u, m.y + (cy - rad) * u, 2 * rad * u, 2 * rad * u}, c, rad * u, w);
    };
    auto dot = [&](float cx, float cy, float rad) {
        d.rect({m.x + (cx - rad) * u, m.y + (cy - rad) * u, 2 * rad * u, 2 * rad * u}, c, rad * u);
    };
    switch (icon) {
        case Icon::Plus:
            L(-1, 0, 1, 0);
            L(0, -1, 0, 1);
            break;
        case Icon::Minus:
            L(-1, 0, 1, 0);
            break;
        case Icon::Close:
            L(-0.8f, -0.8f, 0.8f, 0.8f);
            L(0.8f, -0.8f, -0.8f, 0.8f);
            break;
        case Icon::Check:
            L(-1, 0.1f, -0.3f, 0.8f);
            L(-0.3f, 0.8f, 1, -0.7f);
            break;
        case Icon::ChevronDown:
            L(-0.9f, -0.4f, 0, 0.5f);
            L(0, 0.5f, 0.9f, -0.4f);
            break;
        case Icon::ChevronUp:
            L(-0.9f, 0.4f, 0, -0.5f);
            L(0, -0.5f, 0.9f, 0.4f);
            break;
        case Icon::ChevronLeft:
            L(0.4f, -0.9f, -0.5f, 0);
            L(-0.5f, 0, 0.4f, 0.9f);
            break;
        case Icon::ChevronRight:
            L(-0.4f, -0.9f, 0.5f, 0);
            L(0.5f, 0, -0.4f, 0.9f);
            break;
        case Icon::Search:
            ring(-0.2f, -0.2f, 0.7f);
            L(0.3f, 0.3f, 1, 1);
            break;
        case Icon::Eye:
            d.rect({m.x - 1.1f * u, m.y - 0.62f * u, 2.2f * u, 1.24f * u}, c, 0.62f * u, w);
            dot(0, 0, 0.3f);
            break;
        case Icon::EyeOff:
            d.rect({m.x - 1.1f * u, m.y - 0.62f * u, 2.2f * u, 1.24f * u}, c, 0.62f * u, w);
            L(-1, 1, 1, -1);
            break;
        case Icon::Lock:
            d.rect({m.x - 0.7f * u, m.y - 0.1f * u, 1.4f * u, 1.0f * u}, c, 2);
            d.rect({m.x - 0.45f * u, m.y - 1.0f * u, 0.9f * u, 1.4f * u}, c, 0.45f * u, w);
            break;
        case Icon::Unlock:
            d.rect({m.x - 0.7f * u, m.y - 0.1f * u, 1.4f * u, 1.0f * u}, c, 2);
            d.rect({m.x - 0.05f * u, m.y - 1.0f * u, 0.9f * u, 1.4f * u}, c, 0.45f * u, w);
            break;
        case Icon::Trash:
            L(-0.9f, -0.7f, 0.9f, -0.7f);
            L(-0.3f, -0.7f, -0.3f, -1.0f);
            L(0.3f, -0.7f, 0.3f, -1.0f);
            L(-0.3f, -1.0f, 0.3f, -1.0f);
            d.rect({m.x - 0.6f * u, m.y - 0.55f * u, 1.2f * u, 1.6f * u}, c, 2, w);
            break;
        case Icon::Folder:
            d.rect({m.x - 1.0f * u, m.y - 0.6f * u, 2.0f * u, 1.5f * u}, c, 2, w);
            L(-1.0f, -0.6f, -0.5f, -1.0f);
            L(-0.5f, -1.0f, 0, -1.0f);
            L(0, -1.0f, 0.2f, -0.6f);
            break;
        case Icon::Menu:
            L(-1, -0.7f, 1, -0.7f);
            L(-1, 0, 1, 0);
            L(-1, 0.7f, 1, 0.7f);
            break;
        case Icon::Dots:
            dot(0, -0.8f, 0.2f);
            dot(0, 0, 0.2f);
            dot(0, 0.8f, 0.2f);
            break;
        case Icon::Settings:
            ring(0, 0, 0.55f);
            for (int i = 0; i < 8; i++) {
                float a = (float)i * 0.785398f, cx = std::cos(a), cy = std::sin(a);
                L(cx * 0.85f, cy * 0.85f, cx * 1.1f, cy * 1.1f);
            }
            break;
        case Icon::Star: {
            Vec2 p[10];
            for (int i = 0; i < 10; i++) {
                float a = -1.570796f + (float)i * 0.628318f, rad = (i % 2) ? 0.45f : 1.1f;
                p[i] = {std::cos(a) * rad, std::sin(a) * rad};
            }
            for (int i = 0; i < 10; i++) L(p[i].x, p[i].y, p[(i + 1) % 10].x, p[(i + 1) % 10].y);
            break;
        }
        case Icon::Pencil:
            L(-0.9f, 0.9f, -0.6f, 0.3f);
            L(-0.6f, 0.3f, 0.7f, -1.0f);
            L(0.7f, -1.0f, 1.0f, -0.7f);
            L(1.0f, -0.7f, -0.3f, 0.6f);
            L(-0.3f, 0.6f, -0.9f, 0.9f);
            break;
        case Icon::Undo:
            L(-0.9f, -0.4f, -0.3f, -0.9f);
            L(-0.9f, -0.4f, -0.3f, 0.1f);
            L(-0.9f, -0.4f, 0.4f, -0.4f);
            L(0.4f, -0.4f, 1.0f, 0.1f);
            L(1.0f, 0.1f, 0.5f, 0.8f);
            L(0.5f, 0.8f, -0.2f, 0.8f);
            break;
        case Icon::Redo:
            L(0.9f, -0.4f, 0.3f, -0.9f);
            L(0.9f, -0.4f, 0.3f, 0.1f);
            L(0.9f, -0.4f, -0.4f, -0.4f);
            L(-0.4f, -0.4f, -1.0f, 0.1f);
            L(-1.0f, 0.1f, -0.5f, 0.8f);
            L(-0.5f, 0.8f, 0.2f, 0.8f);
            break;
    }
}

Response icon_button(Context& ctx, std::string_view label, Icon icon, bool selected) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    if (selected) {
        s.dl.rect(r, t.selection, t.corner_radius);
        s.dl.rect(r, with_alpha(t.accent, 0.6f), t.corner_radius, t.border_width);
        if (rs.focused && s.kbd_nav) s.dl.rect(r, t.focus_ring, t.corner_radius, t.focus_ring_width);
    } else {
        draw_frame(s, r, control_color(s, id, rs), rs.focused);
    }
    draw_icon(ctx, icon, r, selected ? t.accent : (rs.hovered ? t.text : t.text_muted));
    return rs;
}

Response icon_button(Context& ctx, std::string_view label, Icon icon) {
    return icon_button(ctx, label, icon, false);
}

}  // namespace rd
