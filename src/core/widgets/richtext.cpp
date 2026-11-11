// Copyright (c) 2026 Wavestorm Software
// Rich text (inline markup) and a small Markdown renderer.
#include "../internal.hpp"

namespace rd {
using namespace detail;

namespace detail {
FontHandle bold_font(const Theme& t) {
    return t.font_bold;
}
}  // namespace detail

namespace {

struct IconName {
    const char* name;
    Icon icon;
};
const IconName kIcons[] = {
    {"home", Icon::Home},         {"star", Icon::Star},         {"heart", Icon::Heart},
    {"check", Icon::Check},       {"info", Icon::Info},         {"warning", Icon::Warning},
    {"user", Icon::User},         {"bell", Icon::Bell},         {"search", Icon::Search},
    {"settings", Icon::Settings}, {"trash", Icon::Trash},       {"folder", Icon::Folder},
    {"calendar", Icon::Calendar}, {"clock", Icon::Clock},       {"mail", Icon::Mail},
    {"phone", Icon::Phone},       {"link", Icon::Link},         {"lock", Icon::Lock},
    {"play", Icon::Play},         {"close", Icon::Close},       {"plus", Icon::Plus},
    {"minus", Icon::Minus},       {"pin", Icon::Pin},           {"code", Icon::Code},
    {"sun", Icon::Sun},           {"moon", Icon::Moon},         {"image", Icon::Image},
    {"send", Icon::Send},         {"download", Icon::Download}, {"upload", Icon::Upload},
    {"bookmark", Icon::Bookmark}, {"filter", Icon::Filter},     {"share", Icon::Share},
};

bool icon_named(std::string_view n, Icon& out) {
    for (const IconName& e : kIcons)
        if (n == e.name) {
            out = e.icon;
            return true;
        }
    return false;
}

int hexv(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)(c | 0x20);
    return (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1;
}

// Parses "{#rrggbb}" or "{#rrggbbaa}" at the start of `m`; returns the length used, 0 if not a color.
int parse_color(std::string_view m, Color& c) {
    for (int n : {8, 6}) {
        if ((int)m.size() < n + 3 || m[0] != '{' || m[1] != '#' || m[(size_t)n + 2] != '}') continue;
        uint8_t v[4] = {0, 0, 0, 255};
        bool ok = true;
        for (int i = 0; i < n / 2 && ok; i++) {
            const int a = hexv(m[(size_t)(2 + 2 * i)]), b = hexv(m[(size_t)(3 + 2 * i)]);
            ok = a >= 0 && b >= 0;
            v[i] = (uint8_t)(a * 16 + b);
        }
        if (!ok) continue;
        c = {v[0], v[1], v[2], v[3]};
        return n + 3;
    }
    return 0;
}

// Lays out and draws markup into a column `w` px wide starting at (x, y). Returns the height used.
struct Rich {
    Context& ctx;
    State& s;
    ID base;
    FontHandle font;  // the body font of this block (headings pass a larger one)
    Color color;
    float x0, y0, w;
    RichTextResponse* out;
    bool soft_breaks = false;  // markdown paragraphs: a newline is a space

    Rich(Context& c, State& st, ID b, FontHandle f, Color col, Rect r, RichTextResponse* o, bool soft)
        : ctx(c), s(st), base(b), font(f), color(col), x0(r.x), y0(r.y), w(r.w), out(o), soft_breaks(soft) {}

    float pen_x = 0, pen_y = 0, lh = 0;
    bool bold = false, italic = false, code = false, colored = false;
    Color custom{};
    int link = -1;  // index of the link being drawn
    std::string_view url;

