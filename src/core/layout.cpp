// Copyright (c) 2026 Wavestorm Software
#include "internal.hpp"

namespace rd {
namespace detail {

Layout& top_layout(State& s) { return s.layouts[s.layout_n > 0 ? s.layout_n - 1 : 0]; }

bool push_layout(State& s, const Rect& body) {
    if (s.layout_n >= s.cfg.max_layout_stack) {
        s.error = true;
        return false;
    }
    Layout& l = s.layouts[s.layout_n++];
    l = Layout{};
    l.body = body;
    l.max = {body.x, body.y};
    l.widths[0] = -1;
    return true;
}

void extend_height(State& s, Rect& r, float needed) {
    if (needed <= r.h) return;
    Layout& l = top_layout(s);
    float extra = needed - r.h;
    l.next_row += extra;
    l.max.y += extra;
    r.h = needed;
}

}  // namespace detail

void layout_row(Context& ctx, int columns, const float* widths, int count, float height) {
    auto& s = detail::state(ctx);
    if (s.layout_n == 0) return;
    detail::Layout& l = detail::top_layout(s);
    if (columns < 1) columns = 1;
    if (columns > detail::kMaxCols) {
        columns = detail::kMaxCols;
        s.error = true;
    }
    l.items = columns;
    for (int i = 0; i < columns; i++) {
        if (!widths || count <= 0)
            l.widths[i] = (l.body.w - s.cfg.theme.spacing * (columns - 1)) / columns;
        else
            l.widths[i] = widths[i < count ? i : count - 1];
    }
    l.index = 0;
    l.pos = {0, l.next_row};
    l.row_h = height;
}

Rect layout_next(Context& ctx) {
    auto& s = detail::state(ctx);
    if (s.layout_n == 0) return {};
    detail::Layout& l = detail::top_layout(s);
    const float sp = s.cfg.theme.spacing;
    if (l.index >= l.items) {
        l.pos = {0, l.next_row};
        l.index = 0;
    }
    float w = l.widths[l.index];
    if (w <= 0) w += 1 + l.body.w - l.pos.x;  // -1 fills exactly; 0 behaves like -1
    if (w < 0) w = 0;
    float h = l.row_h;
    if (h == 0) h = s.cfg.theme.cell_height;
    if (h < 0) h = std::fmax(0.0f, l.body.h - l.pos.y + h + 1);  // -1 fills exactly
    Rect r{l.body.x + l.pos.x, l.body.y + l.pos.y, w, h};
    l.pos.x += w + sp;
    l.next_row = std::fmax(l.next_row, l.pos.y + h + sp);
    l.index++;
    l.max.x = std::fmax(l.max.x, r.x + r.w);
    l.max.y = std::fmax(l.max.y, r.y + r.h);
    return r;
}

void layout_begin_column(Context& ctx) {
    auto& s = detail::state(ctx);
    Rect a = layout_next(ctx);
    if (!detail::push_layout(s, a)) s.skipped_cols++;  // keeps begin/end balanced on overflow
}

void layout_end_column(Context& ctx) {
    auto& s = detail::state(ctx);
    if (s.skipped_cols > 0) {
        s.skipped_cols--;
        return;
    }
    if (s.layout_n < 2) return;
    detail::Layout b = s.layouts[--s.layout_n];
    detail::Layout& a = detail::top_layout(s);
    a.pos.x = std::fmax(a.pos.x, b.pos.x + b.body.x - a.body.x);
    a.next_row = std::fmax(a.next_row, b.next_row + b.body.y - a.body.y);
    a.max.x = std::fmax(a.max.x, b.max.x);
    a.max.y = std::fmax(a.max.y, b.max.y);
}

void spacing(Context& ctx, float px) {
    auto& s = detail::state(ctx);
    if (s.layout_n == 0) return;
    detail::Layout& l = detail::top_layout(s);
    l.next_row += px;
    l.index = l.items;  // next cell starts a fresh row
    l.max.y = std::fmax(l.max.y, l.body.y + l.next_row);
}

void separator(Context& ctx) {
    auto& s = detail::state(ctx);
    if (s.layout_n == 0) return;
    detail::Layout& l = detail::top_layout(s);
    float save_h = l.row_h;
    int save_items = l.items, save_idx = l.index;
    float save_w[detail::kMaxCols];
    std::memcpy(save_w, l.widths, sizeof(save_w));
    layout_row(ctx, 1, nullptr, 0, 1);
    Rect r = layout_next(ctx);
    s.dl.rect({r.x, r.y, r.w, 1}, s.cfg.theme.border);
    l.row_h = save_h;
    l.items = save_items;
    l.index = save_idx;
    std::memcpy(l.widths, save_w, sizeof(save_w));
    l.index = l.items;  // restore the "start a new row" state
}

}  // namespace rd
