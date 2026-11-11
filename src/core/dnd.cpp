// Copyright (c) 2026 Wavestorm Software
// Drag and drop: a payload copied from a source widget, delivered to a target widget on release.
#include "internal.hpp"

namespace rd {
using namespace detail;

namespace detail {

void draw_dnd_preview(State& s) {
    if (!s.dnd_active || !s.dnd_preview[0]) return;
    const Theme& t = s.cfg.theme;
    const Vec2 sz = text_size(s, t.font_body, s.dnd_preview);
    const Rect r{s.in.pointer.x + 10, s.in.pointer.y + (s.in.touch ? -t.cell_height - 24 : 10),
                 sz.x + 2 * t.padding, std::fmax(sz.y + t.padding, t.cell_height * 0.8f)};
    s.dl.set_layer(kTooltipLayer + 1);
    s.dl.break_command();
    s.dl.set_clip({-1e6f, -1e6f, 2e6f, 2e6f});
    s.dl.set_alpha(0.92f);
    if (t.shadows) s.dl.shadow(r, t.shadow, t.corner_radius, 10, {0, 4});
    s.dl.rect(r, t.surface_active, t.corner_radius);
    s.dl.rect(r, t.accent, t.corner_radius, t.border_width);
    draw_text_centered(s, t.font_body, r, s.dnd_preview, t.text, true);
    s.dl.set_alpha(1);
}

}  // namespace detail

bool drag_source(Context& ctx, const char* type, const void* data, int size, std::string_view preview) {
    State& s = state(ctx);
    const ID id = s.last_id;
    if (id == 0 || !type) return false;
    if (s.dnd_active) return s.dnd_source == id;
    if (s.active != id || !s.in.pointer_down || s.disabled > 0) return false;
    const float dx = s.in.pointer.x - s.press_pos.x, dy = s.in.pointer.y - s.press_pos.y;
    // Touch drags start with a long press, so a plain swipe still scrolls.
    const bool start = s.in.touch ? s.long_fired : dx * dx + dy * dy > kTouchSlop * kTouchSlop;
    if (!start) return false;
    if (size < 0 || size > kDndBytes || (size > 0 && !data)) {
        s.error = true;
        return false;
    }
    s.dnd_active = true;
    s.dnd_delivered = false;
    s.dnd_source = id;
    std::snprintf(s.dnd_type, sizeof s.dnd_type, "%s", type);
    if (size > 0) std::memcpy(s.dnd_data, data, (size_t)size);
    s.dnd_size = size;
    const size_t n = utf8_fit(preview, sizeof s.dnd_preview - 1);
    if (n) std::memcpy(s.dnd_preview, preview.data(), n);
    s.dnd_preview[n] = 0;
    s.touch_moved = true;  // the release must not click the source
    s.active_captures = true;
    s.drag_used = true;
    haptic(s, Haptic::Light);
    return true;
}

const void* drop_target(Context& ctx, const char* type, int* size) {
    State& s = state(ctx);
    if (!s.dnd_active || s.dnd_delivered || !type || std::strcmp(type, s.dnd_type) != 0) return nullptr;
    const Vec2 p = s.in.pointer;
    const Rect r = s.last_rect;
    const bool over = r.contains(p) && (s.clip_n == 0 || s.clips[s.clip_n - 1].contains(p)) &&
                      s.hover_window == s.cur_window && s.last_id != s.dnd_source;
    if (!over) return nullptr;
    const Theme& t = s.cfg.theme;
    s.dl.rect(r, with_alpha(t.accent, 0.12f), t.corner_radius);
    s.dl.rect(r, t.accent, t.corner_radius, 2);
    if (!s.in.pointer_released && s.in.pointer_down) return nullptr;
    s.dnd_delivered = true;
    if (size) *size = s.dnd_size;
    return s.dnd_data;
}

bool dragging(Context& ctx) {
    return state(ctx).dnd_active;
}

bool reorder_list(Context& ctx, std::string_view label, int* order, const char* const* labels, int count) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (!order || !labels || count <= 0 || s.layout_n == 0) return false;
    push_id(ctx, label);
    const ID base = top_id(s);
    char type[24];
    std::snprintf(type, sizeof type, "##ro%08x", (unsigned)base);
    const bool mine = s.dnd_active && std::strcmp(s.dnd_type, type) == 0;
    int from = -1;
    if (mine) std::memcpy(&from, s.dnd_data, sizeof from);
    float first_y = 0, step = 0;
    int insert = -1;
    for (int i = 0; i < count; i++) {
        layout_row(ctx, 1, nullptr, 0, 0);
        const Rect r = layout_next(ctx);
        if (i == 0) first_y = r.y;
        step = r.h + t.spacing;
        const ID id = hash_u32(base, (uint32_t)order[i]);
        const Response rs = update_control(ctx, id, r, true, false);
        const bool held = mine && from == i;
        const float h = animate(s, id, rs.hovered, false);
        s.dl.rect(r, held ? t.selection : with_alpha(t.surface_hover, 0.5f + 0.5f * h), t.corner_radius);
        const float g = std::fmin(r.h, 24.0f);  // grip
        for (int k = -1; k <= 1; k++)
            s.dl.rect({r.x + t.padding, r.y + r.h / 2 + (float)k * 5 - 1, g * 0.6f, 2}, t.text_muted, 1);
        const int idx = order[i];
        draw_text_fit(s, {r.x + t.padding * 2 + g * 0.6f, r.y, r.w - 3 * t.padding - g * 0.6f, r.h},
                      labels[idx], held ? t.text_muted : t.text, false);
        a11y_node(s, A11yRole::ListItem, labels[idx]);
        drag_source(ctx, type, &i, sizeof i, labels[idx]);
    }
    bool changed = false;
    if (mine && from >= 0 && from < count && step > 0) {
        insert = (int)std::floor((s.in.pointer.y - first_y) / step + 0.5f);
        insert = insert < 0 ? 0 : (insert > count ? count : insert);
        const Layout& l = top_layout(s);
        const float y = first_y + (float)insert * step - t.spacing / 2 - 1;
        s.dl.rect({l.body.x, y, l.body.w, 2}, t.accent, 1);
        if (!s.in.pointer_down && !s.dnd_delivered) {
            s.dnd_delivered = true;
            const int to = insert > from ? insert - 1 : insert;
            if (to != from) {
                const int v = order[from];
                if (to > from)
                    for (int k = from; k < to; k++) order[k] = order[k + 1];
                else
                    for (int k = from; k > to; k--) order[k] = order[k - 1];
                order[to] = v;
                changed = true;
            }
        }
    }
    pop_id(ctx);
    return changed;
}

}  // namespace rd
