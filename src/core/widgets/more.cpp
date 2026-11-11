// Copyright (c) 2026 Wavestorm Software
// Range slider, segmented control, chips, rating, calendar, pagination, navigation, tables and
// small display widgets.
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace detail {
bool activated(State& s, const Response& rs) {
    return rs.clicked || (rs.focused && (key_pressed(s, KeyEnter) || key_pressed(s, KeySpace)));
}

void focus_ring(State& s, const Rect& r, const Response& rs, float radius) {
    const Theme& t = s.cfg.theme;
    if (rs.focused && s.kbd_nav) s.dl.rect(r, t.focus_ring, radius, t.focus_ring_width);
}
}  // namespace detail

namespace {

void fill_star(DrawList& d, Vec2 c, float radius, Color col) {
    Vec2 p[10];
    for (int i = 0; i < 10; i++) {
        const float a = -1.570796f + (float)i * 0.628318f, rad = (i % 2) ? radius * 0.42f : radius;
        p[i] = {c.x + std::cos(a) * rad, c.y + std::sin(a) * rad};
    }
    for (int i = 0; i < 10; i++) d.triangle(c, p[i], p[(i + 1) % 10], col);
}

int days_in(int y, int m) {
    static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) ? 29 : d[m - 1];
}

int weekday(int y, int m, int d) {  // 0 = Monday (Sakamoto's method, shifted from Sunday-first)
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y--;
    return ((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7 + 6) % 7;
}

bool calendar_impl(Context& ctx, std::string_view label, int& year, int& month, int& day, bool* picked) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    static const char* const months[] = {"January",   "February", "March",    "April",
                                         "May",       "June",     "July",     "August",
                                         "September", "October",  "November", "December"};
    static const char* const wd[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    month = month < 1 ? 1 : (month > 12 ? 12 : month);
    const int y0 = year, m0 = month, d0 = day;
    push_id(ctx, label);
    const float ch = t.cell_height;
    const float nav[] = {ch, -(ch + t.spacing + 1), ch};
    layout_row(ctx, 3, nav, 3, 0);
    if (icon_button(ctx, "prev", Icon::ChevronLeft).clicked && --month < 1) month = 12, year--;
    char title[32];
    std::snprintf(title, sizeof title, "%s %d", months[month - 1], year);
    draw_text_fit(s, layout_next(ctx), title, t.text, true);
    if (icon_button(ctx, "next", Icon::ChevronRight).clicked && ++month > 12) month = 1, year++;
    if (month != m0) std::snprintf(title, sizeof title, "%s %d", months[month - 1], year);

    layout_row(ctx, 7, nullptr, 0, ch * 0.7f);
    for (const char* w : wd) draw_text_centered(s, small_font(t), layout_next(ctx), w, t.text_muted, true);
    layout_row(ctx, 7, nullptr, 0, 0);
    const int n = days_in(year, month);
    if (day > n) day = n;
    if (day < 1) day = 1;
    for (int i = weekday(year, month, 1); i > 0; i--) layout_next(ctx);
    for (int d = 1; d <= n; d++) {
        const Rect c = layout_next(ctx);
        const ID id = hash_u32(top_id(s), (uint32_t)d);
        const Response rs = update_control(ctx, id, c, true, false);
        if (activated(s, rs)) {
            day = d;
            if (picked) *picked = true;
        }
        const bool on = d == day;
        const float side = std::fmin(c.w, c.h);
        const Rect b{c.x + (c.w - side) / 2, c.y + (c.h - side) / 2, side, side};
        s.dl.rect(b, on ? t.accent : with_alpha(t.surface_active, animate(s, id, rs.hovered, false)),
                  side / 2);
        focus_ring(s, b, rs, side / 2);
        char num[12];
        std::snprintf(num, sizeof num, "%d", d);
        draw_text_centered(s, t.font_body, c, num, on ? t.accent_text : t.text, true);
    }
    pop_id(ctx);
    spacing(ctx, 0);  // whatever follows starts on a fresh row, not in the last week
    return year != y0 || month != m0 || day != d0;
}

}  // namespace

