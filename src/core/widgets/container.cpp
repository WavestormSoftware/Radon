// Copyright (c) 2026 Wavestorm Software
// Child regions, trees, menus and modals.
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace {

ChildState* find_child(State& s, ID id) {
    ChildState *free_slot = nullptr, *stale = nullptr;
    for (int i = 0; i < s.cfg.max_children; i++) {
        ChildState& c = s.children[i];
        if (c.id == id) return &c;
        if (c.id == 0) {
            if (!free_slot) free_slot = &c;
        } else if (!stale && c.last_frame + 2 < s.frame) {
            stale = &c;
        }
    }
    ChildState* c = free_slot ? free_slot : stale;
    if (c) *c = ChildState{id, 0, 0, s.frame};
    return c;
}

}  // namespace

bool begin_child(Context& ctx, std::string_view label, float height, bool border) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (s.cstack_n >= kMaxChildStack || s.layout_n >= s.cfg.max_layout_stack || s.clip_n >= kMaxClip) {
        s.error = true;
        return false;
    }
    ChildState* c = find_child(s, get_id(s, label));
    if (!c) {
        s.error = true;
        return false;
    }
    c->last_frame = s.frame;
    layout_row(ctx, 1, nullptr, 0, height);
    Rect r = layout_next(ctx);
    const float pad = border ? 4.0f : 0.0f;
    if (border) {
        s.dl.rect(r, t.background, t.corner_radius);
        s.dl.rect(r, t.border, t.corner_radius, t.border_width);
    }
    const float maxs = std::fmax(0.0f, c->content_h - r.h);
    c->scroll = std::fmin(std::fmax(c->scroll, 0.0f), maxs);
    const float sb = maxs > 0 ? t.scrollbar_width * 0.6f + 2 : 0;
    ChildFrame& f = s.cstack[s.cstack_n++];
    f = ChildFrame{};
    f.slot = (int)(c - s.children.get());
    f.rect = r;
    f.pad = pad;
    f.clip_base = s.clip_n;
    f.layout_index = s.layout_n;
    push_clip(s, r);
    push_layout(s, {r.x + pad, r.y + pad - c->scroll, r.w - 2 * pad - sb, r.h - 2 * pad});
    return true;
}

void end_child(Context& ctx) {
    State& s = state(ctx);
    if (s.cstack_n == 0) {
        s.error = true;
        return;
    }
    const Theme& t = s.cfg.theme;
    ChildFrame f = s.cstack[--s.cstack_n];
    ChildState& c = s.children[f.slot];
    const Layout& L = s.layouts[f.layout_index];
    c.content_h = L.max.y - L.body.y + 2 * f.pad;
    const float maxs = std::fmax(0.0f, c.content_h - f.rect.h);
    s.layout_n = f.layout_index;
    if (maxs > 0) {
        const bool inside = f.rect.contains(s.in.pointer) && s.hover_window == s.cur_window;
        if (inside && !s.wheel_used && s.in.wheel != 0) {
            float ns = std::fmin(std::fmax(c.scroll - s.in.wheel * t.cell_height, 0.0f), maxs);
            if (ns != c.scroll) {
                c.scroll = ns;
                s.wheel_used = true;
            }
        }
        if (s.in.touch && s.in.pointer_down && s.touch_moved && !s.active_captures &&
            f.rect.contains(s.press_pos) && s.press_window == s.cur_window)
            c.scroll = std::fmin(std::fmax(c.scroll - s.pointer_delta.y, 0.0f), maxs);
        const float sbw = t.scrollbar_width * 0.6f;
        Rect track{f.rect.x + f.rect.w - sbw - 2, f.rect.y + 3, sbw, f.rect.h - 6};
        const float thumb_h = std::fmax(20.0f, track.h * f.rect.h / c.content_h);
        const float room = std::fmax(1.0f, track.h - thumb_h);
        Rect thumb{track.x, track.y + room * (c.scroll / maxs), sbw, thumb_h};
        ID tid = hash_u32(c.id, 9);
        const Rect saved_rect = s.last_rect;
        const ID saved_id = s.last_id;
        const bool saved_hov = s.last_hovered;
        update_control(ctx, tid, thumb, false, true);
        s.last_rect = saved_rect;
        s.last_id = saved_id;
        s.last_hovered = saved_hov;
        if (s.active == tid && s.in.pointer_down) c.scroll += s.pointer_delta.y * (maxs / room);
        c.scroll = std::fmin(std::fmax(c.scroll, 0.0f), maxs);
        thumb.y = track.y + room * (c.scroll / maxs);
        s.dl.rect(thumb, (s.hover == tid || s.active == tid) ? t.accent : t.surface_active, sbw / 2);
    } else {
        c.scroll = 0;
    }
    while (s.clip_n > f.clip_base) pop_clip(s);
}

namespace {
void shift(Context& ctx, float px) {
    State& s = state(ctx);
    if (s.layout_n == 0) return;
    Layout& l = top_layout(s);
    l.body.x += px;
    l.body.w -= px;
}
}  // namespace

void indent(Context& ctx, float px) {
    shift(ctx, px < 0 ? state(ctx).cfg.theme.indent : px);
}
void unindent(Context& ctx, float px) {
    shift(ctx, -(px < 0 ? state(ctx).cfg.theme.indent : px));
}

