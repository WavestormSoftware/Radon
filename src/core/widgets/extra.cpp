// Copyright (c) 2026 Wavestorm Software
// Radio, toggle, numeric widgets, tabs, lists and small display widgets.
#include "../internal.hpp"

namespace rd {
using namespace detail;

Response button(Context& ctx, std::string_view label, ButtonStyle style) {
    if (style == ButtonStyle::Default) return button(ctx, label);
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next_w(
        ctx, std::fmax(t.cell_height, text_size(s, t.font_body, visible_label(label)).x + 2 * t.padding));
    Response rs = update_control(ctx, id, r, true, false);
    a11y_node(s, A11yRole::Button, label);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    float press = 0;
    float h = animate(s, id, rs.hovered, s.active == id, &press);
    const float k = clamp01(0.6f * h + 0.4f * press);
    Paint fill;
    Color text = t.text, ring{0, 0, 0, 0};
    switch (style) {
        case ButtonStyle::Primary:
            fill = lighten(pick(t.accent_paint, t.accent), 0.18f * k);
            text = t.accent_text;
            break;
        case ButtonStyle::Danger:
            fill = with_alpha(t.error, 0.12f + 0.25f * k);
            ring = t.error;
            text = t.error;
            break;
        default:  // Ghost
            fill = with_alpha(t.surface_active, k);
            break;
    }
    s.dl.fill(r, fill, t.corner_radius);
    if (ring.a) s.dl.rect(r, ring, t.corner_radius, t.border_width);
    if (rs.focused && s.kbd_nav) s.dl.rect(r, t.focus_ring, t.corner_radius, t.focus_ring_width);
    draw_text_fit(s, r, visible_label(label), text, true);
    return rs;
}

Response radio_button(Context& ctx, std::string_view label, int& value, int option) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.clicked || (rs.focused && (key_pressed(s, KeySpace) || key_pressed(s, KeyEnter)))) {
        if (value != option) rs.changed = true, haptic(s, Haptic::Selection);
        value = option;
    }
    a11y_node(s, A11yRole::Radio, label, {}, value == option ? A11yNode::Checked : 0u);
    const float bs = std::fmin(r.h * 0.64f, r.w);
    Rect box{r.x, r.y + (r.h - bs) / 2, bs, bs};
    const bool on = value == option;
    s.dl.rect(box, control_color(s, id, rs), bs / 2);
    bool ring = rs.focused && s.kbd_nav;
    s.dl.rect(box, ring ? t.focus_ring : (on ? t.accent : with_alpha(t.text_muted, 0.7f)), bs / 2,
              ring ? t.focus_ring_width : 1.5f);
    const float k = anim_to(s, hash_u32(id, 77), on ? 1.0f : 0.0f, 0.12f, on ? 1.0f : 0.0f) * bs * 0.22f;
    if (k > 0.1f) s.dl.rect({box.x + bs / 2 - k, box.y + bs / 2 - k, 2 * k, 2 * k}, t.accent, k);
    draw_text_fit(s, {r.x + bs + t.spacing, r.y, r.w - bs - t.spacing, r.h}, visible_label(label), t.text,
                  false);
    return rs;
}

Response toggle(Context& ctx, std::string_view label, bool& value) {
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
    a11y_node(s, A11yRole::Switch, label, value ? "on" : "off", value ? A11yNode::Checked : 0u);
    const float th = std::fmin(r.h * 0.62f, r.w), tw = th * 1.8f;
    Rect track{r.x, r.y + (r.h - th) / 2, tw, th};
    const float on = anim_to(s, hash_u32(id, 77), value ? 1.0f : 0.0f, 0.15f, value ? 1.0f : 0.0f);
    s.dl.rect(track, lerp(t.surface_active, t.accent, on), th / 2);
    bool ring = rs.focused && s.kbd_nav;
    if (ring)
        s.dl.rect(track, t.focus_ring, th / 2, t.focus_ring_width);
    else if (on < 0.98f)  // same outline as an empty checkbox, so "off" is still findable
        s.dl.rect(track, with_alpha(t.text_muted, 0.5f * (1 - on)), th / 2, 1.5f);
    const float kd = th - 6;
    s.dl.rect({track.x + 3 + (tw - th) * on, track.y + 3, kd, kd}, {255, 255, 255, 255}, kd / 2);
    draw_text_fit(s, {r.x + tw + t.spacing, r.y, r.w - tw - t.spacing, r.h}, visible_label(label), t.text,
                  false);
    return rs;
}

