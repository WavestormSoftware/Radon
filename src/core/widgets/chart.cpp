// Copyright (c) 2026 Wavestorm Software
// Charts (line, area, bar, stacked bar, scatter, pie), sparklines and resizable tables.
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace detail {

/// A "nice" step (1, 2 or 5 times a power of ten) near `raw`.
float nice_step(float raw) {
    if (!(raw > 0)) return 1;
    const float e = std::pow(10.0f, std::floor(std::log10(raw)));
    const float f = raw / e;
    return (f < 1.5f ? 1 : f < 3 ? 2 : f < 7 ? 5 : 10) * e;
}

/// Formats an axis value with as many decimals as `step` needs; thousands and millions are shortened.
void format_tick(char* out, size_t cap, float v, float step) {
    const float a = std::fabs(v);
    if (a >= 1e6f && step >= 1e5f)
        std::snprintf(out, cap, "%.*fM", step >= 1e6f ? 0 : 1, (double)(v / 1e6f));
    else if (a >= 1e3f && step >= 100)
        std::snprintf(out, cap, "%.*fk", step >= 1000 ? 0 : 1, (double)(v / 1e3f));
    else
        std::snprintf(out, cap, "%.*f", step >= 1 ? 0 : (int)std::ceil(-std::log10(step)), (double)v);
}

}  // namespace detail

namespace {

Color series_color(const Theme& t, const ChartSeries& s, int i) {
    return s.color.a ? s.color : t.chart[i % 8];
}

// A small rounded box with up to 9 lines of text near `at`, kept inside `bounds`.
void readout(State& s, Vec2 at, const Rect& bounds, const char (*lines)[64], const Color* dots, int n) {
    const Theme& t = s.cfg.theme;
    const FontHandle f = small_font(t);
    float w = 0, lh = text_size(s, f, "M").y;
    if (lh <= 0) lh = 12;
    for (int i = 0; i < n; i++) w = std::fmax(w, text_size(s, f, lines[i]).x);
    const float dot = dots ? 8.0f : 0.0f;
    Rect b{at.x + 12, at.y - 8, w + 2 * t.padding + dot + (dots ? 4 : 0), (float)n * lh + t.padding};
    if (b.x + b.w > bounds.x + bounds.w) b.x = at.x - 12 - b.w;
    b.x = std::fmax(b.x, bounds.x);
    b.y = std::fmin(std::fmax(b.y, bounds.y), bounds.y + bounds.h - b.h);
    if (t.shadows) s.dl.shadow(b, t.shadow, t.corner_radius, 8, {0, 3});
    s.dl.rect(b, t.surface_active, t.corner_radius);
    s.dl.rect(b, t.border, t.corner_radius, t.border_width);
    for (int i = 0; i < n; i++) {
        float x = b.x + t.padding;
        const float y = b.y + t.padding / 2 + (float)i * lh;
        if (dots && dots[i].a) {
            s.dl.rect({x, y + (lh - dot) / 2, dot, dot}, dots[i], dot / 2);
        }
        if (dots) x += dot + 4;
        draw_text(s, f, {x, y}, lines[i], i == 0 && dots && !dots[0].a ? t.text_muted : t.text);
    }
}

}  // namespace

