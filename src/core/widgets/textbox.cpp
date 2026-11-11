// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace {

TextEdit* find_edit(State& s, ID id) {
    TextEdit *free_slot = nullptr, *stale = nullptr;
    for (int i = 0; i < s.cfg.max_textboxes; i++) {
        TextEdit& e = s.edits[i];
        if (e.id == id) return &e;
        if (e.id == 0) {
            if (!free_slot) free_slot = &e;
        } else if (!stale && e.last_frame + 2 < s.frame) {
            stale = &e;
        }
    }
    TextEdit* e = free_slot ? free_slot : stale;
    if (e) *e = TextEdit{id, 0, 0, 0, 0, s.frame};
    return e;
}

bool is_cont(char c) {
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

int prev_boundary(const TextBuffer& b, int i) {
    if (i <= 0) return 0;
    i--;
    while (i > 0 && is_cont(b.data[i])) i--;
    return i;
}

int next_boundary(const TextBuffer& b, int i) {
    if (i >= b.length) return b.length;
    i++;
    while (i < b.length && is_cont(b.data[i])) i++;
    return i;
}

struct Editor {
    TextBuffer& b;
    TextEdit& e;
    bool changed = false;

    int lo() const { return e.cursor < e.anchor ? e.cursor : e.anchor; }
    int hi() const { return e.cursor < e.anchor ? e.anchor : e.cursor; }
    bool has_sel() const { return e.cursor != e.anchor; }

    void erase(int from, int to) {
        if (to <= from) return;
        std::memmove(b.data + from, b.data + to, (size_t)(b.length - to));
        b.length -= to - from;
        b.data[b.length] = 0;
        e.cursor = e.anchor = from;
        changed = true;
    }
    void insert(const char* p, int n) {
        if (has_sel()) erase(lo(), hi());
        int room = b.capacity - 1 - b.length;
        if (n > room) {
            n = room;
            while (n > 0 && is_cont(p[n])) n--;  // never split a code point
        }
        if (n <= 0) return;
        std::memmove(b.data + e.cursor + n, b.data + e.cursor, (size_t)(b.length - e.cursor));
        std::memcpy(b.data + e.cursor, p, (size_t)n);
        b.length += n;
        b.data[b.length] = 0;
        e.cursor += n;
        e.anchor = e.cursor;
        changed = true;
    }
    void move(int to, bool extend) {
        e.cursor = to;
        if (!extend) e.anchor = to;
    }
};

}  // namespace

namespace detail {

// shortcut: text areas break only at '\n' (no soft wrap) and password fields show at most 256 stars;
// upgrade when someone needs word-wrapped editing.
Response textbox_impl(Context& ctx, ID id, Rect r, TextBuffer& buf, const char* hint, int mode, float inset_l,
                      float inset_r) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const bool multi = mode == TextMulti, pw = mode == TextPassword;
    Response rs = update_control(ctx, id, {r.x, r.y, r.w - inset_r, r.h}, true, true);
    if (rs.hovered || s.active == id) s.cursor = Cursor::Text;
    if (!buf.data || buf.capacity < 1) {
        s.error = true;
        draw_frame(s, r, t.background, false);
        return rs;
    }
    if (buf.length < 0) buf.length = 0;
    if (buf.length > buf.capacity - 1) buf.length = buf.capacity - 1;
    buf.data[buf.length] = 0;

    const float vpad = multi ? t.padding * 0.5f : 0;
    const Rect ir{r.x + t.padding + inset_l, r.y + vpad, r.w - 2 * t.padding - inset_l - inset_r,
                  r.h - 2 * vpad};
    const FontHandle fh = t.font_body;
    float lh = text_size(s, fh, "M").y;
    if (lh <= 0) lh = (multi ? t.cell_height : ir.h) * 0.6f;  // no font loaded: still draw a caret