Response range_slider(Context& ctx, std::string_view label, float& low, float& high, float mn, float mx) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, true);
    const float kr = r.h * 0.3f;  // thumb radius
    const float x0 = r.x + kr, w = std::fmax(1.0f, r.w - 2 * kr);
    if (mx > mn) {
        float nl = low, nh = high;
        const float pv = mn + clamp01((s.in.pointer.x - x0) / w) * (mx - mn);
        if (rs.pressed) s.drag_acc = pv < (low + high) / 2 ? 0.0f : 1.0f;  // grab the nearer thumb
        if (s.active == id && s.in.pointer_down) {
            if (s.drag_acc == 0.0f)
                nl = std::fmin(pv, high);
            else
                nh = std::fmax(pv, low);
        }
        if (rs.focused) {
            const float step = (mx - mn) * 0.05f;
            for (int i = 0; i < s.in.keys.count; i++) {
                const KeyEvent& k = s.in.keys.items[i];
                if (k.key != KeyLeft && k.key != KeyRight) continue;
                (k.shift ? nl : nh) += k.key == KeyLeft ? -step : step;
            }
        }
        nh = std::fmin(std::fmax(nh, mn), mx);
        nl = std::fmin(std::fmax(nl, mn), nh);
        rs.changed = nl != low || nh != high;
        low = nl;
        high = nh;
    }
    const float fl = mx > mn ? clamp01((low - mn) / (mx - mn)) : 0,
                fh = mx > mn ? clamp01((high - mn) / (mx - mn)) : 0;
    const float cy = r.y + r.h / 2;
    s.dl.rect({r.x, cy - 2, r.w, 4}, t.surface_active, 2);
    s.dl.rect({x0 + w * fl, cy - 2, w * (fh - fl), 4}, t.accent, 2);
    for (float f : {fl, fh}) {
        const Rect k{x0 + w * f - kr, cy - kr, 2 * kr, 2 * kr};
        s.dl.rect(k, {255, 255, 255, 255}, kr);
        s.dl.rect(k, t.accent, kr, 2);
    }
    focus_ring(s, r, rs, t.corner_radius);
    return rs;
}

bool segmented(Context& ctx, std::string_view label, int& selected, const char* const* items, int count) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID base = get_id(s, label);
    Rect r = layout_next(ctx);
    if (count < 1 || !items) return false;
    s.dl.rect(r, t.background, t.corner_radius);
    s.dl.rect(r, t.border, t.corner_radius, t.border_width);
    const float sw = r.w / (float)count;
    const float sel = (float)(selected < 0 ? 0 : (selected >= count ? count - 1 : selected));
    const float pos = anim_to(s, hash_u32(base, 1000), sel, 0.18f, sel);
    const Rect thumb{r.x + sw * pos + 2, r.y + 2, sw - 4, r.h - 4};
    s.dl.rect(thumb, t.surface_active, std::fmax(0.0f, t.corner_radius - 2));
    bool changed = false;
    for (int i = 0; i < count; i++) {
        const Rect tr{r.x + sw * (float)i, r.y, sw, r.h};
        const ID id = hash_u32(base, (uint32_t)i);
        const Response rs = update_control(ctx, id, tr, true, false);
        if (activated(s, rs)) {
            if (selected != i) changed = true, haptic(s, Haptic::Selection);
            selected = i;
        }
        a11y_node(s, A11yRole::Tab, items[i], {}, selected == i ? A11yNode::Selected : 0u);
        const float h = animate(s, id, rs.hovered, false);
        draw_text_fit(s, tr, items[i], selected == i ? t.text : lerp(t.text_muted, t.text, h), true);
        focus_ring(s, tr, rs, t.corner_radius);
    }
    return changed;
}

Response chip(Context& ctx, std::string_view label, bool selected) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    const std::string_view text = visible_label(label);
    const float mark0 = selected ? t.cell_height * 0.82f * 0.7f : 0;
    Rect r = layout_next_w(ctx, text_size(s, t.font_body, text).x + 2 * t.padding + mark0);
    const float h = r.h * 0.82f, mark = selected ? h * 0.7f : 0;
    const float w = std::fmin(r.w, text_size(s, t.font_body, text).x + 2 * t.padding + mark);
    const Rect pill{r.x, r.y + (r.h - h) / 2, w, h};
    Response rs = update_control(ctx, id, pill, true, false);
    if (rs.focused && (key_pressed(s, KeyEnter) || key_pressed(s, KeySpace))) rs.clicked = true;
    if (rs.clicked) haptic(s, Haptic::Selection);
    a11y_node(s, A11yRole::Button, label, {}, selected ? A11yNode::Selected : 0u);
    s.dl.rect(pill, selected ? t.selection : control_color(s, id, rs), h / 2);
    s.dl.rect(pill, selected ? t.accent : t.border, h / 2, t.border_width);
    focus_ring(s, pill, rs, h / 2);
    if (selected) draw_icon(ctx, Icon::Check, {pill.x + t.padding * 0.5f, pill.y, h, h}, t.accent);
    draw_text_fit(s, {pill.x + t.padding + mark, pill.y, w - 2 * t.padding - mark, h}, text, t.text, false);
    return rs;
}