Response slider_int(Context& ctx, std::string_view label, int& value, int lo, int hi) {
    float f = (float)value;
    Response rs = slider(ctx, label, f, (float)lo, (float)hi, "%.0f");
    int nv = (int)std::lround(f);
    rs.changed = nv != value;
    value = nv;
    return rs;
}

Response drag_int(Context& ctx, std::string_view label, int& value, float speed) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, true);
    if (rs.pressed) s.drag_acc = 0;
    if (s.active == id && s.in.pointer_down) {
        s.drag_acc += s.pointer_delta.x * speed;
        int whole = (int)s.drag_acc;
        if (whole != 0) {
            value += whole;
            s.drag_acc -= (float)whole;
            rs.changed = true;
        }
    }
    if (rs.focused) {
        if (key_pressed(s, KeyLeft)) value--, rs.changed = true;
        if (key_pressed(s, KeyRight)) value++, rs.changed = true;
    }
    draw_frame(s, r, control_color(s, id, rs), rs.focused);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%d", value);
    draw_text_fit(s, r, buf, t.text, true);
    return rs;
}

Response stepper(Context& ctx, std::string_view label, float& value, float step, float lo, float hi,
                 const char* format) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    const float bw = std::fmin(r.h, r.w / 3);
    Rect left{r.x, r.y, bw, r.h}, right{r.x + r.w - bw, r.y, bw, r.h},
        mid{r.x + bw + 2, r.y, r.w - 2 * bw - 4, r.h};
    Response lr = update_control(ctx, hash_u32(id, 1), left, true, false);
    Response rr = update_control(ctx, hash_u32(id, 2), right, true, false);
    Response rs = update_control(ctx, id, mid, true, true);
    float nv = value;
    if (lr.clicked) nv -= step;
    if (rr.clicked) nv += step;
    if (s.active == id && s.in.pointer_down) nv += s.pointer_delta.x / 8.0f * step;
    if (rs.focused) {
        if (key_pressed(s, KeyLeft)) nv -= step;
        if (key_pressed(s, KeyRight)) nv += step;
    }
    nv = nv < lo ? lo : (nv > hi ? hi : nv);
    rs.changed = nv != value;
    value = nv;
    rs.hovered = rs.hovered || lr.hovered || rr.hovered;
    draw_frame(s, left, control_color(s, hash_u32(id, 1), lr), lr.focused);
    draw_frame(s, right, control_color(s, hash_u32(id, 2), rr), rr.focused);
    draw_frame(s, mid, t.background, rs.focused);
    draw_icon(ctx, Icon::Minus, left, t.text);
    draw_icon(ctx, Icon::Plus, right, t.text);
    char buf[48];
    std::snprintf(buf, sizeof buf, format ? format : "%.2f", (double)value);
    draw_text_fit(s, mid, buf, t.text, true);
    return rs;
}

Response stepper_int(Context& ctx, std::string_view label, int& value, int step, int lo, int hi) {
    float f = (float)value;
    Response rs = stepper(ctx, label, f, (float)step, (float)lo, (float)hi, "%.0f");
    int nv = (int)std::lround(f);
    rs.changed = nv != value;
    value = nv;
    return rs;
}