    char mask[256];
    auto disp = [&]() -> std::string_view {  // what is drawn: the text, or stars for passwords
        if (!pw) return {buf.data, (size_t)buf.length};
        int n = 0;
        for (int i = 0; i < buf.length && n < (int)sizeof mask; i++)
            if (!is_cont(buf.data[i])) mask[n++] = '*';
        return {mask, (size_t)n};
    };
    auto to_disp = [&](int byte) {
        if (!pw) return byte;
        int n = 0;
        for (int i = 0; i < byte; i++)
            if (!is_cont(buf.data[i])) n++;
        return n < (int)sizeof mask ? n : (int)sizeof mask;
    };
    auto from_disp = [&](int di) {
        if (!pw) return di;
        int i = 0;
        for (; i < buf.length && di > 0; di--) i = next_boundary(buf, i);
        return i;
    };
    auto line_start = [&](int i) {
        while (multi && i > 0 && buf.data[i - 1] != '\n') i--;
        return multi ? i : 0;
    };
    auto line_end = [&](int i) {
        while (multi && i < buf.length && buf.data[i] != '\n') i++;
        return multi ? i : buf.length;
    };
    auto line_view = [&](int i) {
        const int a = line_start(i);
        return std::string_view(buf.data + a, (size_t)(line_end(i) - a));
    };
    auto line_no = [&](int i) {
        int n = 0;
        for (int k = 0; multi && k < i; k++) n += buf.data[k] == '\n';
        return n;
    };
    auto caret_x = [&](int i) {
        return multi ? x_of_index(s, fh, line_view(i), i - line_start(i))
                     : x_of_index(s, fh, disp(), to_disp(i));
    };
    auto index_on_line = [&](int i, float x) {  // the byte at `x` on the line holding byte `i`
        return line_start(i) + index_at_x(s, fh, line_view(i), x);
    };

