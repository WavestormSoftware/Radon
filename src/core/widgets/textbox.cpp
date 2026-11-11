// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace detail {

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

}  // namespace detail

namespace {

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

bool is_word(char c) {
    const unsigned char u = static_cast<unsigned char>(c);
    return u >= 0x80 || (u >= '0' && u <= '9') || ((u | 0x20) >= 'a' && (u | 0x20) <= 'z') || c == '_';
}

int word_left(const TextBuffer& b, int i) {
    while (i > 0 && !is_word(b.data[i - 1])) i--;
    while (i > 0 && is_word(b.data[i - 1])) i--;
    return i;
}

int word_right(const TextBuffer& b, int i) {
    while (i < b.length && !is_word(b.data[i])) i++;
    while (i < b.length && is_word(b.data[i])) i++;
    return i;
}

// The word around byte i as [lo, hi); an empty range between two non-word characters.
void word_at(const TextBuffer& b, int i, int& lo, int& hi) {
    lo = hi = i;
    while (lo > 0 && is_word(b.data[lo - 1])) lo--;
    while (hi < b.length && is_word(b.data[hi])) hi++;
    if (lo == hi && hi < b.length) hi = next_boundary(b, hi);  // a lone symbol or space
}

struct Editor {
    TextBuffer& b;
    TextEdit& e;
    bool ro = false;
    bool changed = false;

    int lo() const { return e.cursor < e.anchor ? e.cursor : e.anchor; }
    int hi() const { return e.cursor < e.anchor ? e.anchor : e.cursor; }
    bool has_sel() const { return e.cursor != e.anchor; }