Response chart(Context& ctx, std::string_view label, ChartType type, const ChartSeries* series, int count,
               const char* const* x_labels, float height) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    extend_height(s, r, height);
    r.h = std::fmax(height, r.h);
    Response rs = update_control(ctx, id, r, false, false);
    a11y_node(s, A11yRole::Image, label);
    int n = 0;
    for (int k = 0; k < count && series; k++) n = std::max(n, series[k].values ? series[k].count : 0);
    if (n < 1) return rs;
    const bool stacked = type == ChartType::StackedBar, bars = type == ChartType::Bar || stacked;
    float lo = 1e30f, hi = -1e30f;
    for (int i = 0; i < n; i++) {
        float pos = 0, neg = 0;
        for (int k = 0; k < count; k++) {
            if (i >= series[k].count || !series[k].values) continue;
            const float v = series[k].values[i];
            if (stacked) {
                (v >= 0 ? pos : neg) += v;
            } else {
                lo = std::fmin(lo, v);
                hi = std::fmax(hi, v);
            }
        }
        if (stacked) lo = std::fmin(lo, neg), hi = std::fmax(hi, pos);
    }
    if (bars || type == ChartType::Area) lo = std::fmin(lo, 0.0f), hi = std::fmax(hi, 0.0f);
    if (!(hi > lo)) hi = lo + 1;
    const float step = nice_step((hi - lo) / 4);
    lo = std::floor(lo / step) * step;
    hi = std::ceil(hi / step) * step;

    const FontHandle sf = small_font(t);
    float sh = text_size(s, sf, "M").y;
    if (sh <= 0) sh = 12;
    float top = r.y + 2;
    if (count > 1) {  // legend
        float x = r.x;
        for (int k = 0; k < count; k++) {
            const char* name = series[k].name ? series[k].name : "";
            const float w = text_size(s, sf, name).x;
            if (x + w + 14 > r.x + r.w && x > r.x) x = r.x, top += sh + 2;
            s.dl.rect({x, top + (sh - 8) / 2, 8, 8}, series_color(t, series[k], k), 4);
            draw_text(s, sf, {x + 12, top}, name, t.text_muted);
            x += w + 26;
        }
        top += sh + 6;
    }
    char buf[32];
    float ylw = 0;
    for (float v = lo; v <= hi + step * 0.5f; v += step) {
        format_tick(buf, sizeof buf, v, step);
        ylw = std::fmax(ylw, text_size(s, sf, buf).x);
    }
    ylw += 8;
    const float xlh = x_labels ? sh + 4 : 0;
    const Rect p{r.x + ylw, top + sh / 2, std::fmax(1.0f, r.w - ylw - 6),
                 std::fmax(1.0f, r.y + r.h - xlh - top - sh)};
    auto Y = [&](float v) { return p.y + p.h * (1 - (v - lo) / (hi - lo)); };
    for (float v = lo; v <= hi + step * 0.5f; v += step) {  // grid and y labels
        const float y = std::floor(Y(v)) + 0.5f;
        s.dl.rect({p.x, y, p.w, 1}, std::fabs(v) < step * 0.01f ? t.border : with_alpha(t.border, 0.6f));
        format_tick(buf, sizeof buf, v, step);
        const float w = text_size(s, sf, buf).x;
        draw_text(s, sf, {p.x - 6 - w, y - sh / 2}, buf, t.text_muted);
    }
    const float slot = p.w / (float)n;
    auto X = [&](int i) {
        return bars || n == 1 ? p.x + slot * ((float)i + 0.5f) : p.x + p.w * (float)i / (float)(n - 1);
    };
    if (x_labels) {  // as many x labels as fit
        float mw = 0;
        for (int i = 0; i < n; i++) mw = std::fmax(mw, text_size(s, sf, x_labels[i] ? x_labels[i] : "").x);
        const float gap = bars || n == 1 ? slot : p.w / (float)(n - 1);
        const int every = std::max(1, (int)std::ceil((mw + 8) / std::fmax(1.0f, gap)));
        for (int i = 0; i < n; i += every) {
            if (!x_labels[i]) continue;
            const float w = text_size(s, sf, x_labels[i]).x;
            const float x = std::fmin(std::fmax(X(i) - w / 2, r.x), r.x + r.w - w);
            draw_text(s, sf, {x, p.y + p.h + 4}, x_labels[i], t.text_muted);
        }
    }

    const Vec2 ptr = s.in.pointer;
    const bool show = (rs.hovered || (s.active == id && s.in.pointer_down)) && p.contains(ptr);
    int at = -1;
    if (show) {
        at = bars || n == 1 ? (int)((ptr.x - p.x) / slot)
                            : (int)std::lround((ptr.x - p.x) / p.w * (float)(n - 1));
        at = at < 0 ? 0 : (at >= n ? n - 1 : at);
        if (!bars)
            s.dl.rect({X(at) - 0.5f, p.y, 1, p.h}, with_alpha(t.text_muted, 0.6f));
        else
            s.dl.rect({p.x + slot * (float)at, p.y, slot, p.h}, with_alpha(t.surface_active, 0.6f));
    }
    push_clip(s, {p.x - 6, p.y - 6, p.w + 12, p.h + 12});
    const float zero = Y(std::fmin(std::fmax(0.0f, lo), hi));
    float stack_pos[64], stack_neg[64];  // shortcut: stacked bars beyond 64 points stack only the first 64
    for (int i = 0; i < 64; i++) stack_pos[i] = stack_neg[i] = 0;
    for (int k = 0; k < count; k++) {
        const ChartSeries& se = series[k];
        if (!se.values || se.count < 1) continue;
        const Color c = series_color(t, se, k);
        const int m = se.count;
        switch (type) {
            case ChartType::Area:
            case ChartType::Line:
                for (int i = 0; i + 1 < m; i++) {
                    const Vec2 a{X(i), Y(se.values[i])}, b{X(i + 1), Y(se.values[i + 1])};
                    if (type == ChartType::Area) {
                        const Color f = with_alpha(c, 0.22f);
                        s.dl.triangle(a, b, {b.x, zero}, f);
                        s.dl.triangle(a, {b.x, zero}, {a.x, zero}, f);
                    }
                    s.dl.line(a, b, c, 2);
                }
                if (m == 1 || (m <= 24 && type == ChartType::Line))
                    for (int i = 0; i < m; i++)
                        s.dl.rect({X(i) - 2.5f, Y(se.values[i]) - 2.5f, 5, 5}, c, 2.5f);
                break;
            case ChartType::Scatter:
                for (int i = 0; i < m; i++)
                    s.dl.rect({X(i) - 3.5f, Y(se.values[i]) - 3.5f, 7, 7}, with_alpha(c, 0.85f), 3.5f);
                break;
            case ChartType::Bar: {
                const float bw = slot * 0.78f / (float)count;
                for (int i = 0; i < m; i++) {
                    const float x = p.x + slot * (float)i + slot * 0.11f + bw * (float)k;
                    const float y = Y(se.values[i]);
                    const Rect b{x + 0.5f, std::fmin(y, zero), std::fmax(1.0f, bw - 1), std::fabs(zero - y)};
                    s.dl.rect(b, at == i || at < 0 ? c : with_alpha(c, 0.55f), std::fmin(3.0f, bw / 4));
                }
                break;
            }
            case ChartType::StackedBar: {
                const float bw = slot * 0.7f;
                for (int i = 0; i < m && i < 64; i++) {
                    const float v = se.values[i];
                    float& base = v >= 0 ? stack_pos[i] : stack_neg[i];
                    const float y0 = Y(base), y1 = Y(base + v);
                    base += v;
                    s.dl.rect(
                        {p.x + slot * (float)i + (slot - bw) / 2, std::fmin(y0, y1), bw, std::fabs(y1 - y0)},
                        at == i || at < 0 ? c : with_alpha(c, 0.55f));
                }
                break;
            }
        }
    }
    if (at >= 0 && !bars)
        for (int k = 0; k < count; k++)
            if (series[k].values && at < series[k].count) {
                const Vec2 q{X(at), Y(series[k].values[at])};
                s.dl.rect({q.x - 5, q.y - 5, 10, 10}, t.surface, 5);
                s.dl.rect({q.x - 4, q.y - 4, 8, 8}, series_color(t, series[k], k), 4);
            }
    pop_clip(s);
    if (at >= 0) {
        char lines[9][64];
        Color dots[9];
        int nl = 0;
        if (x_labels && x_labels[at]) {
            std::snprintf(lines[nl], 64, "%s", x_labels[at]);
            dots[nl++] = {0, 0, 0, 0};
        }
        for (int k = 0; k < count && nl < 9; k++) {
            if (!series[k].values || at >= series[k].count) continue;
            format_tick(buf, sizeof buf, series[k].values[at], step / 100);
            std::snprintf(lines[nl], 64, "%.40s%s%.20s", series[k].name ? series[k].name : "",
                          series[k].name ? ": " : "", buf);
            dots[nl++] = series_color(t, series[k], k);
        }
        readout(s, ptr, r, lines, dots, nl);
    }
    return rs;
}