    TextEdit* ed = (rs.focused || multi) ? find_edit(s, id) : nullptr;  // text areas keep their scroll
    if (rs.focused && !ed) s.error = true;
    if (ed) {
        ed->last_frame = s.frame;
        ed->cursor = ed->cursor > buf.length ? buf.length : ed->cursor;
        ed->anchor = ed->anchor > buf.length ? buf.length : ed->anchor;
    }
    if (ed && rs.focused) {
        Editor ep{buf, *ed};
        bool moved = s.in.keys.count > 0 || s.in.text.count > 0;
        if (rs.pressed || (s.active == id && s.in.pointer_down)) {
            const float x = s.in.pointer.x - ir.x + ed->scroll;
            int i = 0;
            if (multi) {
                int row = (int)std::floor((s.in.pointer.y - ir.y + ed->scroll_y) / lh);
                for (; row > 0 && line_end(i) < buf.length; row--) i = line_end(i) + 1;
                i = index_on_line(i, x);
            } else {
                i = from_disp(index_at_x(s, fh, disp(), x));
            }
            ep.move(i, !rs.pressed);
            s.blink = 0;
            moved = true;
        }
        for (int k = 0; k < s.in.keys.count; k++) {
            const KeyEvent& ke = s.in.keys.items[k];
            switch (ke.key) {
                case KeyBackspace:
                    if (ep.has_sel())
                        ep.erase(ep.lo(), ep.hi());
                    else
                        ep.erase(prev_boundary(buf, ed->cursor), ed->cursor);
                    break;
                case KeyDelete:
                    if (ep.has_sel())
                        ep.erase(ep.lo(), ep.hi());
                    else
                        ep.erase(ed->cursor, next_boundary(buf, ed->cursor));
                    break;
                case KeyLeft:
                    if (ep.has_sel() && !ke.shift)
                        ep.move(ep.lo(), false);
                    else
                        ep.move(prev_boundary(buf, ed->cursor), ke.shift);
                    break;
                case KeyRight:
                    if (ep.has_sel() && !ke.shift)
                        ep.move(ep.hi(), false);
                    else
                        ep.move(next_boundary(buf, ed->cursor), ke.shift);
                    break;
                case KeyUp:
                case KeyDown:
                    if (multi) {
                        const float x = caret_x(ed->cursor);
                        const int a = line_start(ed->cursor), e = line_end(ed->cursor);
                        if (ke.key == KeyUp)
                            ep.move(a > 0 ? index_on_line(a - 1, x) : 0, ke.shift);
                        else
                            ep.move(e < buf.length ? index_on_line(e + 1, x) : buf.length, ke.shift);
                    }
                    break;
                case KeyHome:
                    ep.move(line_start(ed->cursor), ke.shift);
                    break;
                case KeyEnd:
                    ep.move(line_end(ed->cursor), ke.shift);
                    break;
                case KeyEnter:
                    if (multi)
                        ep.insert("\n", 1);
                    else
                        rs.clicked = true;
                    break;
                case KeyEscape:
                    s.focus = 0;
                    break;
                case KeyA:
                    if (ke.ctrl) {
                        ed->anchor = 0;
                        ed->cursor = buf.length;
                    }
                    break;
                case KeyC:
                case KeyX:
                    if (ke.ctrl && !pw && ep.has_sel() && s.clip_set) {  // never copy a password
                        s.clip_set(s.clip_user, buf.data + ep.lo(), ep.hi() - ep.lo());
                        if (ke.key == KeyX) ep.erase(ep.lo(), ep.hi());
                    }
                    break;
                case KeyV:
                    if (ke.ctrl && s.clip_get) {
                        char tmp[1024];
                        int n = s.clip_get(s.clip_user, tmp, (int)sizeof tmp);
                        int m = 0;
                        for (int i = 0; i < n && i < (int)sizeof tmp; i++)
                            if (((unsigned char)tmp[i] >= 0x20 && tmp[i] != 0x7f) ||
                                (multi && tmp[i] == '\n'))
                                tmp[m++] = tmp[i];
                        ep.insert(tmp, (int)utf8_trim_tail(tmp, (size_t)m));
                    }
                    break;
                default:
                    break;
            }
            s.blink = 0;
        }
        if (s.in.text.count > 0) {
            ep.insert(s.in.text.items, s.in.text.count);
            s.blink = 0;
        }
        rs.changed = ep.changed;

        // keep the caret in view
        const float cx = caret_x(ed->cursor);
        if (cx - ed->scroll > ir.w - 2) ed->scroll = cx - ir.w + 2;
        if (cx - ed->scroll < 0) ed->scroll = cx;
        if (ed->scroll < 0 || (!multi && text_size(s, fh, disp()).x < ir.w)) ed->scroll = 0;
        if (multi && moved) {
            const float cy = (float)line_no(ed->cursor) * lh;
            if (cy < ed->scroll_y) ed->scroll_y = cy;
            if (cy + lh - ed->scroll_y > ir.h) ed->scroll_y = cy + lh - ir.h;
        }
        s.wants_text_next = true;
        s.text_rect = r;
    }
    if (ed && multi) {
        const float maxy = std::fmax(0.0f, (float)(line_no(buf.length) + 1) * lh - ir.h);
        if (rs.hovered && s.in.wheel != 0 && !s.wheel_used && maxy > 0) {
            ed->scroll_y -= s.in.wheel * lh;
            s.wheel_used = true;
        }
        ed->scroll_y = std::fmin(std::fmax(ed->scroll_y, 0.0f), maxy);
    }

