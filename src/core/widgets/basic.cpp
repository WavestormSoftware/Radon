// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
using namespace detail;

Response label(Context& ctx, std::string_view text) {
    State& s = state(ctx);
    text = visible_label(text);
    Rect r = layout_next_w(ctx, text_size(s, s.cfg.theme.font_body, text).x);
    draw_text_fit(s, r, text, s.cfg.theme.text, false);
    a11y_static(s, A11yRole::Text, r, text);
    return {};
}

Response label_wrapped(Context& ctx, std::string_view text) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    TextLine lines[32];
    text = visible_label(text);
    int n = wrap_text(ctx, t.font_body, text, r.w, lines, 32);
    float lh = measure_text(ctx, t.font_body, "M").y;
    extend_height(s, r, n * lh);
    for (int i = 0; i < n; i++)
        draw_text(s, t.font_body, {r.x, r.y + i * lh}, text.substr(lines[i].start, lines[i].length), t.text);
    a11y_static(s, A11yRole::Text, r, text);
    return {};
}

Response button(Context& ctx, std::string_view label) {
    State& s = state(ctx);
    ID id = get_id(s, label);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next_w(
        ctx, std::fmax(t.cell_height, text_size(s, t.font_body, visible_label(label)).x + 2 * t.padding));
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    draw_frame(s, r, control_color(s, id, rs), rs.focused);
    draw_text_fit(s, r, visible_label(label), t.text, true);
    a11y_node(s, A11yRole::Button, label);
    return rs;
}

Response checkbox(Context& ctx, std::string_view label, bool& value) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.clicked || (rs.focused && key_pressed(s, KeySpace))) {
        value = !value;
        rs.changed = true;
        haptic(s, Haptic::Light);
    }
    a11y_node(s, A11yRole::Checkbox, label, value ? "on" : "off", value ? A11yNode::Checked : 0u);
    const float bs = std::fmin(r.h * 0.64f, r.w);
    Rect box{r.x, r.y + (r.h - bs) / 2, bs, bs};
    const float on = anim_to(s, hash_u32(id, 77), value ? 1.0f : 0.0f, 0.12f, value ? 1.0f : 0.0f);
    draw_frame(s, box, lerp(control_color(s, id, rs), t.accent, on), rs.focused);
    // The theme border is too faint to find an empty box by (WCAG 1.4.11 wants 3:1): outline it.
    if (on < 0.98f && !(rs.focused && s.kbd_nav))
        s.dl.rect(box, with_alpha(t.text_muted, 0.7f * (1 - on)), t.corner_radius, 1.5f);
    if (on > 0.02f) {
        Color c = with_alpha(t.accent_text, on);
        s.dl.line({box.x + bs * 0.24f, box.y + bs * 0.52f}, {box.x + bs * 0.43f, box.y + bs * 0.72f}, c, 2);
        s.dl.line({box.x + bs * 0.43f, box.y + bs * 0.72f}, {box.x + bs * 0.77f, box.y + bs * 0.28f}, c, 2);
    }
    Rect lr{r.x + bs + t.spacing, r.y, r.w - bs - t.spacing, r.h};
    draw_text_fit(s, lr, visible_label(label), t.text, false);
    return rs;
}

Response progress_bar(Context& ctx, float fraction) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    Rect bar{r.x, r.y + r.h * 0.3f, r.w, r.h * 0.4f};
    const bool busy = fraction < 0;
    fraction = fraction < 0 ? 0 : (fraction > 1 ? 1 : fraction);
    s.dl.rect(bar, t.background, bar.h / 2);
    if (busy) {  // a 30% segment sweeping across; still when animations are off
        const float ph = s.cfg.animations ? std::fmod(s.time * 0.7f, 1.0f) : 0.35f;
        const float x0 = std::fmax(bar.x, bar.x + bar.w * (ph * 1.3f - 0.3f));
        const float x1 = std::fmin(bar.x + bar.w, bar.x + bar.w * ph * 1.3f);
        push_clip(s, bar);
        if (x1 > x0) s.dl.rect({x0, bar.y, x1 - x0, bar.h}, t.accent, bar.h / 2);
        pop_clip(s);
        if (s.cfg.animations) s.animating = true;
    } else {
        s.dl.rect({bar.x, bar.y, bar.w * fraction, bar.h}, t.accent, bar.h / 2);
    }
    s.dl.rect(bar, t.border, bar.h / 2, t.border_width);
    if (s.cfg.a11y) {
        char v[16];
        std::snprintf(v, sizeof v, "%d%%", (int)(fraction * 100 + 0.5f));
        if (busy) std::snprintf(v, sizeof v, "busy");
        s.last_id = get_id(s, "##progress");
        s.last_rect = r;
        a11y_node(s, A11yRole::ProgressBar, "progress", v);
    }
    return {};
}