    FontHandle cur_font() const {
        const Theme& t = s.cfg.theme;
        if (code && t.font_mono >= 0) return t.font_mono;
        if (bold && t.font_bold >= 0 && font == t.font_body) return t.font_bold;
        if (italic && t.font_italic >= 0 && font == t.font_body) return t.font_italic;
        return font;
    }
    Color cur_color() const {
        const Theme& t = s.cfg.theme;
        if (link >= 0) return t.accent;
        if (colored) return custom;
        return italic && t.font_italic < 0 ? lerp(color, t.text_muted, 0.35f) : color;
    }
    void newline() {
        pen_x = 0;
        pen_y += lh;
    }
    void word(std::string_view text) {
        const Theme& t = s.cfg.theme;
        const FontHandle f = cur_font();
        const float pad = code ? 3.0f : 0.0f;
        const float ww = text_size(s, f, text).x + 2 * pad;
        if (pen_x > 0 && pen_x + ww > w) newline();
        const Rect r{x0 + pen_x, y0 + pen_y, ww, lh};
        if (code) s.dl.rect({r.x, r.y + 1, r.w, r.h - 2}, t.surface_active, 3);
        const Color c = cur_color();
        Vec2 at{r.x + pad, r.y + (lh - text_size(s, f, "M").y) / 2};
        draw_text(s, f, at, text, c);
        if (bold && (t.font_bold < 0 || font != t.font_body)) draw_text(s, f, {at.x + 0.7f, at.y}, text, c);
        if (link >= 0) {
            const ID lid = hash_u32(base, (uint32_t)link);
            const Response rs = update_control(ctx, lid, r, false, false);
            if (rs.hovered) {
                s.cursor = Cursor::Hand;
                out->hovered = true;
            }
            if (rs.hovered || s.active == lid) s.dl.rect({r.x, r.y + lh - 3, r.w, 1}, c);
            if (rs.clicked) out->clicked = true, out->link = url;
        }
        pen_x += ww;
    }
    void space() {
        if (pen_x <= 0) return;
        pen_x += text_size(s, cur_font(), " ").x;
    }
    // Text without markup: words, spaces and line breaks.
    void plain(std::string_view text) {
        size_t i = 0;
        while (i < text.size()) {
            const char ch = text[i];
            if (ch == '\n') {
                if (soft_breaks)
                    space();
                else
                    newline();
                i++;
            } else if (ch == ' ' || ch == '\t') {
                space();
                i++;
            } else {
                size_t j = i;
                while (j < text.size() && text[j] != ' ' && text[j] != '\n' && text[j] != '\t') j++;
                word(text.substr(i, j - i));
                i = j;
            }
        }
    }
    void icon(Icon ic) {
        const float side = text_size(s, font, "M").y;
        if (pen_x > 0 && pen_x + side > w) newline();
        draw_icon(ctx, ic, {x0 + pen_x, y0 + pen_y + (lh - side) / 2, side, side}, cur_color());
        pen_x += side;
    }
    float run(std::string_view m) {
        lh = text_size(s, font, "M").y;
        if (lh <= 0) lh = s.cfg.theme.cell_height * 0.6f;
        size_t i = 0, start = 0;
        int links = 0;
        auto flush = [&](size_t end) {
            if (end > start) plain(m.substr(start, end - start));
        };
        while (i < m.size()) {
            const char ch = m[i];
            if (ch == '\\' && i + 1 < m.size()) {
                flush(i);
                start = i + 1;
                i += 2;
                continue;
            }
            if (code && ch != '`') {
                i++;
                continue;
            }
            size_t skip = 0;
            if (ch == '*' && i + 1 < m.size() && m[i + 1] == '*') {
                flush(i);
                bold = !bold;
                skip = 2;
            } else if (ch == '*') {
                flush(i);
                italic = !italic;
                skip = 1;
            } else if (ch == '`') {
                flush(i);
                code = !code;
                skip = 1;
            } else if (ch == '[' && link < 0) {
                const size_t mid = m.find("](", i + 1),
                             end = mid == std::string_view::npos ? mid : m.find(')', mid + 2);
                if (end != std::string_view::npos && m.find('\n', i) > mid) {
                    flush(i);
                    link = links++;
                    url = m.substr(mid + 2, end - mid - 2);
                    plain(m.substr(i + 1, mid - i - 1));
                    link = -1;
                    skip = end + 1 - i;
                }
            } else if (ch == '{') {
                Color c;
                if (const int n = parse_color(m.substr(i), c)) {
                    flush(i);
                    custom = c;
                    colored = true;
                    skip = (size_t)n;
                } else if (m.substr(i, 3) == "{/}") {
                    flush(i);
                    colored = false;
                    skip = 3;
                }
            } else if (ch == ':') {
                const size_t e = m.find(':', i + 1);
                Icon ic;
                if (e != std::string_view::npos && e - i <= 12 &&
                    icon_named(m.substr(i + 1, e - i - 1), ic)) {
                    flush(i);
                    icon(ic);
                    skip = e + 1 - i;
                }
            }
            if (skip) {
                i += skip;
                start = i;
            } else {
                i++;
            }
        }
        flush(m.size());
        return pen_y + lh;
    }
};

float rich_block(Context& ctx, Rect& r, std::string_view m, FontHandle font, Color color,
                 RichTextResponse& out, bool soft, ID base) {
    State& s = state(ctx);
    Rich rt(ctx, s, base, font, color, r, &out, soft);
    const float h = rt.run(m);
    extend_height(s, r, h);
    return h;
}

}  // namespace

RichTextResponse rich_text(Context& ctx, std::string_view markup) {
    State& s = state(ctx);
    RichTextResponse out;
    Rect r = layout_next(ctx);
    rich_block(ctx, r, markup, s.cfg.theme.font_body, s.cfg.theme.text, out, false, get_id(s, markup));
    a11y_static(s, A11yRole::Text, r, markup);
    return out;
}

RichTextResponse markdown(Context& ctx, std::string_view text) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    RichTextResponse out;
    const ID base = get_id(s, text);
    const FontHandle mono = t.font_mono >= 0 ? t.font_mono : t.font_body;
    float lh = text_size(s, t.font_body, "M").y;
    if (lh <= 0) lh = t.cell_height * 0.6f;
    auto line_at = [&](size_t i) {
        const size_t e = text.find('\n', i);
        return text.substr(i, (e == std::string_view::npos ? text.size() : e) - i);
    };
    auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
    auto special = [&](std::string_view l) {
        if (l.empty()) return true;
        if (l[0] == '#' || l[0] == '>' || l.substr(0, 3) == "```" || l.substr(0, 3) == "---") return true;
        if (l.size() > 1 && (l[0] == '-' || l[0] == '*' || l[0] == '+') && l[1] == ' ') return true;
        size_t k = 0;
        while (k < l.size() && is_digit(l[k])) k++;
        return k > 0 && k + 1 < l.size() && l[k] == '.' && l[k + 1] == ' ';
    };
    uint32_t block = 0;
    size_t i = 0;
    while (i < text.size()) {
        std::string_view l = line_at(i);
        const size_t next = i + l.size() + 1;
        layout_row(ctx, 1, nullptr, 0, lh + 2);  // blocks grow to fit; plain lines stay tight
        const ID bid = hash_u32(base, block++);
        if (l.empty() || l.find_first_not_of(" \t\r") == std::string_view::npos) {
            spacing(ctx, lh * 0.35f);
            i = next;
            continue;
        }
        if (l.substr(0, 3) == "```") {  // fenced code block, no markup inside
            size_t j = next, end = text.size();
            int n = 0;
            while (j < text.size()) {
                const std::string_view cl = line_at(j);
                if (cl.substr(0, 3) == "```") {
                    end = j;
                    break;
                }
                n++;
                j += cl.size() + 1;
            }
            float mlh = text_size(s, mono, "M").y;
            if (mlh <= 0) mlh = lh;
            Rect r = layout_next(ctx);
            const float h = (float)n * mlh + t.padding;
            extend_height(s, r, h);
            s.dl.rect({r.x, r.y, r.w, h}, t.surface_active, t.corner_radius);
            push_clip(s, {r.x, r.y, r.w, h});
            size_t k = next;
            for (int row = 0; row < n; row++) {
                const std::string_view cl = line_at(k);
                draw_text(s, mono, {r.x + t.padding, r.y + t.padding / 2 + (float)row * mlh}, cl, t.text);
                k += cl.size() + 1;
            }
            pop_clip(s);
            i = end < text.size() ? end + line_at(end).size() + 1 : text.size();
            continue;
        }
        if (l.substr(0, 3) == "---" || l.substr(0, 3) == "***") {
            separator(ctx);
            i = next;
            continue;
        }
        if (l[0] == '#') {
            int level = 0;
            while (level < (int)l.size() && l[(size_t)level] == '#') level++;
            const FontHandle hf = level == 1   ? (t.font_large >= 0 ? t.font_large : t.font_heading)
                                  : level == 2 ? t.font_heading
                                               : (t.font_bold >= 0 ? t.font_bold : t.font_heading);
            Rect r = layout_next(ctx);
            l.remove_prefix(
                std::min(l.size(), (size_t)level + (l.size() > (size_t)level && l[(size_t)level] == ' ')));
            rich_block(ctx, r, l, hf, t.text, out, false, bid);
            a11y_static(s, A11yRole::Heading, r, l);
            i = next;
            continue;
        }
        Rect r = layout_next(ctx);
        size_t k = 0;
        while (k < l.size() && is_digit(l[k])) k++;
        const bool numbered = k > 0 && k + 1 < l.size() && l[k] == '.' && l[k + 1] == ' ';
        const bool bullet = l.size() > 1 && (l[0] == '-' || l[0] == '*' || l[0] == '+') && l[1] == ' ';
        if (bullet || numbered) {
            const float ind = 20;
            if (bullet)
                s.dl.rect({r.x + 7, r.y + lh / 2 - 2.5f, 5, 5}, t.text_muted, 2.5f);
            else
                draw_text(s, t.font_body, {r.x, r.y}, l.substr(0, k + 1), t.text_muted);
            Rect tr{r.x + ind, r.y, r.w - ind, r.h};
            rich_block(ctx, tr, l.substr(numbered ? k + 2 : 2), t.font_body, t.text, out, false, bid);
            i = next;
            continue;
        }
        if (l[0] == '>') {
            l.remove_prefix(l.size() > 1 && l[1] == ' ' ? 2 : 1);
            Rect tr{r.x + 14, r.y, r.w - 14, r.h};
            const float h = rich_block(ctx, tr, l, t.font_body, t.text_muted, out, false, bid);
            s.dl.rect({r.x + 2, r.y, 3, h}, t.accent, 1.5f);
            i = next;
            continue;
        }
        size_t end = next;  // a paragraph runs until a blank or special line
        while (end < text.size() && !special(line_at(end))) end += line_at(end).size() + 1;
        const std::string_view para = text.substr(i, std::min(end, text.size()) - i);
        rich_block(ctx, r, para, t.font_body, t.text, out, true, bid);
        a11y_static(s, A11yRole::Text, r, para);
        i = end;
    }
    return out;
}

}  // namespace rd