bool tab_bar(Context& ctx, std::string_view label, int& selected, const char* const* tabs, int count) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID base = get_id(s, label);
    Rect r = layout_next(ctx);
    s.dl.rect({r.x, r.y + r.h - 1, r.w, 1}, t.border);
    float x = r.x;
    bool changed = false;
    for (int i = 0; i < count; i++) {
        Vec2 sz = text_size(s, t.font_body, tabs[i]);
        Rect tr{x, r.y, sz.x + 2 * t.padding, r.h};
        x += tr.w;
        Response rs = update_control(ctx, hash_u32(base, (uint32_t)i), tr, true, false);
        a11y_node(s, A11yRole::Tab, tabs[i], {}, selected == i ? A11yNode::Selected : 0u);
        if (rs.clicked || (rs.focused && key_pressed(s, KeyEnter))) {
            changed = changed || selected != i;
            selected = i;
        }
        const bool on = selected == i;
        const float h = animate(s, hash_u32(base, (uint32_t)i), rs.hovered, false);
        Color c = on ? t.text : lerp(t.text_muted, t.text, h);
        draw_text_fit(s, tr, tabs[i], c, true);
        if (on) {
            const float ux = anim_to(s, hash_u32(base, 1001), tr.x - r.x, 0.18f, tr.x - r.x);
            const float uw = anim_to(s, hash_u32(base, 1002), tr.w, 0.18f, tr.w);
            s.dl.rect({r.x + ux + t.padding * 0.5f, r.y + r.h - 2, uw - t.padding, 2}, t.accent, 1);
        }
        if (rs.focused && s.kbd_nav) s.dl.rect(tr, t.focus_ring, t.corner_radius, t.focus_ring_width);
    }
    return changed;
}

namespace {
Response list_row(Context& ctx, std::string_view label, bool selected, const Icon* icon) {
    State& s = state(ctx);
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    a11y_node(s, A11yRole::ListItem, label, {}, selected ? A11yNode::Selected : 0u);
    const Theme& t = s.cfg.theme;
    Color bg = selected ? t.selection : with_alpha(t.surface_hover, animate(s, id, rs.hovered, false));
    s.dl.rect(r, bg, t.corner_radius);
    if (selected) s.dl.rect(r, with_alpha(t.accent, 0.35f), t.corner_radius, t.border_width);
    if (rs.focused && s.kbd_nav) s.dl.rect(r, t.focus_ring, t.corner_radius, t.focus_ring_width);
    float tx = r.x + t.padding;
    if (icon) {
        const float side = std::fmin(r.h, 28.0f);  // rows grow with touch themes; the icon does not
        draw_icon(ctx, *icon, {r.x + 4 + (r.h - side) / 2, r.y + (r.h - side) / 2, side, side},
                  selected ? t.accent : t.text_muted);
        tx += r.h;
    }
    draw_text_fit(s, {tx, r.y, r.x + r.w - tx - t.padding, r.h}, visible_label(label), t.text, false);
    return rs;
}
}  // namespace

Response list_item(Context& ctx, std::string_view label, bool selected) {
    return list_row(ctx, label, selected, nullptr);
}
Response list_item(Context& ctx, std::string_view label, bool selected, Icon icon) {
    return list_row(ctx, label, selected, &icon);
}

bool listbox(Context& ctx, std::string_view label, int& selected, const char* const* items, int count,
             float height) {
    State& s = state(ctx);
    bool changed = false;
    if (begin_child(ctx, label, height, true)) {
        for (int i = 0; i < count; i++) {
            layout_row(ctx, 1, nullptr, 0, 0);
            if (s.id_n < s.cfg.max_id_stack) s.id_stack[s.id_n++] = hash_u32(top_id(s), (uint32_t)i);
            if (list_item(ctx, items[i], i == selected).clicked) {
                changed = changed || selected != i;
                selected = i;
            }
            if (s.id_n > 0) s.id_n--;
        }
        end_child(ctx);
    }
    return changed;
}

Response label_colored(Context& ctx, std::string_view text, Color color) {
    State& s = state(ctx);
    Rect r = layout_next(ctx);
    draw_text_fit(s, r, visible_label(text), color, false);
    return {};
}

void badge(Context& ctx, std::string_view text, Color bg) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    text = visible_label(text);
    Vec2 sz = text_size(s, t.font_small >= 0 ? t.font_small : t.font_body, text);
    Rect r = layout_next_w(ctx, sz.x + 2 * t.padding);
    a11y_static(s, A11yRole::Text, r, text);
    const float h = std::fmin(r.h, r.h * 0.72f), w = std::fmin(r.w, sz.x + 2 * t.padding);
    Rect pill{r.x, r.y + (r.h - h) / 2, w, h};
    s.dl.rect(pill, bg, h / 2);
    draw_text_centered(s, t.font_small >= 0 ? t.font_small : t.font_body, pill, text, t.accent_text, true);
}