Response rating(Context& ctx, std::string_view label, int& value, int max) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    if (max < 1) max = 1;
    const float side = std::fmin(r.h, r.w / (float)max);
    const Rect area{r.x, r.y, side * (float)max, r.h};
    Response rs = update_control(ctx, id, area, true, false);
    const int at = 1 + (int)((s.in.pointer.x - r.x) / side);  // star under the pointer
    int nv = value;
    if (rs.clicked && at >= 1 && at <= max) nv = value == at ? 0 : at;
    if (rs.focused) {
        if (key_pressed(s, KeyLeft)) nv--;
        if (key_pressed(s, KeyRight)) nv++;
    }
    nv = nv < 0 ? 0 : (nv > max ? max : nv);
    rs.changed = nv != value;
    value = nv;
    const int preview = rs.hovered ? at : value;
    for (int i = 0; i < max; i++) {
        const Rect c{r.x + side * (float)i, r.y + (r.h - side) / 2, side, side};
        if (i < value)
            fill_star(s.dl, {c.x + side / 2, c.y + side / 2}, side * 0.36f, t.warning);
        else
            draw_icon(ctx, Icon::Star, c, i < preview ? t.warning : t.text_muted);
    }
    focus_ring(s, area, rs, t.corner_radius);
    return rs;
}

Response link(Context& ctx, std::string_view text) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, text);
    text = visible_label(text);
    const Vec2 sz = text_size(s, t.font_body, text);
    Rect r = layout_next_w(ctx, sz.x);
    const Rect lr{r.x, r.y, std::fmin(sz.x, r.w), r.h};
    Response rs = update_control(ctx, id, lr, true, false);
    a11y_node(s, A11yRole::Link, text);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    if (rs.hovered) s.cursor = Cursor::Hand;
    draw_text_fit(s, lr, text, t.accent, false);
    if (rs.hovered || (rs.focused && s.kbd_nav))
        s.dl.rect({lr.x, r.y + (r.h + sz.y) / 2 - 1, lr.w, 1}, t.accent);
    return rs;
}

Response splitter(Context& ctx, std::string_view label, float& size, float mn, float mx) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, false, true);
    const bool drag = s.active == id && s.in.pointer_down;
    if (rs.hovered || drag) s.cursor = Cursor::ResizeEW;
    if (drag && s.pointer_delta.x != 0) {
        const float nv = std::fmin(std::fmax(size + s.pointer_delta.x, mn), mx);
        rs.changed = nv != size;
        size = nv;
    }
    const float h = animate(s, id, rs.hovered || drag, false);
    s.dl.rect({r.x + r.w / 2 - 1, r.y, 2, r.h}, lerp(t.border, t.accent, h), 1);
    return rs;
}

bool calendar(Context& ctx, std::string_view label, int& year, int& month, int& day) {
    return calendar_impl(ctx, label, year, month, day, nullptr);
}

bool date_picker(Context& ctx, std::string_view label, int& year, int& month, int& day) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, false);
    if ((rs.clicked && s.popup_suppress != id) || (rs.focused && key_pressed(s, KeyEnter)))
        open_popup_id(s, id);
    draw_frame(s, r, control_color(s, id, rs), rs.focused);
    char buf[24];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d", year, month, day);
    draw_text_fit(s, {r.x + t.padding, r.y, r.w - 2 * t.padding - r.h, r.h}, buf, t.text, false);
    draw_icon(ctx, Icon::Calendar, {r.x + r.w - r.h, r.y, r.h, r.h}, t.text_muted);
    bool changed = false;
    if (popup_begin_id(ctx, id, 7 * t.cell_height + 6 * t.spacing + 2 * t.padding)) {
        bool picked = false;
        changed = calendar_impl(ctx, "##cal", year, month, day, &picked);
        if (picked) s.popup_id = 0;
        popup_end(ctx);
    }
    return changed;
}

// Page buttons hash their page index; these salts are out of reach of any int page count.
constexpr uint32_t kPrevSalt = 0xFFFFFFF0u, kNextSalt = 0xFFFFFFF1u;

