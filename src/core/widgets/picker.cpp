// Copyright (c) 2026 Wavestorm Software
// Wheel and time pickers, searchable and multi-select combos, autocomplete and a number pad.
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace {

char lower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

// Case-insensitive (ASCII) substring test.
bool contains(std::string_view hay, std::string_view needle) {
    if (needle.empty()) return true;
    for (size_t i = 0; i + needle.size() <= hay.size(); i++) {
        size_t k = 0;
        while (k < needle.size() && lower(hay[i + k]) == lower(needle[k])) k++;
        if (k == needle.size()) return true;
    }
    return false;
}

// A combo-style field: the value text, an icon or arrow at the right. Opens popup `id` when activated.
Response field(Context& ctx, ID id, const Rect& r, std::string_view text, const Icon* icon) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Response rs = update_control(ctx, id, r, true, false);
    if ((rs.clicked && s.popup_suppress != id) || (rs.focused && key_pressed(s, KeyEnter)))
        open_popup_id(s, id);
    draw_frame(s, r, control_color(s, id, rs), rs.focused);
    draw_text_fit(s, {r.x + t.padding, r.y, r.w - 2 * t.padding - r.h, r.h}, text, t.text, false);
    if (icon) {
        draw_icon(ctx, *icon, {r.x + r.w - r.h, r.y, r.h, r.h}, t.text_muted);
    } else {
        const float a = r.h * 0.14f;
        const Vec2 c{r.x + r.w - t.padding - a, r.y + r.h / 2};
        s.dl.triangle({c.x - a, c.y - a * 0.6f}, {c.x + a, c.y - a * 0.6f}, {c.x, c.y + a * 0.8f},
                      t.text_muted);
    }
    return rs;
}

// "00".."59" for the time wheels.
const char* const* two_digits() {
    static char text[60][3];
    static const char* ptr[60];
    static const bool init = [] {
        for (int i = 0; i < 60; i++) {
            text[i][0] = (char)('0' + i / 10);
            text[i][1] = (char)('0' + i % 10);
            text[i][2] = 0;
            ptr[i] = text[i];
        }
        return true;
    }();
    (void)init;
    return ptr;
}

}  // namespace

