// Copyright (c) 2026 Wavestorm Software
#include "internal.hpp"

namespace rd {
namespace detail {

static const Rect kHuge{-1e6f, -1e6f, 2e6f, 2e6f};

void begin_frame_windows(State& s) {
    s.hover_window = 0;
    int best = -1;
    for (int i = 0; i < s.cfg.max_windows; i++) {
        const WindowState& w = s.wins[i];
        if (!w.used || !w.open || w.last_frame + 1 != s.frame) continue;
        int z = w.overlay ? (1 << 20) : w.z;
        if (z > best && w.rect.contains(s.in.pointer)) {
            best = z;
            s.hover_window = w.id;
        }
    }
    // A modal blocks everything except itself and a popup opened from it.
    if (s.modal_id && s.hover_window != s.modal_id && s.hover_window != s.popup_id) s.hover_window = 0;
}

void end_frame_windows(State&) {}

static WindowState* find_window(State& s, ID id, bool overlay) {
    WindowState *free_slot = nullptr, *stale = nullptr;
    for (int i = 0; i < s.cfg.max_windows; i++) {
        WindowState& w = s.wins[i];
        if (w.used && w.id == id) return &w;
        if (!w.used) {
            if (!free_slot) free_slot = &w;
        } else if (!stale && w.last_frame + 2 < s.frame) {
            stale = &w;
        }
    }
    WindowState* w = free_slot ? free_slot : stale;
    if (!w) return nullptr;
    *w = WindowState{};
    w->id = id;
    w->used = true;
    w->overlay = overlay;
    w->z = overlay ? 0 : ++s.z_counter;
    return w;
}

static void unwind(State& s, const WindowFrame& f) {
    while (s.clip_n > f.clip_base) pop_clip(s);
    s.layout_n = f.layout_index;
    if (s.id_n > 0) s.id_n--;
    s.cur_window = f.prev_window;
    s.alpha = f.prev_alpha;
    s.dl.set_alpha(s.alpha);
    s.cur_layer = f.prev_layer;
    s.dl.set_layer(s.cur_layer);
    s.dl.break_command();
    s.wstack_n--;
}

bool begin_window_impl(Context& ctx, ID id, std::string_view title, Rect& rect, uint32_t flags,
                       bool overlay) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (s.wstack_n >= kMaxWinStack || s.id_n >= s.cfg.max_id_stack || s.layout_n >= s.cfg.max_layout_stack ||
        s.clip_n + 2 > kMaxClip) {
        s.error = true;
        return false;
    }
    WindowState* w = find_window(s, id, overlay);
    if (!w) {
        s.error = true;
        return false;
    }
    w->last_frame = s.frame;
    if (!w->open) return false;
    if (flags & kWindowDocked)
        w->z = 0;  // docked panels stay under every floating window
    else if (!overlay && w->z == 0)
        w->z = ++s.z_counter;  // a panel that was just undocked
    else if (!overlay && s.in.pointer_pressed && s.hover_window == id && w->z != s.z_counter)
        w->z = ++s.z_counter;

    WindowFrame& f = s.wstack[s.wstack_n++];
    f = WindowFrame{};
    f.slot = (int)(w - s.wins.get());
    f.id = id;
    f.flags = flags;
    f.overlay = overlay;
    f.prev_window = s.cur_window;
    f.prev_layer = s.cur_layer;
    f.clip_base = s.clip_n;
    f.layout_index = s.layout_n;
    f.prev_alpha = s.alpha;
    if (overlay) s.alpha = anim_to(s, hash_u32(id, 90), 1, 0.14f, 0);  // popups and dialogs fade in
    s.dl.set_alpha(s.alpha);
    s.id_stack[s.id_n++] = id;
    s.cur_window = id;
    s.cur_layer = overlay ? kOverlayLayer + 1 : (uint32_t)w->z;
    s.dl.set_layer(s.cur_layer);
    s.dl.break_command();
    push_clip(s, kHuge, true);

    const bool titlebar = flags & WindowTitleBar;
    const float th = titlebar ? t.title_height : 0;
    const float pad = (flags & WindowNoPadding) ? 0.0f : t.padding, rad = (flags & kWindowDocked) ? 0 : t.window_radius;
    Rect cb{};
    if (titlebar) {
        Rect tr{rect.x, rect.y, rect.w, th};
        if (flags & WindowClosable) {
            cb = {rect.x + rect.w - th + 5, rect.y + 5, th - 10, th - 10};
            tr.w -= th;
        }
        if (flags & WindowMovable) {
            ID tid = hash_u32(id, 1);
            update_control(ctx, tid, tr, false, true);
            if (s.active == tid && s.in.pointer_down) {
                rect.x += s.pointer_delta.x;
                rect.y += s.pointer_delta.y;
            }
        }
        if (flags & WindowClosable) {
            if (update_control(ctx, hash_u32(id, 2), cb, false, false).clicked) w->open = false;
        }
    }
    if (flags & WindowResizable) {
        ID gid = hash_u32(id, 3);
        const float g = std::fmax(16.0f, t.cell_height * 0.5f);  // finger-sized with touch themes
        update_control(ctx, gid, {rect.x + rect.w - g, rect.y + rect.h - g, g, g}, false, true);
        if (s.hover == gid || s.active == gid) s.cursor = Cursor::ResizeNWSE;
        if (s.active == gid && s.in.pointer_down) {
            rect.w = std::fmax(80.0f, rect.w + s.pointer_delta.x);
            rect.h = std::fmax(60.0f, rect.h + s.pointer_delta.y);
        }
    }
    w->rect = rect;
    if (!w->open) {
        unwind(s, f);
        return false;
    }

