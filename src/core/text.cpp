// Copyright (c) 2026 Wavestorm Software
#include <cstdlib>

#include "internal.hpp"

// stb_truetype allocates while rasterizing; route it through a preallocated arena so the library
// never touches the heap after construction.
static void* rd_arena_alloc(void* user, size_t n);
#define STBTT_malloc(x, u) rd_arena_alloc((u), (x))
#define STBTT_free(x, u) ((void)(x), (void)(u))
#define STB_RECT_PACK_IMPLEMENTATION  // before truetype, or it defines its own public stbrp_* copies
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

static void* rd_arena_alloc(void* user, size_t n) {
    auto* a = static_cast<rd::detail::Arena*>(user);
    if (!a || !a->mem) return nullptr;
    n = (n + 15) & ~size_t(15);
    if (a->used + n > rd::detail::kArenaSize) return nullptr;
    void* p = a->mem + a->used;
    a->used += n;
    return p;
}

namespace rd {

int utf8_decode(const char* p, const char* end, uint32_t& cp) {
    const unsigned char* u = reinterpret_cast<const unsigned char*>(p);
    int avail = static_cast<int>(end - p);
    cp = 0xFFFD;
    if (avail <= 0) return 1;
    unsigned c = u[0];
    int len;
    uint32_t v, min;
    if (c < 0x80) {
        cp = c;
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        len = 2, v = c & 0x1F, min = 0x80;
    } else if ((c & 0xF0) == 0xE0) {
        len = 3, v = c & 0x0F, min = 0x800;
    } else if ((c & 0xF8) == 0xF0) {
        len = 4, v = c & 0x07, min = 0x10000;
    } else {
        return 1;
    }
    if (avail < len) return 1;
    for (int i = 1; i < len; i++) {
        if ((u[i] & 0xC0) != 0x80) return 1;
        v = (v << 6) | (u[i] & 0x3F);
    }
    if (v < min || v > 0x10FFFF || (v >= 0xD800 && v <= 0xDFFF)) return 1;
    cp = v;
    return len;
}

namespace detail {

const FontSlot* font_of(const State& s, FontHandle f) {
    if (f < 0) f = s.default_font;
    return (f >= 0 && f < s.font_n) ? &s.fonts[f] : nullptr;
}

// The font in f's fallback chain that has a glyph for `cp` (f itself when none has), its glyph index
// and its scale for f's pixel size.
static const FontSlot* resolve(const State& s, const FontSlot& f, uint32_t cp, int* gi, float* scale) {
    const FontSlot* cur = &f;
    for (int d = 0; d < 4; d++) {
        const int g = stbtt_FindGlyphIndex(&cur->info, (int)cp);
        if (g) {
            *gi = g;
            *scale = cur == &f ? f.scale : stbtt_ScaleForPixelHeight(&cur->info, f.pixel_size);
            return cur;
        }
        if (cur->fallback < 0 || cur->fallback >= s.font_n) break;
        cur = &s.fonts[cur->fallback];
    }
    *gi = 0;
    *scale = f.scale;
    return &f;
}

static float advance_of(const State& s, const FontSlot& f, uint32_t cp, uint32_t next) {
    int adv, lsb;
    if (f.fallback >= 0) {
        int gi;
        float sc;
        const FontSlot* g = resolve(s, f, cp, &gi, &sc);
        if (g != &f) {
            stbtt_GetGlyphHMetrics(&g->info, gi, &adv, &lsb);
            return adv * sc;
        }
    }
    stbtt_GetCodepointHMetrics(&f.info, (int)cp, &adv, &lsb);
    float a = adv * f.scale;
    if (next) a += stbtt_GetCodepointKernAdvance(&f.info, (int)cp, (int)next) * f.scale;
    return a;
}

Vec2 text_size(const State& s, FontHandle h, std::string_view text) {
    const FontSlot* f = font_of(s, h);
    if (!f) return {0, 0};
    float w = 0;
    const char* p = text.data();
    const char* e = p + text.size();
    while (p < e) {
        uint32_t cp, nx = 0;
        p += utf8_decode(p, e, cp);
        if (p < e) utf8_decode(p, e, nx);
        w += advance_of(s, *f, cp, nx);
    }
    return {w, f->ascent - f->descent + f->line_gap};
}

void reset_atlas(State& s) {
    std::memset(s.atlas.get(), 0, (size_t)s.cfg.atlas_size * s.cfg.atlas_size);
    stbrp_init_target(&s.pack, s.cfg.atlas_size, s.cfg.atlas_size, s.nodes.get(), s.cfg.atlas_size);
    for (int i = 0; i < s.cfg.max_glyphs; i++) s.glyphs[i].used = false;
    s.glyph_n = 0;
    s.atlas_ver++;
}

// Returns the cached glyph, rasterizing it on first use. Null when the atlas is full.
constexpr int kSdfPad = 4;  // distance range in bake pixels

static const Glyph* glyph_for(State& s, FontHandle h, const FontSlot& f, uint32_t cp) {
    const int cap = s.cfg.max_glyphs;
    uint32_t hash = (cp * 2654435761u) ^ ((uint32_t)h * 40503u);
    int i = (int)(hash % (uint32_t)cap);
    for (int n = 0; n < cap; n++, i = (i + 1) % cap) {
        Glyph& g = s.glyphs[i];
        if (!g.used) break;
        if (g.font == h && g.cp == cp) return &g;
    }
    if (s.glyph_n >= cap * 3 / 4) {
        s.atlas_full = true;
        s.warning = true;
        return nullptr;
    }
    // Signed-distance glyphs: baked once at ~2x the font size (DPI-independent) and scaled by the shader.
    const float bake = std::fmin(std::fmax(f.pixel_size * 2.0f, 24.0f), 40.0f);
    const float m = bake / f.pixel_size;  // bake pixels per logical pixel
    int gi;
    float gscale;
    const FontSlot& src = *resolve(s, f, cp, &gi, &gscale);
    const float sc = gscale * m;
    Glyph g;
    g.font = h;
    g.cp = cp;
    g.used = true;
    int adv, lsb;
    stbtt_GetGlyphHMetrics(&src.info, gi, &adv, &lsb);
    g.advance = adv * gscale;
    s.arena.used = 0;
    int bw = 0, bh = 0, ox = 0, oy = 0;
    uint8_t* sdf = stbtt_GetGlyphSDF(&src.info, sc, gi, kSdfPad, 128, 128.0f / kSdfPad, &bw, &bh, &ox, &oy);
    g.xoff = ox / m;
    g.yoff = oy / m;
    g.w = bw / m;
    g.h = bh / m;
    if (sdf && bw > 0 && bh > 0) {
        stbrp_rect rc{};
        rc.w = (stbrp_coord)(bw + 2);
        rc.h = (stbrp_coord)(bh + 2);
        if (!stbrp_pack_rects(&s.pack, &rc, 1) || !rc.was_packed) {
            s.atlas_full = true;
            s.warning = true;
            return nullptr;
        }
        const int stride = s.cfg.atlas_size;
        for (int y = 0; y < bh; y++)
            std::memcpy(s.atlas.get() + (size_t)(rc.y + 1 + y) * stride + rc.x + 1, sdf + (size_t)y * bw, (size_t)bw);
        s.arena.used = 0;
        const float inv = 1.0f / stride;
        g.u0 = (rc.x + 1) * inv;
        g.v0 = (rc.y + 1) * inv;
        g.u1 = (rc.x + 1 + bw) * inv;
        g.v1 = (rc.y + 1 + bh) * inv;
        s.atlas_ver++;
    }
    s.glyphs[i] = g;
    s.glyph_n++;
    return &s.glyphs[i];
}

void draw_text(State& s, FontHandle h, Vec2 pos, std::string_view text, Color c) {
    if (h < 0) h = s.default_font;
    const FontSlot* f = font_of(s, h);
    if (!f || c.a == 0) return;
    const float dpi = s.cfg.dpi_scale;
    // Subpixel x; the baseline snaps to a device pixel so horizontal stems stay crisp.
    const float base = std::floor((pos.y + f->ascent) * dpi + 0.5f) / dpi;
    float pen = pos.x;
    const char* p = text.data();
    const char* e = p + text.size();
    while (p < e) {
        uint32_t cp, nx = 0;
        p += utf8_decode(p, e, cp);
        if (p < e) utf8_decode(p, e, nx);
        const Glyph* g = glyph_for(s, h, *f, cp);
        if (g && g->w > 0 && g->h > 0)
            s.dl.glyph({pen + g->xoff, base + g->yoff, g->w, g->h}, {g->u0, g->v0, g->u1, g->v1},
                       c);
        pen += advance_of(s, *f, cp, nx);
    }
}

void draw_text_centered(State& s, FontHandle h, const Rect& r, std::string_view text, Color c,
                        bool center_x) {
    Vec2 sz = text_size(s, h, text);
    float x = center_x ? r.x + (r.w - sz.x) / 2 : r.x;
    draw_text(s, h, {x, r.y + (r.h - sz.y) / 2}, text, c);
}

float x_of_index(const State& s, FontHandle h, std::string_view text, int idx) {
    if (idx > (int)text.size()) idx = (int)text.size();
    return text_size(s, h, text.substr(0, idx)).x;
}

int index_at_x(const State& s, FontHandle h, std::string_view text, float x) {
    const FontSlot* f = font_of(s, h);
    if (!f || x <= 0) return 0;
    const char* b = text.data();
    const char* p = b;
    const char* e = b + text.size();
    float pen = 0;
    while (p < e) {
        uint32_t cp, nx = 0;
        int len = utf8_decode(p, e, cp);
        if (p + len < e) utf8_decode(p + len, e, nx);
        float a = advance_of(s, *f, cp, nx);
        if (x < pen + a / 2) return (int)(p - b);
        pen += a;
        p += len;
    }
    return (int)text.size();
}

}  // namespace detail

Vec2 measure_text(const Context& ctx, FontHandle font, std::string_view text) {
    const auto& s = detail::state(ctx);
    Vec2 v = detail::text_size(s, font, text);
    if (!detail::font_of(s, font)) v.y = s.cfg.theme.cell_height;
    return v;
}

namespace detail {

int wrap_with(std::string_view text, float max_w, AdvanceFn adv, void* user, TextLine* lines, int max_lines) {
    int n = 0;
    auto emit = [&](int start, int len) {
        if (n < max_lines && lines) lines[n] = TextLine{start, len};
        n++;
    };
    const char* b = text.data();
    const char* e = b + text.size();
    const char* p = b;
    int line_start = 0;
    float w = 0;
    int last_space = -1;  // byte index of the last space on this line
    while (p < e) {
        uint32_t cp;
        int len = utf8_decode(p, e, cp);
        int at = (int)(p - b);
        if (cp == '\n') {
            emit(line_start, at - line_start);
            line_start = at + 1;
            w = 0;
            last_space = -1;
            p += len;
            continue;
        }
        float a = adv(user, cp);
        if (w + a > max_w && at > line_start && cp != ' ') {
            if (last_space >= line_start) {
                emit(line_start, last_space - line_start);
                line_start = last_space + 1;
                // re-measure the carried-over part of the word
                w = 0;
                for (const char* q = b + line_start; q < p;) {
                    uint32_t c2;
                    q += utf8_decode(q, e, c2);
                    w += adv(user, c2);
                }
            } else {
                emit(line_start, at - line_start);
                line_start = at;
                w = 0;
            }
            last_space = -1;
        }
        if (cp == ' ') {
            last_space = at;
        }
        w += a;
        p += len;
    }
    emit(line_start, (int)text.size() - line_start);
    return n < max_lines ? n : max_lines;
}

}  // namespace detail

int wrap_text(const Context& ctx, FontHandle font, std::string_view text, float max_width, TextLine* lines,
              int max_lines) {
    struct Env {
        const detail::State* s;
        const detail::FontSlot* f;
    } env{&detail::state(ctx), detail::font_of(detail::state(ctx), font)};
    if (!env.f) {
        if (max_lines > 0 && lines) lines[0] = TextLine{0, (int)text.size()};
        return max_lines > 0 ? 1 : 0;
    }
    return detail::wrap_with(
        text, max_width,
        [](void* u, uint32_t cp) {
            const Env& e = *static_cast<Env*>(u);
            return detail::advance_of(*e.s, *e.f, cp, 0);
        },
        &env, lines, max_lines);
}

}  // namespace rd