bool wheel_picker(Context& ctx, std::string_view label, int& selected, const char* const* items, int count,
                  int visible) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const ID id = get_id(s, label);
    visible = visible < 1 ? 1 : visible;
    const float rh = t.cell_height;
    Rect r = layout_next(ctx);
    extend_height(s, r, rh * (float)visible);
    r.h = rh * (float)visible;
    if (!items || count < 1) return false;
    ChildState* c = find_child(s, id);
    if (!c) {
        s.error = true;
        return false;
    }
    const int sel0 = selected;
    selected = selected < 0 ? 0 : (selected >= count ? count - 1 : selected);
    if (c->content_h == 0 || c->last_frame + 1 < s.frame) {  // first frame shown: start at the value
        c->scroll = (float)selected * rh;
        c->vel = 0;
        c->content_h = 1;
        c->lock = selected;
    }
    c->last_frame = s.frame;
    Response rs = update_control(ctx, id, r, true, true);
    const float maxs = (float)(count - 1) * rh;
    const bool drag = s.active == id && s.in.pointer_down && !rs.pressed;
    kinetic(s, c->scroll, c->vel, drag, maxs);
    c->scroll = std::fmin(std::fmax(c->scroll, 0.0f), maxs);
    const bool settling = drag || std::fabs(c->vel) > 8;
    if (settling) {
        selected = (int)std::lround(c->scroll / rh);
    } else {
        const float dy = s.in.pointer.y - (r.y + r.h / 2);
        if (rs.clicked && !s.touch_moved && std::fabs(dy) > rh / 2)  // tap a row above or below the band
            selected += (int)std::lround(dy / rh);
        if (rs.hovered && s.in.wheel != 0 && !s.wheel_used) {
            selected -= s.in.wheel > 0 ? 1 : -1;
            s.wheel_used = true;
        }
        if (rs.focused) {
            if (key_pressed(s, KeyUp)) selected--;
            if (key_pressed(s, KeyDown)) selected++;
        }
        selected = selected < 0 ? 0 : (selected >= count ? count - 1 : selected);
        const float target = (float)selected * rh;
        c->scroll += (target - c->scroll) * (s.cfg.animations ? 1 - std::exp(-s.dt * 14) : 1.0f);
        if (std::fabs(c->scroll - target) < 0.3f) c->scroll = target;
        if (c->scroll != target) s.animating = true;
    }
    selected = selected < 0 ? 0 : (selected >= count ? count - 1 : selected);
    const int detent = (int)std::lround(c->scroll / rh);
    if (detent != c->lock) {
        c->lock = detent;
        haptic(s, Haptic::Selection);
    }
    rs.changed = selected != sel0;
    a11y_node(s, A11yRole::Slider, label, items[selected]);

    s.dl.rect(r, t.background, t.corner_radius);
    const float cy = r.y + r.h / 2;
    s.dl.rect({r.x + 3, cy - rh / 2, r.w - 6, rh}, t.surface_active, t.corner_radius);
    push_clip(s, r);
    const int first = (int)std::floor((c->scroll - r.h / 2) / rh) - 1;
    for (int i = first < 0 ? 0 : first; i < count && i <= first + visible + 2; i++) {
        const float y = cy - rh / 2 + (float)i * rh - c->scroll;
        const float d = clamp01(std::fabs(y + rh / 2 - cy) / (r.h / 2));
        const bool on = i == detent;
        draw_text_fit(s, {r.x + t.padding, y, r.w - 2 * t.padding, rh}, items[i],
                      with_alpha(on ? t.text : t.text_muted, 1 - 0.7f * d), true);
    }
    pop_clip(s);
    if (visible > 1) {  // fade the rows toward the edges
        const Color b = t.background, z = with_alpha(t.background, 0);
        const float fh = rh * 0.9f;
        s.dl.gradient_rect({r.x + 1, r.y + 1, r.w - 2, fh}, b, b, z, z);
        s.dl.gradient_rect({r.x + 1, r.y + r.h - fh - 1, r.w - 2, fh}, z, z, b, b);
    }
    s.dl.rect(r, t.border, t.corner_radius, t.border_width);
    focus_ring(s, r, rs, t.corner_radius);
    return rs.changed;
}

bool time_picker(Context& ctx, std::string_view label, int& hour, int& minute, bool h24) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const ID id = get_id(s, label);
    hour = hour < 0 ? 0 : (hour > 23 ? 23 : hour);
    minute = minute < 0 ? 0 : (minute > 59 ? 59 : minute);
    const int h0 = hour, m0 = minute;
    char text[16];
    if (h24)
        std::snprintf(text, sizeof text, "%02d:%02d", hour, minute);
    else
        std::snprintf(text, sizeof text, "%d:%02d %s", (hour + 11) % 12 + 1, minute, hour < 12 ? "AM" : "PM");
    const Icon clock = Icon::Clock;
    field(ctx, id, layout_next(ctx), text, &clock);
    a11y_node(s, A11yRole::ComboBox, label, text);
    const int cols = h24 ? 2 : 3;
    const float cw = t.cell_height * 2.2f;
    if (popup_begin_id(ctx, id, cw * (float)cols + t.spacing * (float)(cols - 1) + 2 * t.padding)) {
        static const char* const ampm[] = {"AM", "PM"};
        static const char* const h12[] = {"12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};
        layout_row(ctx, cols, nullptr, 0, t.cell_height * 5);
        if (h24) {
            wheel_picker(ctx, "##h", hour, two_digits(), 24, 5);
        } else {
            int h = hour % 12, pm = hour >= 12;
            wheel_picker(ctx, "##h", h, h12, 12, 5);
            wheel_picker(ctx, "##m", minute, two_digits(), 60, 5);
            wheel_picker(ctx, "##p", pm, ampm, 2, 5);
            hour = h + 12 * pm;
        }
        if (h24) wheel_picker(ctx, "##m", minute, two_digits(), 60, 5);
        end_window(ctx);
    }
    return hour != h0 || minute != m0;
}