bool pagination(Context& ctx, std::string_view label, int& page, int n) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID base = get_id(s, label);
    Rect r = layout_next(ctx);
    if (n < 1) return false;
    const int old = page;
    page = page < 0 ? 0 : (page >= n ? n - 1 : page);
    int items[7], k = 0;  // page indices, -1 = ellipsis
    if (n <= 7) {
        for (int i = 0; i < n; i++) items[k++] = i;
    } else {
        int lo = page - 1, hi = page + 1;
        if (page <= 2) lo = 1, hi = 3;
        if (page >= n - 3) lo = n - 4, hi = n - 2;
        items[k++] = 0;
        if (lo > 1) items[k++] = -1;
        for (int i = lo; i <= hi; i++) items[k++] = i;
        if (hi < n - 2) items[k++] = -1;
        items[k++] = n - 1;
    }
    const float side = std::fmin(r.h, (r.w - 4 * (float)(k + 1)) / (float)(k + 2));
    float x = r.x;
    auto cell = [&](uint32_t salt, Rect& c) {
        c = {x, r.y + (r.h - side) / 2, side, side};
        x += side + 4;
        return update_control(ctx, hash_u32(base, salt), c, true, false);
    };
    Rect c;
    int np = page;
    Response rs = cell(kPrevSalt, c);
    if (activated(s, rs) && page > 0) np = page - 1;
    draw_frame(s, c, control_color(s, hash_u32(base, kPrevSalt), rs), rs.focused);
    draw_icon(ctx, Icon::ChevronLeft, c, page > 0 ? t.text : t.text_muted);
    for (int i = 0; i < k; i++) {
        if (items[i] < 0) {
            c = {x, r.y, side, r.h};
            x += side + 4;
            draw_text_centered(s, t.font_body, c, "...", t.text_muted, true);
            continue;
        }
        rs = cell((uint32_t)items[i], c);
        if (activated(s, rs)) np = items[i];
        const bool on = items[i] == page;
        draw_frame(s, c, on ? t.accent : control_color(s, hash_u32(base, (uint32_t)items[i]), rs),
                   rs.focused);
        char num[12];
        std::snprintf(num, sizeof num, "%d", items[i] + 1);
        draw_text_fit(s, c, num, on ? t.accent_text : t.text, true);
    }
    rs = cell(kNextSalt, c);
    if (activated(s, rs) && page < n - 1) np = page + 1;
    draw_frame(s, c, control_color(s, hash_u32(base, kNextSalt), rs), rs.focused);
    draw_icon(ctx, Icon::ChevronRight, c, page < n - 1 ? t.text : t.text_muted);
    page = np;
    return page != old;
}

bool nav_bar(Context& ctx, std::string_view label, int& selected, const Icon* icons,
             const char* const* labels, int count) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID base = get_id(s, label);
    Rect r = layout_next(ctx);
    if (count < 1) return false;
    const float sw = r.w / (float)count, ih = r.h * 0.42f, top = r.y + r.h * 0.08f;
    const float sel = (float)(selected < 0 ? 0 : (selected >= count ? count - 1 : selected));
    const float pos = anim_to(s, hash_u32(base, 1000), sel, 0.2f, sel);
    const float pw = std::fmin(sw * 0.7f, ih * 2.4f);
    s.dl.rect({r.x + sw * pos + (sw - pw) / 2, top, pw, ih}, t.selection, ih / 2);
    bool changed = false;
    for (int i = 0; i < count; i++) {
        const Rect tr{r.x + sw * (float)i, r.y, sw, r.h};
        const ID id = hash_u32(base, (uint32_t)i);
        const Response rs = update_control(ctx, id, tr, true, false);
        if (activated(s, rs)) {
            if (selected != i) changed = true, haptic(s, Haptic::Selection);
            selected = i;
        }
        a11y_node(s, A11yRole::Tab, labels ? labels[i] : "", {}, selected == i ? A11yNode::Selected : 0u);
        const Color c =
            selected == i ? t.accent : lerp(t.text_muted, t.text, animate(s, id, rs.hovered, false));
        if (icons) draw_icon(ctx, icons[i], {tr.x + (sw - ih) / 2, top, ih, ih}, c);
        if (labels)
            draw_text_centered(s, small_font(t), {tr.x, top + ih, sw, r.y + r.h - top - ih}, labels[i], c,
                               true);
        focus_ring(s, tr, rs, t.corner_radius);
    }
    return changed;
}