    const Rect r = rect;
    if ((flags & WindowBorder) && t.shadows) s.dl.shadow(r, t.shadow, rad, t.shadow_blur, {0, 8});
    if (!(flags & WindowTransparent)) s.dl.fill(r, pick(t.window_paint, t.surface), rad);
    if (titlebar) {
        s.dl.rect({r.x, r.y, r.w, th}, t.surface_hover, rad);
        s.dl.rect({r.x, r.y + th / 2, r.w, th / 2}, t.surface_hover, 0);
        s.dl.rect({r.x, r.y + th - 1, r.w, 1}, t.border);
        draw_text_centered(s, t.font_heading, {r.x + pad, r.y, r.w - 2 * pad - th, th}, visible_label(title),
                           t.text, false);
        if (flags & WindowClosable) {
            bool hov = s.hover == hash_u32(id, 2);
            Color c = hov ? t.error : t.text_muted;
            float m = 5;
            s.dl.line({cb.x + m, cb.y + m}, {cb.x + cb.w - m, cb.y + cb.h - m}, c, 1.5f);
            s.dl.line({cb.x + cb.w - m, cb.y + m}, {cb.x + m, cb.y + cb.h - m}, c, 1.5f);
        }
    }
    if (flags & WindowResizable)
        s.dl.triangle({r.x + r.w - 4, r.y + r.h - 13}, {r.x + r.w - 4, r.y + r.h - 4},
                      {r.x + r.w - 13, r.y + r.h - 4}, t.border);
    if (flags & WindowBorder) s.dl.rect(r, t.border, rad, t.border_width);

    f.content = {r.x, r.y + th, r.w, r.h - th};
    const bool scrollable = flags & WindowScrollable;
    const float max_scroll = std::fmax(0.0f, w->content_h - f.content.h);
    w->scroll = scrollable ? std::fmin(std::fmax(w->scroll, 0.0f), max_scroll) : 0;
    const float sb = (scrollable && max_scroll > 0) ? t.scrollbar_width + 4 : 0;
    Rect body{f.content.x + pad, f.content.y + pad - w->scroll, f.content.w - 2 * pad - sb,
              f.content.h - 2 * pad};
    push_clip(s, f.content, true);
    push_layout(s, body);
    if (s.cfg.a11y && !title.empty()) {
        s.last_id = id;
        s.last_rect = r;
        a11y_node(s, A11yRole::Window, title);
    }
    return true;
}

}  // namespace detail

bool begin_window(Context& ctx, std::string_view title, Rect& rect, uint32_t flags) {
    auto& s = detail::state(ctx);
    return detail::begin_window_impl(ctx, detail::get_id(s, title), title, rect, flags, false);
}

void open_window(Context& ctx, std::string_view title) {
    auto& s = detail::state(ctx);
    ID id = detail::get_id(s, title);
    for (int i = 0; i < s.cfg.max_windows; i++)
        if (s.wins[i].used && s.wins[i].id == id) s.wins[i].open = true;
}

void end_window(Context& ctx) {
    using namespace detail;
    State& s = state(ctx);
    if (s.wstack_n == 0) {
        s.error = true;
        return;
    }
    const Theme& t = s.cfg.theme;
    WindowFrame f = s.wstack[s.wstack_n - 1];
    WindowState& w = s.wins[f.slot];
    const Layout& L = s.layouts[f.layout_index];
    w.content_h = L.max.y - L.body.y + 2 * t.padding;
    // While the soft keyboard covers the bottom, a window with a focused field can scroll that much further.
    if (s.in.keyboard_inset > 0 && s.wants_text_next && s.focus_window == f.id)
        w.content_h += s.in.keyboard_inset;

    if (f.flags & WindowScrollable) {
        const Rect saved_rect = s.last_rect;
        const ID saved_id = s.last_id;
        const bool saved_hov = s.last_hovered;
        const float view = f.content.h;
        const float maxs = std::fmax(0.0f, w.content_h - view);
        s.layout_n = f.layout_index;  // widgets are done; drop nested layouts
        update_control(ctx, hash_u32(f.id, 4), f.content, false, false);
        if (s.hover_window == f.id && s.in.wheel != 0 && !s.wheel_used)
            w.scroll -= s.in.wheel * t.cell_height;
        kinetic(s, w.scroll, w.vel,
                s.in.touch && s.in.pointer_down && s.touch_moved && !s.active_captures && !s.drag_used &&
                    s.press_window == f.id,
                maxs);
        if (maxs > 0) {
            const float sbw = t.scrollbar_width;
            Rect track{f.content.x + f.content.w - sbw - 2, f.content.y + 2, sbw, f.content.h - 4};
            const float thumb_h = std::fmax(24.0f, track.h * view / w.content_h);
            const float room = std::fmax(1.0f, track.h - thumb_h);
            Rect thumb{track.x, track.y + room * (w.scroll / maxs), sbw, thumb_h};
            ID tid = hash_u32(f.id, 5);
            update_control(ctx, tid, thumb, false, true);
            if (s.active == tid && s.in.pointer_down) w.scroll += s.pointer_delta.y * (maxs / room);
            w.scroll = std::fmin(std::fmax(w.scroll, 0.0f), maxs);
            thumb.y = track.y + room * (w.scroll / maxs);
            s.dl.rect(track, t.background, sbw / 2);
            bool hot = s.hover == tid || s.active == tid;
            s.dl.rect(thumb, hot ? t.accent : t.surface_active, sbw / 2);
        } else {
            w.scroll = 0;
        }
        s.last_rect = saved_rect;
        s.last_id = saved_id;
        s.last_hovered = saved_hov;
    }
    unwind(s, f);
}

bool Context::pointer_over_ui() const {
    return s_->hover_window != 0;
}

}  // namespace rd
