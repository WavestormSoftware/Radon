// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
namespace detail {

void open_popup_id(State& s, ID id) {
    s.popup_id = id;
    s.popup_anchor = s.last_rect;
    s.popup_open_frame = s.frame;
    s.popup_seen = true;
}

bool popup_begin_id(Context& ctx, ID pid, float min_w) {
    State& s = state(ctx);
    if (s.popup_id != pid) return false;
    s.popup_seen = true;
    float h = 24;
    for (int i = 0; i < s.cfg.max_windows; i++)
        if (s.wins[i].used && s.wins[i].id == pid)
            h = std::fmin(std::fmax(s.wins[i].content_h, 24.0f), 260.0f);
    Rect r{s.popup_anchor.x, s.popup_anchor.y + s.popup_anchor.h + 2, std::fmax(s.popup_anchor.w, min_w), h};
    return begin_window_impl(ctx, pid, "", r, WindowBorder | WindowScrollable, true);
}

Response selectable(Context& ctx, std::string_view text, bool selected) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, text);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    Color bg = selected ? with_alpha(t.accent, 0.35f) : control_color(s, id, rs);
    s.dl.rect(r, bg, t.corner_radius);
    if (rs.focused && s.kbd_nav) s.dl.rect(r, t.focus_ring, t.corner_radius, 2);
    Rect tr{r.x + t.padding, r.y, r.w - 2 * t.padding, r.h};
    draw_text_fit(s, tr, visible_label(text), t.text, false);
    return rs;
}

}  // namespace detail

void tooltip(Context& ctx, std::string_view text) {
    auto& s = detail::state(ctx);
    if (!s.last_hovered || s.in.touch) return;
    size_t n = text.size() < sizeof(s.tip) - 1 ? text.size() : sizeof(s.tip) - 1;
    std::memcpy(s.tip, text.data(), n);
    s.tip[n] = 0;
    s.tip_pos = {s.in.pointer.x + 12, s.in.pointer.y + 18};
    s.tip_set = true;
}

void open_popup(Context& ctx, std::string_view id) {
    auto& s = detail::state(ctx);
    detail::open_popup_id(s, hash_id(detail::top_id(s), id));
}

void close_popup(Context& ctx) {
    detail::state(ctx).popup_id = 0;
}

bool popup_begin(Context& ctx, std::string_view id) {
    auto& s = detail::state(ctx);
    return detail::popup_begin_id(ctx, hash_id(detail::top_id(s), id), 120);
}

void popup_end(Context& ctx) {
    end_window(ctx);
}

bool combo(Context& ctx, std::string_view label, int& selected, const char* const* items, int count) {
    using namespace detail;
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if ((rs.clicked && s.popup_suppress != id) || (rs.focused && key_pressed(s, KeyEnter)))
        open_popup_id(s, id);
    draw_frame(s, r, control_color(s, id, rs), rs.focused);
    if (selected >= 0 && selected < count && items)
        draw_text_fit(s, {r.x + t.padding, r.y, r.w - 3 * t.padding - r.h * 0.3f, r.h}, items[selected],
                      t.text, false);
    const float a = r.h * 0.14f;
    const Vec2 c{r.x + r.w - t.padding - a, r.y + r.h / 2};
    s.dl.triangle({c.x - a, c.y - a * 0.6f}, {c.x + a, c.y - a * 0.6f}, {c.x, c.y + a * 0.8f}, t.text_muted);

    bool changed = false;
    if (popup_begin_id(ctx, id, r.w)) {
        for (int i = 0; i < count; i++) {
            layout_row(ctx, 1, nullptr, 0, 0);
            if (s.id_n < s.cfg.max_id_stack) s.id_stack[s.id_n++] = hash_u32(top_id(s), (uint32_t)i);
            Response ir = selectable(ctx, items[i], i == selected);
            if (s.id_n > 0) s.id_n--;
            if (ir.clicked) {
                changed = changed || selected != i;
                selected = i;
                s.popup_id = 0;
            }
        }
        popup_end(ctx);
    }
    return changed;
}

}  // namespace rd