int breadcrumbs(Context& ctx, std::string_view label, const char* const* items, int count) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID base = get_id(s, label);
    Rect r = layout_next(ctx);
    int clicked = -1;
    float x = r.x;
    push_clip(s, r);
    for (int i = 0; i < count; i++) {
        const Vec2 sz = text_size(s, t.font_body, items[i]);
        const Rect tr{x, r.y, sz.x, r.h};
        x += sz.x;
        if (i == count - 1) {
            draw_text_centered(s, t.font_body, tr, items[i], t.text, false);
            break;
        }
        const ID id = hash_u32(base, (uint32_t)i);
        const Response rs = update_control(ctx, id, tr, true, false);
        if (activated(s, rs)) clicked = i;
        if (rs.hovered) s.cursor = Cursor::Hand;
        draw_text_centered(s, t.font_body, tr, items[i],
                           lerp(t.text_muted, t.text, animate(s, id, rs.hovered, false)), false);
        focus_ring(s, tr, rs, t.corner_radius);
        const float cw = r.h * 0.7f;
        draw_icon(ctx, Icon::ChevronRight, {x, r.y, cw, r.h}, t.text_muted);
        x += cw;
    }
    pop_clip(s);
    return clicked;
}

void heading(Context& ctx, std::string_view text) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    const FontHandle f = t.font_large >= 0 ? t.font_large : t.font_heading;
    text = visible_label(text);
    extend_height(s, r, text_size(s, f, text).y);
    draw_text_centered(s, f, r, text, t.text, false);
    a11y_static(s, A11yRole::Heading, r, text);
}

void avatar(Context& ctx, std::string_view name, Color color) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    const float d = std::fmin(r.w, r.h);
    const Rect c{r.x, r.y + (r.h - d) / 2, d, d};
    s.dl.rect(c, color, d / 2);
    char in[3] = {0, 0, 0};
    int n = 0;
    for (size_t i = 0; i < name.size() && n < 2; i++) {
        const unsigned char ch = (unsigned char)name[i];
        if (ch < 0x80 && ch != ' ' && (i == 0 || name[i - 1] == ' '))  // ASCII initials only
            in[n++] = (char)((ch >= 'a' && ch <= 'z') ? ch - 32 : ch);
    }
    const bool dark = color.r * 299 + color.g * 587 + color.b * 114 < 150000;
    draw_text_centered(s, t.font_body, c, {in, (size_t)n},
                       dark ? Color{255, 255, 255, 255} : Color{20, 20, 20, 255}, true);
}

void alert(Context& ctx, Tone tone, std::string_view text) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    const Color c = tone_color(t, tone);
    const float is = t.cell_height * 0.8f, pad = t.padding;
    TextLine lines[16];
    text = visible_label(text);
    const int n = wrap_text(ctx, t.font_body, text, r.w - is - 2 * pad, lines, 16);
    const float lh = measure_text(ctx, t.font_body, "M").y;
    extend_height(s, r, std::fmax(r.h, (float)n * lh + pad));
    s.dl.rect(r, with_alpha(c, 0.14f), t.corner_radius);
    s.dl.rect(r, with_alpha(c, 0.6f), t.corner_radius, t.border_width);
    static const Icon icons[] = {Icon::Info, Icon::Check, Icon::Warning, Icon::Close};
    draw_icon(ctx, icons[(int)tone], {r.x + pad * 0.5f, r.y + (r.h - is) / 2, is, is}, c);
    const float y = r.y + (r.h - (float)n * lh) / 2;
    a11y_static(s, A11yRole::Text, r, text);
    for (int i = 0; i < n; i++)
        draw_text(s, t.font_body, {r.x + is + pad, y + (float)i * lh},
                  text.substr(lines[i].start, lines[i].length), t.text);
}

void progress_ring(Context& ctx, float fraction) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    const float d = std::fmin(r.w, r.h) * 0.85f, th = std::fmax(2.0f, d * 0.12f), rad = d / 2 - th / 2;
    const Vec2 c{r.x + r.h / 2, r.y + r.h / 2};
    s.dl.rect({c.x - d / 2, c.y - d / 2, d, d}, t.surface_active, d / 2, th);
    const int seg = (int)std::ceil(48.0f * clamp01(fraction));
    const float end = clamp01(fraction) * 6.283185f;
    for (int i = 0; i < seg; i++) {
        const float a0 = -1.570796f + std::fmin(end, 6.283185f * (float)i / 48),
                    a1 = -1.570796f + std::fmin(end, 6.283185f * (float)(i + 1) / 48 + 0.02f);
        s.dl.line({c.x + std::cos(a0) * rad, c.y + std::sin(a0) * rad},
                  {c.x + std::cos(a1) * rad, c.y + std::sin(a1) * rad}, t.accent, th);
    }
}