Response image(Context& ctx, TextureId tex, Vec2 size, Rect uv) {
    State& s = state(ctx);
    Rect r = layout_next(ctx);
    extend_height(s, r, size.y);
    r.w = std::fmin(r.w, size.x);
    r.h = size.y;
    Response rs = update_control(ctx, get_id(s, "##image") ^ (ID)tex, r, false, false);
    s.dl.image(tex, r, uv, {255, 255, 255, 255});
    a11y_static(s, A11yRole::Image, r, "image");
    return rs;
}

ViewportResponse viewport(Context& ctx, TextureId tex, Vec2 size, bool flip_y) {
    State& s = state(ctx);
    Rect r = layout_next(ctx);
    extend_height(s, r, size.y);
    r.w = std::fmin(r.w, size.x);
    r.h = size.y;
    const ID id = get_id(s, "##viewport") ^ (ID)tex;
    const bool was_active = s.active == id;
    ViewportResponse vr;
    static_cast<Response&>(vr) = update_control(ctx, id, r, true, true);
    vr.rect = r;
    vr.local = {s.in.pointer.x - r.x, s.in.pointer.y - r.y};
    vr.down = (vr.pressed || was_active) && s.in.pointer_down;
    vr.released = (vr.pressed || was_active) && s.in.pointer_released;
    if (vr.hovered) vr.wheel = s.in.wheel;
    if (vr.focused) vr.keys = &s.in.keys;
    if (r.contains(s.gesture.center) || vr.down) vr.pinch = s.gesture.pinch, vr.pan2 = s.gesture.pan;
    s.dl.image(tex, r, flip_y ? Rect{0, 1, 1, -1} : Rect{0, 0, 1, 1}, {255, 255, 255, 255});
    return vr;
}

Canvas canvas_begin(Context& ctx, Vec2 size, Color background) {
    State& s = state(ctx);
    Rect r = layout_next(ctx);
    if (size.x <= 0) size.x = r.w;  // fill the cell (e.g. a layout_row with height -1)
    if (size.y <= 0) size.y = r.h;
    extend_height(s, r, size.y);
    r.w = std::fmin(r.w, size.x);
    r.h = size.y;
    Canvas c;
    c.ctx_ = &ctx;
    c.rect = r;
    c.local = {s.in.pointer.x - r.x, s.in.pointer.y - r.y};
    c.response = update_control(ctx, get_id(s, "##canvas"), r, true, true);
    if (r.contains(s.gesture.center) || s.active == s.last_id)
        c.pinch = s.gesture.pinch, c.pan2 = s.gesture.pan;
    if (background.a) s.dl.rect(r, background);
    push_clip(s, r);
    return c;
}

void canvas_end(Context& ctx) {
    pop_clip(state(ctx));
}

void Canvas::fill_rect(Rect r, Color c, float radius) {
    state(*ctx_).dl.rect({rect.x + r.x, rect.y + r.y, r.w, r.h}, c, radius);
}
void Canvas::stroke_rect(Rect r, Color c, float thickness, float radius) {
    state(*ctx_).dl.rect({rect.x + r.x, rect.y + r.y, r.w, r.h}, c, radius, thickness);
}
void Canvas::circle(Vec2 center, float radius, Color c) {
    state(*ctx_).dl.rect({rect.x + center.x - radius, rect.y + center.y - radius, radius * 2, radius * 2}, c,
                         radius);
}
void Canvas::line(Vec2 a, Vec2 b, Color c, float thickness) {
    state(*ctx_).dl.line({rect.x + a.x, rect.y + a.y}, {rect.x + b.x, rect.y + b.y}, c, thickness);
}
void Canvas::triangle(Vec2 a, Vec2 b, Vec2 c, Color col) {
    state(*ctx_).dl.triangle({rect.x + a.x, rect.y + a.y}, {rect.x + b.x, rect.y + b.y},
                             {rect.x + c.x, rect.y + c.y}, col);
}
void Canvas::sprite(TextureId tex, Rect dst, Rect uv, Color tint) {
    state(*ctx_).dl.image(tex, {rect.x + dst.x, rect.y + dst.y, dst.w, dst.h}, uv, tint);
}

bool collapsing_header(Context& ctx, std::string_view label, bool& open) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.clicked || (rs.focused && key_pressed(s, KeyEnter))) open = !open;
    a11y_node(s, A11yRole::TreeItem, label, {}, open ? A11yNode::Expanded : A11yNode::Collapsed);
    draw_frame(s, r, control_color(s, id, rs), rs.focused);
    const float a = r.h * 0.18f;
    const Vec2 c{r.x + t.padding + a, r.y + r.h / 2};
    if (open)
        s.dl.triangle({c.x - a, c.y - a * 0.6f}, {c.x + a, c.y - a * 0.6f}, {c.x, c.y + a * 0.8f}, t.text);
    else
        s.dl.triangle({c.x - a * 0.6f, c.y - a}, {c.x + a * 0.8f, c.y}, {c.x - a * 0.6f, c.y + a}, t.text);
    Rect tr{r.x + t.padding * 2 + a * 2, r.y, r.w - t.padding * 3 - a * 2, r.h};
    draw_text_fit(s, tr, visible_label(label), t.text, false);
    return open;
}

}  // namespace rd