    draw_frame(s, r, t.background, rs.focused, true);
    const bool editing = ed && rs.focused;
    const float sx = ed ? ed->scroll : 0, sy = ed ? ed->scroll_y : 0;
    const int lo = editing ? (ed->cursor < ed->anchor ? ed->cursor : ed->anchor) : 0;
    const int hi = editing ? (ed->cursor < ed->anchor ? ed->anchor : ed->cursor) : 0;
    const bool caret = editing && std::fmod(s.blink, 1.0f) < 0.5f;
    const Color sel = with_alpha(t.accent, 0.4f);
    push_clip(s, ir);
    if (multi) {
        if (buf.length == 0 && hint) draw_text(s, fh, {ir.x, ir.y}, hint, t.text_muted);
        int a = 0;
        for (int row = 0;; row++) {
            const int e = line_end(a);
            const float y = ir.y - sy + (float)row * lh;
            if (y + lh > ir.y && y < ir.y + ir.h) {
                const std::string_view line(buf.data + a, (size_t)(e - a));
                if (lo < hi && lo <= e && hi > a) {
                    const float x0 = x_of_index(s, fh, line, (lo > a ? lo : a) - a);
                    const float x1 =
                        x_of_index(s, fh, line, (hi < e ? hi : e) - a) + (hi > e ? lh * 0.3f : 0);
                    s.dl.rect({ir.x - sx + x0, y, x1 - x0, lh}, sel);
                }
                draw_text(s, fh, {ir.x - sx, y}, line, t.text);
                if (caret && ed->cursor >= a && ed->cursor <= e)
                    s.dl.rect({ir.x - sx + x_of_index(s, fh, line, ed->cursor - a), y, 1.5f, lh}, t.text);
            }
            if (e >= buf.length) break;
            a = e + 1;
        }
    } else {
        const std::string_view text = disp();
        const float ty = ir.y + (ir.h - lh) / 2;
        if (lo < hi) {
            const float x0 = x_of_index(s, fh, text, to_disp(lo)), x1 = x_of_index(s, fh, text, to_disp(hi));
            s.dl.rect({ir.x - sx + x0, ty, x1 - x0, lh}, sel);
        }
        if (buf.length == 0 && hint) draw_text(s, fh, {ir.x, ty}, hint, t.text_muted);
        draw_text(s, fh, {ir.x - sx, ty}, text, t.text);
        if (caret)
            s.dl.rect({ir.x - sx + x_of_index(s, fh, text, to_disp(ed->cursor)), ty, 1.5f, lh}, t.text);
    }
    pop_clip(s);
    return rs;
}

}  // namespace detail

Response textbox(Context& ctx, std::string_view label, TextBuffer& buf, const char* hint) {
    State& s = state(ctx);
    ID id = get_id(s, label);
    return textbox_impl(ctx, id, layout_next(ctx), buf, hint, TextSingle, 0, 0);
}

Response password_box(Context& ctx, std::string_view label, TextBuffer& buf, const char* hint) {
    State& s = state(ctx);
    ID id = get_id(s, label);
    return textbox_impl(ctx, id, layout_next(ctx), buf, hint, TextPassword, 0, 0);
}

Response text_area(Context& ctx, std::string_view label, TextBuffer& buf, float height, const char* hint) {
    State& s = state(ctx);
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    extend_height(s, r, height);
    r.h = height;
    return textbox_impl(ctx, id, r, buf, hint, TextMulti, 0, 0);
}

Response search_box(Context& ctx, std::string_view label, TextBuffer& buf, const char* hint) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    const bool has_text = buf.data && buf.length > 0;
    const float side = r.h;
    Response rs = textbox_impl(ctx, id, r, buf, hint, TextSingle, side * 0.7f, has_text ? side : 0);
    draw_icon(ctx, Icon::Search, {r.x + t.padding * 0.5f, r.y, side, side}, t.text_muted);
    if (has_text) {  // clear button
        const Rect cb{r.x + r.w - side, r.y, side, side};
        const ID cid = hash_u32(id, 1);
        const Response cr = update_control(ctx, cid, cb, false, false);
        draw_icon(ctx, Icon::Close, {cb.x + side * 0.15f, cb.y + side * 0.15f, side * 0.7f, side * 0.7f},
                  cr.hovered ? t.text : t.text_muted);
        if (cr.clicked) {
            buf.length = 0;
            buf.data[0] = 0;
            rs.changed = true;
        }
    }
    return rs;
}

}  // namespace rd