    void erase(int from, int to) {
        if (to <= from || ro) return;
        std::memmove(b.data + from, b.data + to, (size_t)(b.length - to));
        b.length -= to - from;
        b.data[b.length] = 0;
        e.cursor = e.anchor = from;
        changed = true;
    }
    void insert(const char* p, int n) {
        if (ro) return;
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
    void restore(const UndoStack& u) {
        const UndoStack::Entry& en = u.e[u.n - 1];
        const int n = en.len < b.capacity - 1 ? en.len : b.capacity - 1;
        std::memcpy(b.data, u.mem + en.off, (size_t)n);
        b.length = n;
        b.data[n] = 0;
        e.cursor = e.anchor = en.cursor < n ? en.cursor : n;
        changed = true;
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
    const bool multi = mode & TextMulti, pw = mode & TextPassword, ro = mode & TextReadOnly;
    const ID h1 = hash_u32(id, 0x48A1u), h2 = hash_u32(id, 0x48A2u), bar = hash_u32(id, 0xBA7u);
    const bool had_focus = s.focus == id;
    if (!buf.data || buf.capacity < 1) {
        Response rs = update_control(ctx, id, {r.x, r.y, r.w - inset_r, r.h}, true, true);
        s.error = true;
        draw_frame(s, r, t.background, false);
        return rs;
    }
    if (buf.length < 0) buf.length = 0;
    if (buf.length > buf.capacity - 1) buf.length = buf.capacity - 1;
    if (!ro) buf.data[buf.length] = 0;  // read-only text may be a view into someone else's string

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

    TextEdit* ed = (had_focus || multi || s.active == h1 || s.active == h2) ? find_edit(s, id) : nullptr;
    const float sx0 = ed ? ed->scroll : 0, sy0 = ed ? ed->scroll_y : 0;
    const float ty = multi ? ir.y : ir.y + (ir.h - lh) / 2;
    auto hit = [&](Vec2 p) {  // byte index under a screen point
        const float x = p.x - ir.x + sx0;
        if (!multi) return from_disp(index_at_x(s, fh, disp(), x));
        int i = 0;
        for (int row = (int)std::floor((p.y - ir.y + sy0) / lh); row > 0 && line_end(i) < buf.length; row--)
            i = line_end(i) + 1;
        return index_on_line(i, x);
    };
    auto caret_pos = [&](int i) {  // bottom of the caret at byte i, on screen
        return Vec2{ir.x - sx0 + caret_x(i), ty - sy0 + (float)line_no(i) * lh + lh};
    };

    // Touch selection handles take the press before the field does.
    Response hr1, hr2;
    const bool handles = ed && had_focus && s.in.touch && ed->cursor != ed->anchor && !pw;
    if (handles) {
        const int lo = ed->cursor < ed->anchor ? ed->cursor : ed->anchor;
        const int hi = ed->cursor < ed->anchor ? ed->anchor : ed->cursor;
        const float hs = 28;
        const Vec2 a = caret_pos(lo), b = caret_pos(hi);
        hr1 = update_control(ctx, h1, {a.x - hs / 2, a.y - 4, hs, hs}, false, true);
        hr2 = update_control(ctx, h2, {b.x - hs / 2, b.y - 4, hs, hs}, false, true);
        if (hr1.pressed) ed->anchor = hi, ed->cursor = lo, ed->handle = 1;
        if (hr2.pressed) ed->anchor = lo, ed->cursor = hi, ed->handle = 2;
    }

    Response rs = update_control(ctx, id, {r.x, r.y, r.w - inset_r, r.h}, true, true);
    if (s.active == h1 || s.active == h2) rs.focused = true, s.focus = id, s.updated_focus = true;
    if (rs.hovered || s.active == id) s.cursor = Cursor::Text;
    if (rs.focused && !ed) ed = find_edit(s, id);
    if (rs.focused && !ed) s.error = true;
    if (ed) {
        ed->last_frame = s.frame;
        ed->cursor = ed->cursor > buf.length ? buf.length : ed->cursor;
        ed->anchor = ed->anchor > buf.length ? buf.length : ed->anchor;
    }
    bool show_bar = false;
    if (ed && rs.focused) {
        Editor ep{buf, *ed, ro};
        bool moved = s.in.keys.count > 0 || s.in.text.count > 0;
        // Each change is recorded for undo; typing runs are merged into one step.
        auto edit = [&](bool typing, auto&& fn) {
            if (ro) return;
            if (s.undo_id != id) {
                s.undo.clear();
                s.redo.clear();
                s.undo_id = id;
                s.undo_typing = false;
            }
            const bool merge = typing && s.undo_typing;
            const bool before = ep.changed;
            ep.changed = false;
            if (!merge) s.undo.push(buf.data, buf.length, ed->cursor);
            fn();
            if (ep.changed) {
                s.redo.clear();
                s.undo_typing = typing;
            } else if (!merge) {
                s.undo.pop();
            }
            ep.changed = ep.changed || before;
        };
        auto copy = [&]() {
            if (!pw && ep.has_sel() && s.clip_set)
                s.clip_set(s.clip_user, buf.data + ep.lo(), ep.hi() - ep.lo());
        };
        auto paste = [&]() {
            if (!s.clip_get) return;
            char tmp[1024];
            int n = s.clip_get(s.clip_user, tmp, (int)sizeof tmp);
            int m = 0;
            for (int i = 0; i < n && i < (int)sizeof tmp; i++)
                if (((unsigned char)tmp[i] >= 0x20 && tmp[i] != 0x7f) || (multi && tmp[i] == '\n'))
                    tmp[m++] = tmp[i];
            edit(false, [&] { ep.insert(tmp, (int)utf8_trim_tail(tmp, (size_t)m)); });
        };
        if (s.active == h1 || s.active == h2) {  // dragging a selection handle
            if (s.in.pointer_down) ep.move(hit({s.in.pointer.x, s.in.pointer.y - lh}), true);
            moved = true;
        } else if (rs.pressed || (s.active == id && s.in.pointer_down)) {
            const int i = hit(s.in.pointer);
            if (rs.pressed) {
                ed->word_lo = -1;
                ep.move(i, false);
                if (s.clicks == 2) {
                    word_at(buf, i, ed->word_lo, ed->word_hi);
                    show_bar = s.in.touch;
                } else if (s.clicks == 3) {
                    ed->word_lo = line_start(i), ed->word_hi = line_end(i);
                }
                s.undo_typing = false;
            } else if (s.long_fired && s.secondary) {  // long press: select the word, offer the edit bar
                word_at(buf, i, ed->word_lo, ed->word_hi);
                show_bar = true;
            }
            if (ed->word_lo >= 0) {  // word-wise selection while dragging
                int a, b;
                word_at(buf, i, a, b);
                if (i < ed->word_lo)
                    ed->anchor = ed->word_hi, ed->cursor = a;
                else if (i > ed->word_hi)
                    ed->anchor = ed->word_lo, ed->cursor = b;
                else
                    ed->anchor = ed->word_lo, ed->cursor = ed->word_hi;
            } else if (!rs.pressed) {
                ep.move(i, true);
            }
            s.blink = 0;
            moved = true;
        }
        const int page = multi ? (int)(ir.h / lh) - 1 : 0;
        for (int k = 0; k < s.in.keys.count; k++) {
            const KeyEvent& ke = s.in.keys.items[k];
            const bool word = ke.ctrl && !pw;
            switch (ke.key) {
                case KeyBackspace:
                    edit(false, [&] {
                        if (ep.has_sel())
                            ep.erase(ep.lo(), ep.hi());
                        else
                            ep.erase(word ? word_left(buf, ed->cursor) : prev_boundary(buf, ed->cursor),
                                     ed->cursor);
                    });
                    break;
                case KeyDelete:
                    edit(false, [&] {
                        if (ep.has_sel())
                            ep.erase(ep.lo(), ep.hi());
                        else
                            ep.erase(ed->cursor,
                                     word ? word_right(buf, ed->cursor) : next_boundary(buf, ed->cursor));
                    });
                    break;
                case KeyLeft:
                    if (ep.has_sel() && !ke.shift && !word)
                        ep.move(ep.lo(), false);
                    else
                        ep.move(word ? word_left(buf, ed->cursor) : prev_boundary(buf, ed->cursor), ke.shift);
                    s.undo_typing = false;
                    break;
                case KeyRight:
                    if (ep.has_sel() && !ke.shift && !word)
                        ep.move(ep.hi(), false);
                    else
                        ep.move(word ? word_right(buf, ed->cursor) : next_boundary(buf, ed->cursor),
                                ke.shift);
                    s.undo_typing = false;
                    break;
                case KeyUp:
                case KeyDown:
                case KeyPageUp:
                case KeyPageDown: {
                    const bool up = ke.key == KeyUp || ke.key == KeyPageUp;
                    if (!multi) {
                        if (ke.key == KeyPageUp || ke.key == KeyPageDown)
                            ep.move(up ? 0 : buf.length, ke.shift);
                        break;
                    }
                    const float x = caret_x(ed->cursor);
                    int c = ed->cursor;
                    for (int n = (ke.key == KeyUp || ke.key == KeyDown) ? 1 : (page > 1 ? page : 1); n > 0;
                         n--) {
                        const int a = line_start(c), e = line_end(c);
                        if (up)
                            c = a > 0 ? index_on_line(a - 1, x) : 0;
                        else
                            c = e < buf.length ? index_on_line(e + 1, x) : buf.length;
                    }
                    ep.move(c, ke.shift);
                    s.undo_typing = false;
                    break;
                }
                case KeyHome:
                    ep.move(ke.ctrl ? 0 : line_start(ed->cursor), ke.shift);
                    break;
                case KeyEnd:
                    ep.move(ke.ctrl ? buf.length : line_end(ed->cursor), ke.shift);
                    break;
                case KeyEnter:
                    if (multi)
                        edit(false, [&] { ep.insert("\n", 1); });
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
                    if (ke.ctrl) copy();
                    break;
                case KeyX:
                    if (ke.ctrl && !pw && ep.has_sel()) {
                        copy();
                        edit(false, [&] { ep.erase(ep.lo(), ep.hi()); });
                    }
                    break;
                case KeyV:
                    if (ke.ctrl) paste();
                    break;
                case KeyZ:
                case KeyY: {
                    if (!ke.ctrl || ro || s.undo_id != id) break;
                    const bool redo = ke.key == KeyY || ke.shift;
                    UndoStack& from = redo ? s.redo : s.undo;
                    UndoStack& to = redo ? s.undo : s.redo;
                    if (from.n == 0) break;
                    to.push(buf.data, buf.length, ed->cursor);
                    ep.restore(from);
                    from.pop();
                    s.undo_typing = false;
                    break;
                }
                default:
                    break;
            }
            s.blink = 0;
        }
        if (s.in.text.count > 0) {
            edit(true, [&] { ep.insert(s.in.text.items, s.in.text.count); });
            s.blink = 0;
        }

        // The edit bar: Cut / Copy / Paste / Select all, for touch screens without shortcuts.
        if (show_bar) {
            open_popup_id(s, bar);
            s.popup_anchor = r;
        }
        const char* acts[4];
        int act_id[4], na = 0;
        if (!pw && !ro && ep.has_sel()) acts[na] = "Cut", act_id[na++] = 0;
        if (!pw && ep.has_sel()) acts[na] = "Copy", act_id[na++] = 1;
        if (!ro && s.clip_get) acts[na] = "Paste", act_id[na++] = 2;
        acts[na] = "Select all", act_id[na++] = 3;
        if (s.popup_id == bar) {
            float bw = 0;
            for (int i = 0; i < na; i++) bw += text_size(s, fh, acts[i]).x + 2 * t.padding;
            if (popup_begin_id(ctx, bar, bw + 2 * t.padding + (float)na * t.spacing)) {
                layout_row(ctx, na, nullptr, 0, 0);
                for (int i = 0; i < na; i++) {
                    const Rect c = layout_next(ctx);
                    const ID bid = hash_u32(bar, (uint32_t)i);
                    const Response br = update_control(ctx, bid, c, false, false);
                    s.dl.rect(
                        c,
                        with_alpha(t.surface_active, animate(s, bid, br.hovered || s.active == bid, false)),
                        t.corner_radius);
                    draw_text_fit(s, c, acts[i], t.text, true);
                    if (!br.clicked) continue;
                    switch (act_id[i]) {
                        case 0:
                            copy();
                            edit(false, [&] { ep.erase(ep.lo(), ep.hi()); });
                            break;
                        case 1:
                            copy();
                            break;
                        case 2:
                            paste();
                            break;
                        default:
                            ed->anchor = 0;
                            ed->cursor = buf.length;
                            break;
                    }
                    if (act_id[i] != 3) s.popup_id = 0;
                }
                end_window(ctx);
            }
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
        // Keep the field above the soft keyboard: scroll the enclosing window or child.
        const float kb = s.in.keyboard_inset;
        if (kb > 0 && s.in.screen_size.y > 0 && (moved || rs.pressed || !had_focus || ed->kb != kb)) {
            const float limit = s.in.screen_size.y - kb - t.spacing;
            Rect view;
            if (float* sc = scroll_target(s, &view))
                if (r.y + r.h > limit) *sc += r.y + r.h - limit;
        }
        ed->kb = kb;
        if (!ro) s.wants_text_next = true;
        s.text_rect = r;
        s.text_type = (mode & TextPassword)  ? TextInputType::Password
                      : (mode & TextNumber)  ? TextInputType::Number
                      : (mode & TextDecimal) ? TextInputType::Decimal
                      : (mode & TextEmail)   ? TextInputType::Email
                      : (mode & TextUrl)     ? TextInputType::Url
                                             : TextInputType::Text;
    }
    if (ed && multi) {
        const float maxy = std::fmax(0.0f, (float)(line_no(buf.length) + 1) * lh - ir.h);
        if (rs.hovered && s.in.wheel != 0 && !s.wheel_used && maxy > 0) {
            ed->scroll_y -= s.in.wheel * lh;
            s.wheel_used = true;
        }
        ed->scroll_y = std::fmin(std::fmax(ed->scroll_y, 0.0f), maxy);
    }
    if (s.cfg.a11y) {
        s.last_id = id;
        s.last_rect = r;
        a11y_node(s, A11yRole::TextField, hint ? hint : "", pw ? std::string_view{} : disp(),
                  ro ? A11yNode::Disabled : 0u);
    }

    if (!ro || rs.focused) draw_frame(s, r, ro ? Color{0, 0, 0, 0} : t.background, rs.focused, !ro);
    const bool editing = ed && rs.focused;
    const float sx = ed ? ed->scroll : 0, sy = ed ? ed->scroll_y : 0;
    const int lo = editing ? (ed->cursor < ed->anchor ? ed->cursor : ed->anchor) : 0;
    const int hi = editing ? (ed->cursor < ed->anchor ? ed->anchor : ed->cursor) : 0;
    const bool caret = editing && !ro && std::fmod(s.blink, 1.0f) < 0.5f;
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
    if (editing && s.in.touch && lo < hi && !pw) {  // selection handles
        for (int i : {lo, hi}) {
            const Vec2 p{ir.x - sx + caret_x(i), ty - sy + (float)line_no(i) * lh + lh};
            s.dl.rect({p.x - 0.75f, p.y - lh, 1.5f, lh}, t.accent);
            s.dl.rect({p.x - 7, p.y + 2, 14, 14}, t.accent, 7);
        }
    }
    return rs;
}

}  // namespace detail

Response textbox(Context& ctx, std::string_view label, TextBuffer& buf, const char* hint) {
    State& s = state(ctx);
    ID id = get_id(s, label);
    return textbox_impl(ctx, id, layout_next(ctx), buf, hint, TextSingle, 0, 0);
}

Response textbox(Context& ctx, std::string_view label, TextBuffer& buf, const char* hint,
                 TextInputType type) {
    State& s = state(ctx);
    ID id = get_id(s, label);
    static const int modes[] = {TextSingle, TextNumber, TextDecimal, TextEmail, TextUrl, TextPassword};
    return textbox_impl(ctx, id, layout_next(ctx), buf, hint, modes[(int)type], 0, 0);
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

Response selectable_text(Context& ctx, std::string_view label, std::string_view text) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    int lines = 1;
    for (char c : text) lines += c == '\n';
    const float lh = measure_text(ctx, t.font_body, "M").y;
    const float h = (float)lines * lh + t.padding;
    extend_height(s, r, h);
    r.h = h;
    TextBuffer b{const_cast<char*>(text.data()), (int)text.size() + 1, (int)text.size()};  // never written
    return textbox_impl(ctx, id, r, b, nullptr, TextMulti | TextReadOnly, 0, 0);
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