bool tree_node(Context& ctx, std::string_view label, bool& open) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.clicked || (rs.focused && key_pressed(s, KeyEnter))) open = !open;
    s.dl.rect(r, with_alpha(t.surface_hover, animate(s, id, rs.hovered, false)), t.corner_radius);
    if (rs.focused && s.kbd_nav) s.dl.rect(r, t.focus_ring, t.corner_radius, t.focus_ring_width);
    draw_icon(ctx, open ? Icon::ChevronDown : Icon::ChevronRight, {r.x, r.y, r.h, r.h}, t.text_muted);
    draw_text_fit(s, {r.x + r.h, r.y, r.w - r.h, r.h}, visible_label(label), t.text, false);
    if (open) indent(ctx);
    return open;
}

void tree_pop(Context& ctx) {
    unindent(ctx);
}

bool begin_menu_bar(Context& ctx) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    layout_row(ctx, 1, nullptr, 0, 0);
    Rect r = layout_next(ctx);
    s.dl.rect(r, t.surface_hover, t.corner_radius);
    s.menu_bar = r;
    s.menu_x = r.x + 2;
    s.in_menu = true;
    return true;
}

void end_menu_bar(Context& ctx) {
    state(ctx).in_menu = false;
}

bool begin_menu(Context& ctx, std::string_view label) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (!s.in_menu) {
        s.error = true;
        return false;
    }
    ID id = get_id(s, label);
    std::string_view text = visible_label(label);
    const float w = text_size(s, t.font_body, text).x + 2 * t.padding;
    Rect tr{s.menu_x, s.menu_bar.y, w, s.menu_bar.h};
    s.menu_x += w;
    Response rs = update_control(ctx, id, tr, true, false);
    const bool menu_open = s.popup_id != 0 && s.popup_is_menu;
    if (rs.clicked && s.popup_suppress != id) {
        open_popup_id(s, id);
        s.popup_is_menu = true;
    } else if (menu_open && s.popup_id != id && rs.hovered) {  // sweep to a neighbouring menu
        open_popup_id(s, id);
        s.popup_is_menu = true;
    }
    const bool open = s.popup_id == id;
    if (open || rs.hovered) s.dl.rect(tr, open ? t.surface_active : t.surface_active, t.corner_radius);
    draw_text_fit(s, tr, text, t.text, true);
    return popup_begin_id(ctx, id, 180);
}

void end_menu(Context& ctx) {
    end_window(ctx);
}

bool menu_item(Context& ctx, std::string_view label, const char* shortcut, bool checked) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    layout_row(ctx, 1, nullptr, 0, 0);
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    s.dl.rect(r, with_alpha(t.selection, animate(s, id, rs.hovered, false)), t.corner_radius);
    if (rs.focused && s.kbd_nav) s.dl.rect(r, t.focus_ring, t.corner_radius, t.focus_ring_width);
    if (checked) draw_icon(ctx, Icon::Check, {r.x, r.y, r.h, r.h}, t.accent);
    draw_text_fit(s, {r.x + r.h, r.y, r.w - r.h - t.padding, r.h}, visible_label(label), t.text, false);
    if (shortcut) {
        Vec2 sz = text_size(s, t.font_small >= 0 ? t.font_small : t.font_body, shortcut);
        draw_text_centered(s, t.font_small >= 0 ? t.font_small : t.font_body,
                           {r.x + r.w - sz.x - t.padding, r.y, sz.x, r.h}, shortcut, t.text_muted, false);
    }
    if (rs.clicked) s.popup_id = 0;
    return rs.clicked;
}

void open_modal(Context& ctx, std::string_view id) {
    State& s = state(ctx);
    s.modal_id = hash_id(top_id(s), id);
    s.modal_open_frame = s.frame;
    s.modal_seen = true;
}

void close_modal(Context& ctx) {
    state(ctx).modal_id = 0;
}

bool modal_begin(Context& ctx, std::string_view id, std::string_view title, float width) {
    State& s = state(ctx);
    ID mid = hash_id(top_id(s), id);
    if (s.modal_id != mid) return false;
    s.modal_seen = true;
    // dim everything behind the dialog
    const uint32_t layer = s.cur_layer;
    s.dl.set_layer(kOverlayLayer);
    s.dl.break_command();
    s.dl.set_clip({-1e6f, -1e6f, 2e6f, 2e6f});
    const Vec2 sc = s.in.screen_size;
    s.dl.rect({0, 0, sc.x, sc.y}, {0, 0, 0, 150});
    s.dl.set_layer(layer);
    s.dl.break_command();
    float h = 60;
    for (int i = 0; i < s.cfg.max_windows; i++)
        if (s.wins[i].used && s.wins[i].id == mid)
            h = std::fmax(s.wins[i].content_h + s.cfg.theme.title_height, 60.0f);
    Rect r{(sc.x - width) / 2, (sc.y - h) / 2, width, h};
    return begin_window_impl(ctx, mid, title, r, WindowBorder | WindowTitleBar, true);
}

void modal_end(Context& ctx) {
    end_window(ctx);
}

}  // namespace rd
