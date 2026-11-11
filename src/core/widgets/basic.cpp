// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
using namespace detail;

Response label(Context& ctx, std::string_view text) {
    State& s = state(ctx);
    Rect r = layout_next(ctx);
    draw_text_fit(s, r, visible_label(text), s.cfg.theme.text, false);
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
    return {};
}

Response button(Context& ctx, std::string_view label) {
    State& s = state(ctx);
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    draw_frame(s, r, control_color(s, id, rs), rs.focused);
    draw_text_fit(s, r, visible_label(label), s.cfg.theme.text, true);
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
    }
    const float bs = std::fmin(r.h * 0.64f, r.w);
    Rect box{r.x, r.y + (r.h - bs) / 2, bs, bs};
    const float on = anim_to(s, hash_u32(id, 77), value ? 1.0f : 0.0f, 0.12f, value ? 1.0f : 0.0f);
    draw_frame(s, box, lerp(control_color(s, id, rs), t.accent, on), rs.focused);
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
    fraction = fraction < 0 ? 0 : (fraction > 1 ? 1 : fraction);
    s.dl.rect(bar, t.background, bar.h / 2);
    s.dl.rect({bar.x, bar.y, bar.w * fraction, bar.h}, t.accent, bar.h / 2);
    s.dl.rect(bar, t.border, bar.h / 2, t.border_width);
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
    return rs;
}

bool collapsing_header(Context& ctx, std::string_view label, bool& open) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.clicked || (rs.focused && key_pressed(s, KeyEnter))) open = !open;
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