bool combo_search(Context& ctx, std::string_view label, int& selected, const char* const* items, int count) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const ID id = get_id(s, label);
    const bool was_open = s.popup_id == id;
    field(ctx, id, layout_next(ctx), (selected >= 0 && selected < count && items) ? items[selected] : "",
          nullptr);
    a11y_node(s, A11yRole::ComboBox, label,
              (selected >= 0 && selected < count && items) ? items[selected] : "");
    const ID fid = hash_u32(id, 1);
    if (s.popup_id == id && !was_open) {  // just opened: clear the filter and type straight into it
        s.search_len = 0;
        s.search[0] = 0;
        s.focus = fid;
        s.focus_window = id;
    }
    bool changed = false;
    if (popup_begin_id(ctx, id, s.last_rect.w)) {
        layout_row(ctx, 1, nullptr, 0, 0);
        TextBuffer tb{s.search, (int)sizeof s.search, s.search_len};
        const Rect fr = layout_next(ctx);
        const Response sr = textbox_impl(ctx, fid, fr, tb, "Search", TextSingle, fr.h * 0.7f, 0);
        draw_icon(ctx, Icon::Search, {fr.x + t.padding * 0.5f, fr.y, fr.h, fr.h}, t.text_muted);
        s.search_len = tb.length;
        const std::string_view q(s.search, (size_t)s.search_len);
        int shown = 0;
        for (int i = 0; i < count && items; i++) {
            if (!contains(items[i], q)) continue;
            if (sr.clicked && shown == 0) {  // Enter picks the first match
                changed = selected != i;
                selected = i;
                s.popup_id = 0;
            }
            shown++;
            layout_row(ctx, 1, nullptr, 0, 0);
            if (s.id_n < s.cfg.max_id_stack) s.id_stack[s.id_n++] = hash_u32(top_id(s), (uint32_t)i);
            if (selectable(ctx, items[i], i == selected).clicked) {
                changed = changed || selected != i;
                selected = i;
                s.popup_id = 0;
            }
            if (s.id_n > 0) s.id_n--;
        }
        if (shown == 0) {
            layout_row(ctx, 1, nullptr, 0, 0);
            draw_text_fit(s, layout_next(ctx), "No matches", t.text_muted, true);
        }
        end_window(ctx);
    }
    return changed;
}

bool multi_combo(Context& ctx, std::string_view label, bool* selected, const char* const* items, int count) {
    State& s = state(ctx);
    const ID id = get_id(s, label);
    char summary[128];
    int n = 0, len = 0;
    summary[0] = 0;
    for (int i = 0; i < count && selected && items; i++) {
        if (!selected[i]) continue;
        n++;
        if (len < 0) continue;
        const int w =
            std::snprintf(summary + len, sizeof summary - (size_t)len, "%s%s", len ? ", " : "", items[i]);
        len = (w < 0 || len + w >= (int)sizeof summary) ? -1 : len + w;  // too long: show a count
    }
    if (len < 0) std::snprintf(summary, sizeof summary, "%d selected", n);
    if (n == 0) std::snprintf(summary, sizeof summary, "None");
    field(ctx, id, layout_next(ctx), summary, nullptr);
    a11y_node(s, A11yRole::ComboBox, label, summary);
    bool changed = false;
    if (popup_begin_id(ctx, id, s.last_rect.w)) {
        for (int i = 0; i < count && selected && items; i++) {
            layout_row(ctx, 1, nullptr, 0, 0);
            if (s.id_n < s.cfg.max_id_stack) s.id_stack[s.id_n++] = hash_u32(top_id(s), (uint32_t)i);
            changed |= checkbox(ctx, items[i], selected[i]).changed;
            if (s.id_n > 0) s.id_n--;
        }
        end_window(ctx);
    }
    return changed;
}