void skeleton(Context& ctx, float width) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Rect r = layout_next(ctx);
    const float pulse = s.cfg.animations ? 0.5f + 0.5f * std::sin(s.time * 3.5f) : 0.5f;
    s.dl.rect({r.x, r.y + r.h * 0.25f, r.w * clamp01(width), r.h * 0.5f},
              lerp(t.surface_hover, t.surface_active, pulse), t.corner_radius);
    if (s.cfg.animations) s.animating = true;
}

bool table_header(Context& ctx, std::string_view label, const char* const* columns, int count,
                  const float* widths, int* sort_column, bool* ascending) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (s.layout_n == 0 || count < 1) return false;
    ID base = get_id(s, label);
    layout_row(ctx, count, widths, widths ? count : 0, 0);
    const Layout& l = top_layout(s);
    s.dl.rect({l.body.x, l.body.y + l.next_row, l.body.w, t.cell_height}, t.surface_hover, t.corner_radius);
    bool changed = false;
    for (int i = 0; i < count; i++) {
        const Rect c = layout_next(ctx);
        const bool sorted = sort_column && *sort_column == i;
        if (sort_column) {
            const Response rs = update_control(ctx, hash_u32(base, (uint32_t)i), c, true, false);
            if (activated(s, rs)) {
                if (ascending) *ascending = sorted ? !*ascending : true;
                *sort_column = i;
                changed = true;
            }
            focus_ring(s, c, rs, t.corner_radius);
        }
        const Vec2 sz = text_size(s, small_font(t), columns[i]);
        const Rect tr{c.x + t.padding * 0.5f, c.y, c.w - t.padding * 0.5f, c.h};
        draw_text_centered(s, small_font(t), tr, columns[i], sorted ? t.text : t.text_muted, false);
        if (sorted && tr.x + sz.x + c.h * 0.7f <= c.x + c.w)
            draw_icon(ctx, (ascending && !*ascending) ? Icon::ChevronDown : Icon::ChevronUp,
                      {tr.x + sz.x, c.y, c.h * 0.7f, c.h}, t.accent);
    }
    return changed;
}

Response table_row(Context& ctx, int index, bool selected) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    if (s.layout_n == 0) return {};
    Layout& l = top_layout(s);
    l.index = l.items;  // start a fresh row
    const Rect rr{l.body.x, l.body.y + l.next_row, l.body.w, l.row_h > 0 ? l.row_h : t.cell_height};
    const ID id = hash_u32(get_id(s, "##row"), (uint32_t)index);
    Response rs = update_control(ctx, id, rr, true, false);
    if (rs.focused && key_pressed(s, KeyEnter)) rs.clicked = true;
    const float h = animate(s, id, rs.hovered, false);
    if (selected)
        s.dl.rect(rr, t.selection, t.corner_radius);
    else
        s.dl.rect(rr, with_alpha(t.surface_hover, (index & 1 ? 0.55f : 0.0f) + 0.45f * h), t.corner_radius);
    focus_ring(s, rr, rs, t.corner_radius);
    return rs;
}

void list_clipper_begin(Context& ctx, int count, float row_height, int& first, int& last) {
    State& s = state(ctx);
    first = 0;
    last = count < 0 ? 0 : count;
    if (s.layout_n == 0 || s.clip_n == 0 || count <= 0) return;
    const Layout& l = top_layout(s);
    const float step = (row_height > 0 ? row_height : s.cfg.theme.cell_height) + s.cfg.theme.spacing;
    const float y0 = l.body.y + l.next_row;
    const Rect& clip = s.clips[s.clip_n - 1];
    auto clampi = [](float v, int lo, int hi) { return v < (float)lo ? lo : (v > (float)hi ? hi : (int)v); };
    first = clampi(std::floor((clip.y - y0) / step), 0, count);
    last = clampi(std::ceil((clip.y + clip.h - y0) / step), first, count);
    spacing(ctx, (float)first * step);
    s.clipper_rest = (float)(count - last) * step;
}

void list_clipper_end(Context& ctx) {
    State& s = state(ctx);
    if (s.clipper_rest > 0) spacing(ctx, s.clipper_rest);
    s.clipper_rest = 0;
}

}  // namespace rd
