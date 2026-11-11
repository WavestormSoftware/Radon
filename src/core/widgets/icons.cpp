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
        case Icon::ArrowLeft:
            L(-1, 0, 1, 0);
            L(-1, 0, -0.3f, -0.7f);
            L(-1, 0, -0.3f, 0.7f);
            break;
        case Icon::ArrowRight:
            L(-1, 0, 1, 0);
            L(1, 0, 0.3f, -0.7f);
            L(1, 0, 0.3f, 0.7f);
            break;
        case Icon::ArrowUp:
            L(0, -1, 0, 1);
            L(0, -1, -0.7f, -0.3f);
            L(0, -1, 0.7f, -0.3f);
            break;
        case Icon::ArrowDown:
            L(0, -1, 0, 1);
            L(0, 1, -0.7f, 0.3f);
            L(0, 1, 0.7f, 0.3f);
            break;
        case Icon::Home:
            L(-1.1f, 0, 0, -1);
            L(0, -1, 1.1f, 0);
            L(-0.75f, -0.3f, -0.75f, 0.9f);
            L(0.75f, -0.3f, 0.75f, 0.9f);
            L(-0.75f, 0.9f, 0.75f, 0.9f);
            break;
        case Icon::User:
            ring(0, -0.45f, 0.45f);
            L(-0.9f, 1, -0.6f, 0.35f);
            L(-0.6f, 0.35f, 0.6f, 0.35f);
            L(0.6f, 0.35f, 0.9f, 1);
            break;
        case Icon::Bell:
            d.rect({m.x - 0.7f * u, m.y - 0.9f * u, 1.4f * u, 1.9f * u}, c, 0.7f * u, w);
            L(-1, 0.6f, 1, 0.6f);
            dot(0, 0.95f, 0.18f);
            break;
        case Icon::Heart:
            L(0, 0.9f, -1, -0.1f);
            L(-1, -0.1f, -0.55f, -0.7f);
            L(-0.55f, -0.7f, 0, -0.25f);
            L(0, -0.25f, 0.55f, -0.7f);
            L(0.55f, -0.7f, 1, -0.1f);
            L(1, -0.1f, 0, 0.9f);
            break;
        case Icon::Info:
            ring(0, 0, 1);
            L(0, -0.1f, 0, 0.55f);
            dot(0, -0.5f, 0.12f);
            break;
        case Icon::Warning:
            L(0, -1, 1.1f, 0.85f);
            L(1.1f, 0.85f, -1.1f, 0.85f);
            L(-1.1f, 0.85f, 0, -1);
            L(0, -0.3f, 0, 0.25f);
            dot(0, 0.55f, 0.1f);
            break;
        case Icon::Calendar:
            d.rect({m.x - 0.95f * u, m.y - 0.75f * u, 1.9f * u, 1.7f * u}, c, 2, w);
            L(-0.95f, -0.2f, 0.95f, -0.2f);
            L(-0.45f, -1, -0.45f, -0.55f);
            L(0.45f, -1, 0.45f, -0.55f);
            break;
        case Icon::Clock:
            ring(0, 0, 1);
            L(0, -0.55f, 0, 0);
            L(0, 0, 0.4f, 0.3f);
            break;
        case Icon::Sun:
            ring(0, 0, 0.45f);
            for (int i = 0; i < 8; i++) {
                float a = (float)i * 0.785398f, cx = std::cos(a), cy = std::sin(a);
                L(cx * 0.75f, cy * 0.75f, cx * 1.1f, cy * 1.1f);
            }
            break;
        case Icon::Moon:
            for (int i = 0; i < 10; i++) {  // outer arc, then a shallower inner arc back
                float a0 = 0.9f + (float)i * 0.45f, a1 = a0 + 0.45f;
                L(std::cos(a0), std::sin(a0), std::cos(a1), std::sin(a1));
            }
            L(std::cos(0.9f), std::sin(0.9f), 0.1f, 0.05f);
            L(0.1f, 0.05f, std::cos(5.4f), std::sin(5.4f));
            break;
        case Icon::Play:
            L(-0.6f, -0.9f, 0.9f, 0);
            L(0.9f, 0, -0.6f, 0.9f);
            L(-0.6f, 0.9f, -0.6f, -0.9f);
            break;
        case Icon::Pause:
            L(-0.45f, -0.9f, -0.45f, 0.9f);
            L(0.45f, -0.9f, 0.45f, 0.9f);
            break;
        case Icon::Refresh:
            for (int i = 0; i < 9; i++) {
                float a0 = -0.6f + (float)i * 0.6f, a1 = a0 + 0.6f;
                L(std::cos(a0) * 0.9f, std::sin(a0) * 0.9f, std::cos(a1) * 0.9f, std::sin(a1) * 0.9f);
            }
            L(0.74f, -0.51f, 0.95f, -1.05f);
            L(0.74f, -0.51f, 0.2f, -0.6f);
            break;
        case Icon::Copy:
            d.rect({m.x - 0.95f * u, m.y - 0.95f * u, 1.35f * u, 1.35f * u}, c, 2, w);
            d.rect({m.x - 0.4f * u, m.y - 0.4f * u, 1.35f * u, 1.35f * u}, c, 2, w);
            break;
        case Icon::Download:
            L(0, -1, 0, 0.35f);
            L(0, 0.35f, -0.6f, -0.25f);
            L(0, 0.35f, 0.6f, -0.25f);
            L(-0.95f, 0.9f, 0.95f, 0.9f);
            break;
        case Icon::Share:
            dot(-0.7f, 0, 0.28f);
            dot(0.7f, -0.75f, 0.28f);
            dot(0.7f, 0.75f, 0.28f);
            L(-0.7f, 0, 0.7f, -0.75f);
            L(-0.7f, 0, 0.7f, 0.75f);
            break;
        case Icon::Filter:
            L(-1, -0.8f, 1, -0.8f);
            L(-1, -0.8f, -0.2f, 0.1f);
            L(1, -0.8f, 0.2f, 0.1f);
            L(-0.2f, 0.1f, -0.2f, 1);
            L(0.2f, 0.1f, 0.2f, 0.8f);
            L(-0.2f, 1, 0.2f, 0.8f);
            break;
        case Icon::Grid:
            for (float y : {-0.95f, 0.15f})
                for (float x : {-0.95f, 0.15f})
                    d.rect({m.x + x * u, m.y + y * u, 0.8f * u, 0.8f * u}, c, 2, w);
            break;
        case Icon::List:
            for (float y : {-0.7f, 0.0f, 0.7f}) {
                dot(-0.9f, y, 0.13f);
                L(-0.45f, y, 1, y);
            }
            break;
        case Icon::Image:
            d.rect({m.x - 1.0f * u, m.y - 0.8f * u, 2.0f * u, 1.6f * u}, c, 2, w);
            L(-0.8f, 0.6f, -0.2f, -0.1f);
            L(-0.2f, -0.1f, 0.3f, 0.4f);
            L(0.3f, 0.4f, 0.55f, 0.15f);
            L(0.55f, 0.15f, 0.85f, 0.6f);
            dot(0.45f, -0.4f, 0.15f);
            break;
        case Icon::Mail:
            d.rect({m.x - 1.0f * u, m.y - 0.7f * u, 2.0f * u, 1.4f * u}, c, 2, w);
            L(-1, -0.7f, 0, 0.1f);
            L(0, 0.1f, 1, -0.7f);
            break;
        case Icon::Phone:
            d.rect({m.x - 0.55f * u, m.y - 1.0f * u, 1.1f * u, 2.0f * u}, c, 0.2f * u, w);
            L(-0.2f, 0.75f, 0.2f, 0.75f);
            break;
        case Icon::Link:
            d.rect({m.x - 1.05f * u, m.y - 0.35f * u, 1.2f * u, 0.7f * u}, c, 0.35f * u, w);
            d.rect({m.x - 0.15f * u, m.y - 0.35f * u, 1.2f * u, 0.7f * u}, c, 0.35f * u, w);
            break;
        case Icon::Upload:
            L(0, 0.35f, 0, -1);
            L(0, -1, -0.6f, -0.4f);
            L(0, -1, 0.6f, -0.4f);
            L(-0.95f, 0.9f, 0.95f, 0.9f);
            break;
        case Icon::Send:
            L(-1, -0.9f, 1, 0);
            L(1, 0, -1, 0.9f);
            L(-1, 0.9f, -0.6f, 0);
            L(-0.6f, 0, -1, -0.9f);
            L(-0.6f, 0, 0.2f, 0);
            break;
        case Icon::Archive:
            d.rect({m.x - 1.0f * u, m.y - 0.9f * u, 2.0f * u, 0.5f * u}, c, 2, w);
            d.rect({m.x - 0.85f * u, m.y - 0.4f * u, 1.7f * u, 1.3f * u}, c, 2, w);
            L(-0.3f, 0.05f, 0.3f, 0.05f);
            break;
        case Icon::Bookmark:
            L(-0.6f, -1, 0.6f, -1);
            L(-0.6f, -1, -0.6f, 1);
            L(0.6f, -1, 0.6f, 1);
            L(-0.6f, 1, 0, 0.45f);
            L(0, 0.45f, 0.6f, 1);
            break;
        case Icon::Pin:
            ring(0, -0.35f, 0.55f);
            L(-0.45f, 0, 0, 1);
            L(0.45f, 0, 0, 1);
            break;
        case Icon::Backspace:
            L(-1.1f, 0, -0.5f, -0.75f);
            L(-0.5f, -0.75f, 1, -0.75f);
            L(1, -0.75f, 1, 0.75f);
            L(1, 0.75f, -0.5f, 0.75f);
            L(-0.5f, 0.75f, -1.1f, 0);
            L(-0.1f, -0.35f, 0.6f, 0.35f);
            L(0.6f, -0.35f, -0.1f, 0.35f);
            break;
        case Icon::Code:
            L(-0.4f, -0.8f, -1, 0);
            L(-1, 0, -0.4f, 0.8f);
            L(0.4f, -0.8f, 1, 0);
            L(1, 0, 0.4f, 0.8f);
            L(0.15f, -1, -0.15f, 1);
            break;
        case Icon::Count:
            break;
    }
}

Response icon_button(Context& ctx, std::string_view label, Icon icon, bool selected) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next_w(ctx, std::fmax(t.cell_height, 0.0f));
    if (in_flow(s)) r.w = r.h;
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    a11y_node(s, A11yRole::Button, label, {}, selected ? A11yNode::Selected : 0u);
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
