// Copyright (c) 2026 Wavestorm Software
// Child regions, trees, menus and modals.
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace detail {

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

}  // namespace detail

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

bool begin_card(Context& ctx, std::string_view label) {
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
    // The card is as tall as its content was last frame.
    layout_row(ctx, 1, nullptr, 0, std::fmax(c->content_h, t.cell_height));
    Rect r = layout_next(ctx);
    s.dl.fill(r, pick(t.card_paint, t.surface_hover), t.window_radius);
    s.dl.rect(r, t.border, t.window_radius, t.border_width);
    ChildFrame& f = s.cstack[s.cstack_n++];
    f = ChildFrame{};
    f.slot = (int)(c - s.children.get());
    f.rect = r;
    f.pad = t.padding;
    f.kind = ChildCard;
    f.clip_base = s.clip_n;
    f.layout_index = s.layout_n;
    push_clip(s, r);
    push_layout(s, {r.x + f.pad, r.y + f.pad, r.w - 2 * f.pad, r.h - 2 * f.pad});
    return true;
}

void end_card(Context& ctx) {
    State& s = state(ctx);
    if (s.cstack_n == 0 || s.cstack[s.cstack_n - 1].kind != ChildCard) {
        s.error = true;
        return;
    }
    ChildFrame f = s.cstack[--s.cstack_n];
    ChildState& c = s.children[f.slot];
    const Layout& L = s.layouts[f.layout_index];
    c.content_h = L.max.y - L.body.y + 2 * f.pad;
    if (c.content_h != f.rect.h) s.animating = true;  // one more frame to settle the height
    s.layout_n = f.layout_index;
    while (s.clip_n > f.clip_base) pop_clip(s);
}

bool carousel_begin(Context& ctx, std::string_view label, float height, float item_w) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (s.cstack_n >= kMaxChildStack || s.layout_n >= s.cfg.max_layout_stack || s.clip_n >= kMaxClip ||
        s.layout_n == 0) {
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
    const Rect r = layout_next(ctx);
    const Rect view{r.x, r.y, r.w, std::fmax(0.0f, r.h - 16)};  // page dots below
    item_w = std::fmin(item_w > 0 ? item_w : view.w, view.w);
    const float step = item_w + t.spacing;
    const float maxs = std::fmax(0.0f, c->content_h - view.w);
    const bool mine = view.contains(s.press_pos) && s.press_window == s.cur_window;
    if (s.in.pointer_pressed && mine) c->lock = 0;
    bool drag = false;
    if (s.in.touch && s.in.pointer_down && s.touch_moved && mine && !s.active_captures &&
        s.in.touch_count < 2) {
        const float dx = s.in.pointer.x - s.press_pos.x, dy = s.in.pointer.y - s.press_pos.y;
        if (c->lock == 0) c->lock = std::fabs(dx) > std::fabs(dy) ? 1 : 2;
        drag = c->lock == 1;
    }
    if (drag) {
        c->scroll = std::fmin(std::fmax(c->scroll - s.pointer_delta.x, -40.0f), maxs + 40);
        if (s.dt > 0) c->vel = 0.6f * c->vel + 0.4f * (-s.pointer_delta.x / s.dt);
        c->target = c->scroll;
        s.drag_used = true;  // the window must not scroll vertically too
    } else {
        if (c->lock == 1) {  // released: snap to the nearest item, a flick moves one further
            const float aim = c->scroll + std::fmax(-step, std::fmin(step, c->vel * 0.15f));
            c->target = std::round(aim / step) * step;
            c->lock = 0;
            c->vel = 0;
            haptic(s, Haptic::Selection);
        }
        if (view.contains(s.in.pointer) && s.hover_window == s.cur_window && s.in.wheel != 0 &&
            !s.wheel_used) {
            c->target = (std::round(c->target / step) + (s.in.wheel < 0 ? 1 : -1)) * step;
            s.wheel_used = true;
        }
        c->target = std::fmin(std::fmax(c->target, 0.0f), maxs);
        c->scroll += (c->target - c->scroll) * (s.cfg.animations ? 1 - std::exp(-s.dt * 12) : 1.0f);
        if (std::fabs(c->scroll - c->target) < 0.3f) c->scroll = c->target;
        if (c->scroll != c->target) s.animating = true;
    }
    ChildFrame& f = s.cstack[s.cstack_n++];
    f = ChildFrame{};
    f.slot = (int)(c - s.children.get());
    f.rect = r;
    f.kind = ChildStrip;
    f.extra = step;
    f.clip_base = s.clip_n;
    f.layout_index = s.layout_n;
    push_clip(s, view);
    push_layout(s, {view.x - c->scroll, view.y, view.w, view.h});
    Layout& l = top_layout(s);
    l.mode = LayoutStrip;
    l.flow_w = item_w;
    l.row_h = view.h;
    return true;
}