Response pie_chart(Context& ctx, std::string_view label, const float* values, const char* const* labels,
                   int count, float height, bool donut) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    extend_height(s, r, height);
    r.h = std::fmax(height, r.h);
    Response rs = update_control(ctx, id, r, false, false);
    a11y_node(s, A11yRole::Image, label);
    float total = 0;
    for (int i = 0; i < count && values; i++) total += std::fmax(0.0f, values[i]);
    if (total <= 0) return rs;
    const float d = std::fmin(r.h - 8, r.w * 0.55f), R = d / 2, inner = donut ? R * 0.58f : 0;
    const Vec2 c{r.x + 4 + R, r.y + r.h / 2};
    const Vec2 ptr = s.in.pointer;
    const float dx = ptr.x - c.x, dy = ptr.y - c.y, dist = std::sqrt(dx * dx + dy * dy);
    int hot = -1;
    const bool show = rs.hovered || (s.active == id && s.in.pointer_down);
    float ang = std::atan2(dy, dx) + 1.570796f;  // 0 at 12 o'clock, clockwise
    if (ang < 0) ang += 6.283185f;
    float a0 = 0;
    for (int i = 0; i < count; i++) {
        const float a1 = a0 + std::fmax(0.0f, values[i]) / total * 6.283185f;
        if (show && dist <= R + 4 && dist >= inner && ang >= a0 && ang < a1) hot = i;
        a0 = a1;
    }
    a0 = 0;
    for (int i = 0; i < count; i++) {
        const float a1 = a0 + std::fmax(0.0f, values[i]) / total * 6.283185f;
        const Color col = t.chart[i % 8];
        const float grow = anim_to(s, hash_u32(id, (uint32_t)i), i == hot ? 1.0f : 0.0f, 0.12f, 0) * 5;
        const float mid = (a0 + a1) / 2 - 1.570796f;
        const Vec2 o{c.x + std::cos(mid) * grow, c.y + std::sin(mid) * grow};
        const int steps = std::max(2, (int)((a1 - a0) / 6.283185f * 96));
        for (int k = 0; k < steps; k++) {
            const float b0 = a0 + (a1 - a0) * (float)k / (float)steps - 1.570796f;
            const float b1 = a0 + (a1 - a0) * (float)(k + 1) / (float)steps - 1.570796f + 0.004f;
            const Vec2 p0{o.x + std::cos(b0) * R, o.y + std::sin(b0) * R},
                p1{o.x + std::cos(b1) * R, o.y + std::sin(b1) * R};
            if (donut) {
                const Vec2 q0{o.x + std::cos(b0) * inner, o.y + std::sin(b0) * inner},
                    q1{o.x + std::cos(b1) * inner, o.y + std::sin(b1) * inner};
                s.dl.triangle(q0, p0, p1, col);
                s.dl.triangle(q0, p1, q1, col);
            } else {
                s.dl.triangle(o, p0, p1, col);
            }
        }
        a0 = a1;
    }
    char buf[64];
    if (donut) {  // the hovered share, or the total, in the hole
        if (hot >= 0)
            std::snprintf(buf, sizeof buf, "%.0f%%", (double)(values[hot] / total * 100));
        else
            format_tick(buf, sizeof buf, total, nice_step(total / 100));
        const FontHandle f = t.font_large >= 0 ? t.font_large : t.font_heading;
        draw_text_centered(s, f, {c.x - inner, c.y - inner, inner * 2, inner * 2}, buf, t.text, true);
    }
    const FontHandle sf = small_font(t);
    float lh = text_size(s, sf, "M").y + 6;
    if (lh <= 6) lh = 18;
    const float lx = c.x + R + 16;
    float ly = r.y + std::fmax(0.0f, (r.h - lh * (float)count) / 2);
    for (int i = 0; i < count && labels; i++) {  // legend
        if (ly + lh > r.y + r.h + 1) break;
        s.dl.rect({lx, ly + (lh - 10) / 2, 10, 10}, t.chart[i % 8], 3);
        std::snprintf(buf, sizeof buf, "%s  %.0f%%", labels[i] ? labels[i] : "",
                      (double)(std::fmax(0.0f, values[i]) / total * 100));
        if (lx + 16 < r.x + r.w) {
            push_clip(s, {lx, ly, r.x + r.w - lx, lh});
            draw_text(s, sf, {lx + 16, ly + 3}, buf, i == hot ? t.text : t.text_muted);
            pop_clip(s);
        }
        ly += lh;
    }
    if (hot >= 0 && !donut) {
        char lines[1][64];
        format_tick(buf, sizeof buf, values[hot], nice_step(total / 100));
        std::snprintf(lines[0], 64, "%.40s: %.20s", labels && labels[hot] ? labels[hot] : "", buf);
        const Color dot = t.chart[hot % 8];
        readout(s, ptr, r, lines, &dot, 1);
    }
    return rs;
}

