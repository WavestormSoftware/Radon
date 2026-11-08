// Copyright (c) 2026 Wavestorm Software
#include "harness.hpp"

using namespace rd;

TEST_CASE("utf8_decode") {
    auto dec = [](const char* s, int n, uint32_t& cp) { return utf8_decode(s, s + n, cp); };
    uint32_t cp;
    CHECK(dec("A", 1, cp) == 1);
    CHECK(cp == 0x41);
    CHECK(dec("\xC3\xA9", 2, cp) == 2);
    CHECK(cp == 0xE9);
    CHECK(dec("\xE2\x82\xAC", 3, cp) == 3);
    CHECK(cp == 0x20AC);
    CHECK(dec("\xF0\x9F\x98\x80", 4, cp) == 4);
    CHECK(cp == 0x1F600);
    CHECK(dec("\xFF", 1, cp) == 1);  // invalid lead
    CHECK(cp == 0xFFFD);
    CHECK(dec("\xE2\x82", 2, cp) == 1);  // truncated
    CHECK(cp == 0xFFFD);
    CHECK(dec("\xC0\x80", 2, cp) == 1);  // overlong
    CHECK(cp == 0xFFFD);
    CHECK(dec("\xED\xA0\x80", 3, cp) == 1);  // surrogate
    CHECK(cp == 0xFFFD);
    CHECK(dec("\xC3(", 2, cp) == 1);  // bad continuation
}

namespace {
float ten(void*, uint32_t) {
    return 10;
}
std::string join(std::string_view t, const TextLine* l, int n) {
    std::string out;
    for (int i = 0; i < n; i++) out += "[" + std::string(t.substr(l[i].start, l[i].length)) + "]";
    return out;
}
}  // namespace

TEST_CASE("wrapping at spaces") {
    TextLine l[8];
    std::string_view t = "hello world foo";
    int n = detail::wrap_with(t, 60, ten, nullptr, l, 8);
    CHECK(join(t, l, n) == "[hello][world][foo]");
    t = "ab cd";
    n = detail::wrap_with(t, 100, ten, nullptr, l, 8);
    CHECK(join(t, l, n) == "[ab cd]");
}

TEST_CASE("wrapping breaks long words and honours newlines") {
    TextLine l[8];
    std::string_view t = "abcdefghij";
    int n = detail::wrap_with(t, 40, ten, nullptr, l, 8);
    CHECK(join(t, l, n) == "[abcd][efgh][ij]");
    t = "ab\ncd\n\nef";
    n = detail::wrap_with(t, 100, ten, nullptr, l, 8);
    CHECK(join(t, l, n) == "[ab][cd][][ef]");
}

TEST_CASE("wrapping breaks on code point boundaries") {
    TextLine l[8];
    std::string_view t = "\xC3\xA9\xC3\xA9\xC3\xA9";  // three 2-byte code points
    int n = detail::wrap_with(t, 20, ten, nullptr, l, 8);
    REQUIRE(n == 2);
    CHECK(l[0].length == 4);
    CHECK(l[1].start == 4);
    CHECK(l[1].length == 2);
}

TEST_CASE("wrapping clamps to max_lines") {
    TextLine l[2];
    int n = detail::wrap_with("a b c d e f", 10, ten, nullptr, l, 2);
    CHECK(n == 2);
}

TEST_CASE("font: measurement of known strings") {
    const auto& f = th::font_bytes();
    if (f.empty()) {
        MESSAGE("no font found (set RD_TEST_FONT or add examples/assets/font.ttf): skipped");
        return;
    }
    th::Harness h;
    FontHandle font = h.ctx.load_font(f.data(), f.size(), 20);
    REQUIRE(font >= 0);
    Vec2 e = measure_text(h.ctx, font, "");
    CHECK(e.x == 0);
    CHECK(e.y > 15);
    CHECK(e.y < 40);
    float m = measure_text(h.ctx, font, "M").x;
    float sp = measure_text(h.ctx, font, " ").x;
    CHECK(m > 8);
    CHECK(sp > 0);
    CHECK(measure_text(h.ctx, font, "M M").x == doctest::Approx(2 * m + sp).epsilon(0.05));
    // an invalid byte measures as one replacement glyph, not zero
    CHECK(measure_text(h.ctx, font, "\xFF").x > 0);
    // wrapping with the real font: a width of 3 M's fits "MMM" per line
    TextLine l[8];
    int n = wrap_text(h.ctx, font, "MMMMMM", m * 3.5f, l, 8);
    CHECK(n == 2);
}

TEST_CASE("font: rasterized glyphs land inside the atlas") {
    const auto& f = th::font_bytes();
    if (f.empty()) {
        MESSAGE("no font found: skipped");
        return;
    }
    th::Harness h;
    FontHandle font = h.ctx.load_font(f.data(), f.size(), 24);
    REQUIRE(font >= 0);
    Theme t = Theme::dark();
    t.font_body = t.font_heading = font;
    h.ctx.set_theme(t);
    uint32_t v0 = h.ctx.atlas_version();
    Rect r{0, 0, 300, 100};
    h.frame([&] {
        if (begin_window(h.ctx, "w", r, WindowNone)) {
            label(h.ctx, "Hi");
            end_window(h.ctx);
        }
    });
    CHECK(h.ctx.atlas_version() != v0);
    const DrawList& dl = h.ctx.draw_list();
    int glyph_quads = 0, ink = 0;
    const int N = h.ctx.atlas_size();
    for (int i = 0; i < dl.vertex_count(); i += 4) {
        const DrawVertex& v = dl.vertices()[i];
        if (v.shape.z != (float)ShapeGlyph) continue;
        glyph_quads++;
        const DrawVertex& br = dl.vertices()[i + 2];
        CHECK(v.uv.x >= 0);
        CHECK(br.uv.x <= 1);
        CHECK(v.uv.x < br.uv.x);
        CHECK(v.uv.y < br.uv.y);
        for (int y = (int)(v.uv.y * N); y < (int)(br.uv.y * N); y++)
            for (int x = (int)(v.uv.x * N); x < (int)(br.uv.x * N); x++)
                if (h.ctx.atlas_pixels()[y * N + x] > 128) ink++;
    }
    CHECK(glyph_quads == 2);
    CHECK(ink > 20);
}

TEST_CASE("atlas overflow: never crashes, warns, recovers next frame") {
    const auto& f = th::font_bytes();
    if (f.empty()) {
        MESSAGE("no font found: skipped");
        return;
    }
    Config cfg;
    cfg.atlas_size = 64;
    th::Harness h(cfg);
    FontHandle font = h.ctx.load_font(f.data(), f.size(), 24);
    Theme t = Theme::dark();
    t.font_body = t.font_heading = font;
    h.ctx.set_theme(t);
    Rect r{0, 0, 500, 100};
    auto ui = [&] {
        if (begin_window(h.ctx, "w", r, WindowNone)) {
            label(h.ctx, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
            end_window(h.ctx);
        }
    };
    h.frame(ui);
    CHECK(h.ctx.warning());
    h.frame(ui);  // atlas was cleared; frame must still complete
}