void carousel_end(Context& ctx) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (s.cstack_n == 0 || s.cstack[s.cstack_n - 1].kind != ChildStrip) {
        s.error = true;
        return;
    }
    const ChildFrame f = s.cstack[--s.cstack_n];
    ChildState& c = s.children[f.slot];
    const Layout& L = s.layouts[f.layout_index];
    c.content_h = L.max.x - L.body.x;  // the strip's width
    s.layout_n = f.layout_index;
    while (s.clip_n > f.clip_base) pop_clip(s);
    const int n = (int)std::lround((c.content_h + t.spacing) / f.extra);
    if (n < 2) return;
    const int cur = (int)std::lround(c.scroll / f.extra);
    const float d = 7, gap = 6, w = (float)n * d + (float)(n - 1) * gap;
    float x = f.rect.x + (f.rect.w - w) / 2;
    const float y = f.rect.y + f.rect.h - 11;
    for (int i = 0; i < n; i++, x += d + gap) {
        const ID did = hash_u32(c.id, 0xD0u + (uint32_t)i);
        if (update_control(ctx, did, {x - gap / 2, y - 4, d + gap, d + 8}, false, false).clicked)
            c.target = std::fmin((float)i * f.extra, std::fmax(0.0f, c.content_h - f.rect.w));
        s.dl.rect({x, y, d, d}, i == cur ? t.accent : t.surface_active, d / 2);
    }
}

bool accordion_begin(Context& ctx, std::string_view label, int& open, int index) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const ID id = get_id(s, label);
    layout_row(ctx, 1, nullptr, 0, 0);
    const Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if (activated(s, rs)) open = open == index ? -1 : index;
    const bool on = open == index;
    a11y_node(s, A11yRole::TreeItem, label, {}, on ? A11yNode::Expanded : A11yNode::Collapsed);
    s.dl.rect(r, control_color(s, id, rs), t.corner_radius);
    focus_ring(s, r, rs, t.corner_radius);
    const float k = anim_to(s, hash_u32(id, 2), on ? 1.0f : 0.0f, 0.18f, on ? 1.0f : 0.0f);
    draw_text_fit(s, {r.x + t.padding, r.y, r.w - 2 * t.padding - r.h, r.h}, visible_label(label), t.text,
                  false);
    const Rect ar{r.x + r.w - r.h, r.y, r.h, r.h};
    draw_icon(ctx, k > 0.5f ? Icon::ChevronUp : Icon::ChevronDown, ar, t.text_muted);
    if (k <= 0.001f) return false;
    if (s.cstack_n >= kMaxChildStack || s.layout_n >= s.cfg.max_layout_stack || s.clip_n >= kMaxClip) {
        s.error = true;
        return false;
    }
    ChildState* c = find_child(s, hash_u32(id, 1));
    if (!c) {
        s.error = true;
        return false;
    }
    if (c->content_h <= 0) s.animating = true;  // measured next frame
    c->last_frame = s.frame;
    layout_row(ctx, 1, nullptr, 0, std::fmax(1.0f, c->content_h * k));
    const Rect body = layout_next(ctx);
    ChildFrame& f = s.cstack[s.cstack_n++];
    f = ChildFrame{};
    f.slot = (int)(c - s.children.get());
    f.rect = body;
    f.pad = t.padding * 0.5f;
    f.kind = ChildAccordion;
    f.clip_base = s.clip_n;
    f.layout_index = s.layout_n;
    push_clip(s, body);
    push_layout(s, {body.x + f.pad, body.y + f.pad, body.w - 2 * f.pad, 1e6f});
    return true;
}

void accordion_end(Context& ctx) {
    State& s = state(ctx);
    if (s.cstack_n == 0 || s.cstack[s.cstack_n - 1].kind != ChildAccordion) {
        s.error = true;
        return;
    }
    const ChildFrame f = s.cstack[--s.cstack_n];
    ChildState& c = s.children[f.slot];
    const Layout& L = s.layouts[f.layout_index];
    c.content_h = L.max.y - L.body.y + 2 * f.pad;
    s.layout_n = f.layout_index;
    while (s.clip_n > f.clip_base) pop_clip(s);
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
        const bool drag = s.in.touch && s.in.pointer_down && s.touch_moved && !s.active_captures &&
                          !s.drag_used && f.rect.contains(s.press_pos) && s.press_window == s.cur_window;
        if (drag) s.drag_used = true;  // the enclosing window must not scroll too
        kinetic(s, c.scroll, c.vel, drag, maxs);
        c.scroll = std::fmin(std::fmax(c.scroll, 0.0f), maxs);
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
    s.dl.rect({0, 0, sc.x, sc.y}, {0, 0, 0, (uint8_t)(150 * anim_to(s, hash_u32(mid, 90), 1, 0.14f, 0))});
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