void sparkline(Context& ctx, const float* v, int n, Color color) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const Rect r0 = layout_next(ctx);
    if (!v || n < 2) return;
    const Rect r{r0.x + 2, r0.y + r0.h * 0.2f, r0.w - 4, r0.h * 0.6f};
    float lo = v[0], hi = v[0];
    for (int i = 1; i < n; i++) lo = std::fmin(lo, v[i]), hi = std::fmax(hi, v[i]);
    if (hi == lo) hi = lo + 1;
    const Color c = color.a ? color : t.accent;
    auto P = [&](int i) {
        return Vec2{r.x + r.w * (float)i / (float)(n - 1), r.y + r.h * (1 - (v[i] - lo) / (hi - lo))};
    };
    const float base = r.y + r.h;
    for (int i = 0; i + 1 < n; i++) {
        const Vec2 a = P(i), b = P(i + 1);
        s.dl.triangle(a, b, {b.x, base}, with_alpha(c, 0.15f));
        s.dl.triangle(a, {b.x, base}, {a.x, base}, with_alpha(c, 0.15f));
        s.dl.line(a, b, c, 1.5f);
    }
    const Vec2 e = P(n - 1);
    s.dl.rect({e.x - 2.5f, e.y - 2.5f, 5, 5}, c, 2.5f);
}