Response autocomplete(Context& ctx, std::string_view label, TextBuffer& buf, const char* const* suggestions,
                      int count, const char* hint) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const ID id = get_id(s, label);
    const ID pid = hash_u32(id, 0xAC);
    const Rect r = layout_next(ctx);
    const bool open = s.popup_id == pid;
    bool fresh = false;
    Anim* hl = anim_slot(s, hash_u32(pid, 1), fresh);  // highlighted suggestion in `key`
    if (hl && (fresh || !open)) hl->key = -1;
    if (hl) hl->last_frame = s.frame;
    // Arrow keys move the highlight; Enter takes it (consume Enter before the text box submits).
    int pick = -1;
    if (open && hl && s.focus == id) {
        for (int k = 0; k < s.in.keys.count; k++) {
            const int key = s.in.keys.items[k].key;
            if (key == KeyDown) hl->key++;
            if (key == KeyUp) hl->key--;
            if (key == KeyEnter && hl->key >= 0) pick = hl->key, s.in.keys.items[k].key = KeyNone;
        }
    }
    Response rs = textbox_impl(ctx, id, r, buf, hint, TextSingle, 0, 0);
    const std::string_view q(buf.data ? buf.data : "", buf.data ? (size_t)buf.length : 0);
    int idx[8], n = 0;
    for (int i = 0; i < count && suggestions && n < 8 && !q.empty(); i++)
        if (contains(suggestions[i], q) && q != suggestions[i]) idx[n++] = i;
    if (hl) hl->key = n == 0 ? -1 : (hl->key < -1 ? n - 1 : (hl->key >= n ? 0 : hl->key));
    if (rs.focused && rs.changed && n > 0 && !open) {
        open_popup_id(s, pid);
        s.popup_anchor = r;
    }
    if (s.popup_id == pid && (n == 0 || q.empty())) s.popup_id = 0;
    auto choose = [&](int i) {
        const std::string_view v = suggestions[i];
        const int m = (int)utf8_fit(v, (size_t)(buf.capacity - 1));
        if (m > 0) std::memcpy(buf.data, v.data(), (size_t)m);
        buf.length = m;
        buf.data[m] = 0;
        if (TextEdit* e = find_edit(s, id)) e->cursor = e->anchor = m;
        rs.changed = rs.clicked = true;
        s.popup_id = 0;
        s.focus = id;
    };
    if (pick >= 0 && pick < n) choose(idx[pick]);
    if (popup_begin_id(ctx, pid, r.w)) {
        for (int i = 0; i < n; i++) {
            layout_row(ctx, 1, nullptr, 0, 0);
            const Rect c = layout_next(ctx);
            const ID cid = hash_u32(pid, 16 + (uint32_t)i);
            const Response cr = update_control(ctx, cid, c, false, false);  // keeps the text box focused
            const bool on = hl && hl->key == i;
            s.dl.rect(c, on ? t.selection : with_alpha(t.surface_active, animate(s, cid, cr.hovered, false)),
                      t.corner_radius);
            draw_text_fit(s, {c.x + t.padding, c.y, c.w - 2 * t.padding, c.h}, suggestions[idx[i]], t.text,
                          false);
            if (cr.clicked) choose(idx[i]);
        }
        end_window(ctx);
    }
    return rs;
}

Response number_pad(Context& ctx, std::string_view label, TextBuffer& buf, bool decimal) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    Response out;
    if (!buf.data || buf.capacity < 1) {
        s.error = true;
        return out;
    }
    push_id(ctx, label);
    static const char* const keys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", ""};
    for (int row = 0; row < 4; row++) {
        layout_row(ctx, 3, nullptr, 0, t.cell_height * 1.2f);
        for (int col = 0; col < 3; col++) {
            const int k = row * 3 + col;
            const Rect c = layout_next(ctx);
            if (k == 9 && !decimal) continue;
            const ID kid = hash_u32(top_id(s), (uint32_t)k);
            const Response kr = update_control(ctx, kid, c, false, false);  // the field keeps focus
            s.dl.rect(c, control_color(s, kid, kr), t.corner_radius);
            if (k == 11)
                draw_icon(ctx, Icon::Backspace, c, t.text);
            else
                draw_text_fit(s, c, keys[k], t.text, true);
            if (kr.pressed) haptic(s, Haptic::Light);
            if (!kr.clicked) continue;
            out.clicked = true;
            if (k == 11) {
                int n = buf.length;
                while (n > 0 && (static_cast<unsigned char>(buf.data[n - 1]) & 0xC0) == 0x80) n--;
                buf.length = n > 0 ? n - 1 : 0;
            } else if (buf.length < buf.capacity - 1 &&
                       (k != 9 || !std::memchr(buf.data, '.', (size_t)buf.length))) {
                buf.data[buf.length++] = keys[k][0];
            } else {
                continue;
            }
            buf.data[buf.length] = 0;
            out.changed = true;
        }
    }
    pop_id(ctx);
    return out;
}

}  // namespace rd