void bullet_text(Context& ctx, std::string_view text) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    const float d = 5;
    s.dl.rect({r.x + 4, r.y + r.h / 2 - d / 2, d, d}, t.text_muted, d / 2);
    draw_text_fit(s, {r.x + 16, r.y, r.w - 16, r.h}, visible_label(text), t.text, false);
}

void separator_text(Context& ctx, std::string_view text) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    layout_row(ctx, 1, nullptr, 0, 0);  // always a full-width row
    Rect r = layout_next(ctx);
    text = visible_label(text);
    Vec2 sz = text_size(s, t.font_body, text);
    draw_text_centered(s, t.font_body, {r.x, r.y, sz.x, r.h}, text, t.text_muted, false);
    const float x = r.x + sz.x + t.spacing;
    if (x < r.x + r.w) s.dl.rect({x, r.y + r.h / 2, r.x + r.w - x, 1}, t.border);
}

void spinner(Context& ctx) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    const float d = std::fmin(r.h, r.w) * 0.7f, rad = d / 2;
    const Vec2 c{r.x + r.h / 2, r.y + r.h / 2};
    const int n = 10;
    const float head = std::fmod(s.time * 1.2f, 1.0f);
    s.animating = true;
    for (int i = 0; i < n; i++) {
        float f = (float)i / n, a = f * 6.283185f;
        float fade = std::fmod(head - f + 1.0f, 1.0f);
        float sz = d * 0.14f;
        s.dl.rect({c.x + std::cos(a) * rad * 0.8f - sz / 2, c.y + std::sin(a) * rad * 0.8f - sz / 2, sz, sz},
                  with_alpha(t.accent, 1.0f - fade * 0.85f), sz / 2);
    }
}

namespace {
void plot(Context& ctx, const float* v, int n, float lo, float hi, float height, bool bars) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    extend_height(s, r, height);
    r.h = height;
    s.dl.rect(r, t.background, t.corner_radius);
    s.dl.rect(r, t.border, t.corner_radius, t.border_width);
    if (!v || n < 1) return;
    if (lo == hi) {
        lo = hi = v[0];
        for (int i = 1; i < n; i++) lo = std::fmin(lo, v[i]), hi = std::fmax(hi, v[i]);
        if (lo == hi) hi = lo + 1;
    }
    const Rect in{r.x + 4, r.y + 4, r.w - 8, r.h - 8};
    auto Y = [&](float val) { return in.y + in.h * (1.0f - clamp01((val - lo) / (hi - lo))); };
    if (bars) {
        const float bw = in.w / (float)n;
        for (int i = 0; i < n; i++) {
            float y = Y(v[i]);
            s.dl.rect({in.x + bw * (float)i + 1, y, std::fmax(1.0f, bw - 2), in.y + in.h - y}, t.accent, 1);
        }
    } else if (n > 1) {
        for (int i = 0; i + 1 < n; i++) {
            float x0 = in.x + in.w * (float)i / (float)(n - 1),
                  x1 = in.x + in.w * (float)(i + 1) / (float)(n - 1);
            s.dl.line({x0, Y(v[i])}, {x1, Y(v[i + 1])}, t.accent, 1.5f);
        }
    }
}
}  // namespace

void plot_lines(Context& ctx, const float* v, int n, float lo, float hi, float h) {
    plot(ctx, v, n, lo, hi, h, false);
}
void plot_histogram(Context& ctx, const float* v, int n, float lo, float hi, float h) {
    plot(ctx, v, n, lo, hi, h, true);
}

void push_theme(Context& ctx, const Theme& theme) {
    State& s = state(ctx);
    if (s.theme_n >= kMaxThemeStack) {
        s.error = true;
        return;
    }
    s.theme_stack[s.theme_n++] = s.cfg.theme;
    s.cfg.theme = theme;
}

void pop_theme(Context& ctx) {
    State& s = state(ctx);
    if (s.theme_n > 0) s.cfg.theme = s.theme_stack[--s.theme_n];
}

}  // namespace rd