// ---------------------------------------------------------------------------------------------
// Tables
// ---------------------------------------------------------------------------------------------

namespace {

// Concrete column widths for a row `total` px wide: fixed columns keep their width, the rest share.
void column_widths(const TableColumn* cols, int n, float total, float sp, float* out) {
    float fixed = sp * (float)(n - 1);
    int fill = 0;
    for (int i = 0; i < n; i++) {
        if (cols[i].width > 0)
            fixed += cols[i].width;
        else
            fill++;
    }
    const float each = fill ? std::fmax(0.0f, total - fixed) / (float)fill : 0;
    for (int i = 0; i < n; i++)
        out[i] = cols[i].width > 0 ? cols[i].width : std::fmax(cols[i].min_width, each);
}

}  // namespace

bool table_begin(Context& ctx, std::string_view id, TableColumn* cols, int count, float height,
                 TableSort* sort) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (!cols || count < 1 || count > kMaxCols || s.layout_n == 0) {
        s.error = true;
        return false;
    }
    if (sort) sort->changed = false;
    push_id(ctx, id);
    const ID base = top_id(s);
    layout_row(ctx, 1, nullptr, 0, 0);
    const Rect hr = layout_next(ctx);
    s.dl.rect(hr, t.surface_hover, t.corner_radius);
    float w[kMaxCols];
    column_widths(cols, count, hr.w, t.spacing, w);
    const FontHandle sf = small_font(t);
    float x = hr.x;
    for (int i = 0; i < count; i++) {
        const Rect c{x, hr.y, w[i], hr.h};
        if (i + 1 < count && cols[i].width > 0) {  // resize handle on the right edge
            const ID gid = hash_u32(base, 0x5E00u + (uint32_t)i);
            const Rect g{c.x + c.w + t.spacing / 2 - 4, hr.y, 8, hr.h};
            const Response gr = update_control(ctx, gid, g, false, true);
            const bool drag = s.active == gid && s.in.pointer_down;
            if (gr.hovered || drag) s.cursor = Cursor::ResizeEW;
            if (drag) cols[i].width = std::fmax(cols[i].min_width, cols[i].width + s.pointer_delta.x);
            s.dl.rect({g.x + 3, hr.y + 5, 2, hr.h - 10},
                      lerp(t.border, t.accent, animate(s, gid, gr.hovered || drag, false)), 1);
        }
        const bool sorted = sort && sort->column == i;
        if (sort) {
            const Response rs = update_control(ctx, hash_u32(base, (uint32_t)i), c, true, false);
            if (activated(s, rs)) {
                sort->ascending = sorted ? !sort->ascending : true;
                sort->column = i;
                sort->changed = true;
            }
            a11y_node(s, A11yRole::Button, cols[i].name ? cols[i].name : "");
            focus_ring(s, c, rs, t.corner_radius);
        }
        const char* name = cols[i].name ? cols[i].name : "";
        const Rect tr{c.x + t.padding * 0.5f, c.y, c.w - t.padding * 0.5f, c.h};
        push_clip(s, tr);
        draw_text_centered(s, sf, tr, name, sorted ? t.text : t.text_muted, false);
        const float tw = text_size(s, sf, name).x;
        if (sorted)
            draw_icon(ctx, sort->ascending ? Icon::ChevronUp : Icon::ChevronDown,
                      {tr.x + tw, c.y, c.h * 0.7f, c.h}, t.accent);
        pop_clip(s);
        x += w[i] + t.spacing;
    }
    if (!begin_child(ctx, "##body", height, false)) {
        pop_id(ctx);
        return false;
    }
    column_widths(cols, count, top_layout(s).body.w, t.spacing, w);
    layout_row(ctx, count, w, count, 0);
    a11y_static(s, A11yRole::Table, hr, id);
    return true;
}

