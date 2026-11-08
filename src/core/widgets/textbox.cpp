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
    if (e) *e = TextEdit{id, 0, 0, 0, s.frame};
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

Response textbox(Context& ctx, std::string_view label, TextBuffer& buf, const char* hint) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    ID id = get_id(s, label);
    Rect r = layout_next(ctx);
    Response rs = update_control(ctx, id, r, true, true);
    if (!buf.data || buf.capacity < 1) {
        s.error = true;
        draw_frame(s, r, t.background, false);
        return rs;
    }
    if (buf.length < 0) buf.length = 0;
    if (buf.length > buf.capacity - 1) buf.length = buf.capacity - 1;
    buf.data[buf.length] = 0;

    const Rect ir{r.x + t.padding, r.y, r.w - 2 * t.padding, r.h};
    const FontHandle fh = t.font_body;
    TextEdit* ed = rs.focused ? find_edit(s, id) : nullptr;
    if (rs.focused && !ed) s.error = true;
    if (ed) {
        ed->last_frame = s.frame;
        ed->cursor = ed->cursor > buf.length ? buf.length : ed->cursor;
        ed->anchor = ed->anchor > buf.length ? buf.length : ed->anchor;
        Editor ep{buf, *ed};
        auto text = [&] { return std::string_view(buf.data, (size_t)buf.length); };

        if (rs.pressed || (s.active == id && s.in.pointer_down)) {
            int i = index_at_x(s, fh, text(), s.in.pointer.x - ir.x + ed->scroll);
            ep.move(i, !rs.pressed);
            s.blink = 0;
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
                case KeyHome:
                    ep.move(0, ke.shift);
                    break;
                case KeyEnd:
                    ep.move(buf.length, ke.shift);
                    break;
                case KeyEnter:
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
                    if (ke.ctrl && ep.has_sel() && s.clip_set) {
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
                            if ((unsigned char)tmp[i] >= 0x20 && tmp[i] != 0x7f) tmp[m++] = tmp[i];
                        ep.insert(tmp, m);
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
        const float cx = x_of_index(s, fh, text(), ed->cursor);
        if (cx - ed->scroll > ir.w - 2) ed->scroll = cx - ir.w + 2;
        if (cx - ed->scroll < 0) ed->scroll = cx;
        if (ed->scroll < 0 || text_size(s, fh, text()).x < ir.w) ed->scroll = 0;
        s.wants_text_next = true;
    }

    draw_frame(s, r, t.background, rs.focused, true);
    const std::string_view text{buf.data, (size_t)buf.length};
    const float scroll = ed ? ed->scroll : 0;
    Vec2 sz = text_size(s, fh, text);
    if (sz.y <= 0) sz.y = ir.h * 0.6f;  // no font loaded: still draw a caret and selection
    const float ty = ir.y + (ir.h - sz.y) / 2;
    push_clip(s, ir);
    if (ed && ed->cursor != ed->anchor) {
        int a = ed->cursor < ed->anchor ? ed->cursor : ed->anchor;
        int b = ed->cursor < ed->anchor ? ed->anchor : ed->cursor;
        float x0 = ir.x - scroll + x_of_index(s, fh, text, a);
        float x1 = ir.x - scroll + x_of_index(s, fh, text, b);
        s.dl.rect({x0, ty, x1 - x0, sz.y}, with_alpha(t.accent, 0.4f));
    }
    if (buf.length == 0 && hint) draw_text(s, fh, {ir.x, ty}, hint, t.text_muted);
    draw_text(s, fh, {ir.x - scroll, ty}, text, t.text);
    if (ed && std::fmod(s.blink, 1.0f) < 0.5f) {
        float cx = ir.x - scroll + x_of_index(s, fh, text, ed->cursor);
        s.dl.rect({cx, ty, 1.5f, sz.y}, t.text);
    }
    pop_clip(s);
    return rs;
}

}  // namespace rd