void table_end(Context& ctx) {
    end_child(ctx);
    pop_id(ctx);
}

Response table_tree_row(Context& ctx, int index, std::string_view label, int depth, bool* open,
                        bool selected) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (s.layout_n == 0) return {};
    const Layout& l = top_layout(s);
    const float rh = l.row_h > 0 ? l.row_h : t.cell_height;
    const float ind = (float)(depth < 0 ? 0 : depth) * t.indent;
    const Rect arrow{l.body.x + ind, l.body.y + l.next_row, rh, rh};
    const ID aid = hash_u32(get_id(s, "##tree"), (uint32_t)index);
    Response ar;
    if (open) ar = update_control(ctx, aid, arrow, false, false);  // before the row, so it gets the press
    Response rs = table_row(ctx, index, selected);
    if (open) {
        if (ar.clicked) *open = !*open;
        if (rs.focused && key_pressed(s, KeyRight)) *open = true;
        if (rs.focused && key_pressed(s, KeyLeft)) *open = false;
        draw_icon(ctx, *open ? Icon::ChevronDown : Icon::ChevronRight, arrow,
                  ar.hovered ? t.text : t.text_muted);
    }
    a11y_node(s, A11yRole::TreeItem, label, {},
              (open ? (*open ? A11yNode::Expanded : A11yNode::Collapsed) : 0u) |
                  (selected ? A11yNode::Selected : 0u));
    const Rect c = layout_next(ctx);
    const float x = arrow.x + rh;
    draw_text_fit(s, {x, c.y, c.x + c.w - x, c.h}, visible_label(label), t.text, false);
    return rs;
}

}  // namespace rd
