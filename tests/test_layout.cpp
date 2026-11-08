// Copyright (c) 2026 Wavestorm Software
#include "harness.hpp"

using namespace rd;

namespace {
struct Win {
    th::Harness h;
    Rect rect{0, 0, 300, 200};
    template <class F>
    void run(F f) {
        h.frame([&] {
            if (begin_window(h.ctx, "w", rect, WindowNone)) {
                f();
                end_window(h.ctx);
            }
        });
    }
};
}  // namespace

TEST_CASE("fixed + fill widths") {
    Win w;  // body: x=8 (padding), width=284
    w.run([&] {
        const float cols[] = {90, -1};
        layout_row(w.h.ctx, 2, cols, 2, 0);
        Rect a = layout_next(w.h.ctx), b = layout_next(w.h.ctx);
        CHECK(a.x == doctest::Approx(8));
        CHECK(a.w == doctest::Approx(90));
        CHECK(b.x == doctest::Approx(8 + 90 + 6));
        CHECK(b.x + b.w == doctest::Approx(8 + 284));
        CHECK(a.h == doctest::Approx(28));
    });
}

TEST_CASE("fill leaves room for fixed cells after it") {
    Win w;
    w.run([&] {
        const float cols[] = {-60, 50};  // fill, then 50 fixed: -60 leaves 59 px (microui rule)
        layout_row(w.h.ctx, 2, cols, 2, 20);
        Rect a = layout_next(w.h.ctx);
        CHECK(a.w == doctest::Approx(284 - 59));
        CHECK(a.h == doctest::Approx(20));
    });
}

TEST_CASE("rows advance by height + spacing; equal split") {
    Win w;
    w.run([&] {
        layout_row(w.h.ctx, 3, nullptr, 0, 0);
        Rect a = layout_next(w.h.ctx), b = layout_next(w.h.ctx), c = layout_next(w.h.ctx);
        CHECK(a.w == doctest::Approx((284 - 12) / 3.0f));
        CHECK(c.x + c.w == doctest::Approx(8 + 284));
        (void)b;
        Rect d = layout_next(w.h.ctx);  // wraps to the next row
        CHECK(d.x == doctest::Approx(8));
        CHECK(d.y == doctest::Approx(a.y + 28 + 6));
    });
}

TEST_CASE("columns nest and report the tallest child") {
    Win w;
    w.run([&] {
        const float cols[] = {100, -1};
        layout_row(w.h.ctx, 2, cols, 2, 0);
        layout_begin_column(w.h.ctx);
        Rect c1 = layout_next(w.h.ctx);
        Rect c2 = layout_next(w.h.ctx);
        layout_end_column(w.h.ctx);
        Rect right = layout_next(w.h.ctx);
        CHECK(c1.x == doctest::Approx(8));
        CHECK(c2.y == doctest::Approx(c1.y + 28 + 6));
        CHECK(right.x == doctest::Approx(8 + 100 + 6));
        Rect below = layout_next(w.h.ctx);  // next row starts below the 2-cell column
        CHECK(below.y == doctest::Approx(c2.y + 28 + 6));
    });
}

TEST_CASE("spacing and negative height") {
    Win w;
    w.run([&] {
        layout_row(w.h.ctx, 1, nullptr, 0, 0);
        Rect a = layout_next(w.h.ctx);
        spacing(w.h.ctx, 10);
        layout_row(w.h.ctx, 1, nullptr, 0, 0);
        Rect b = layout_next(w.h.ctx);
        CHECK(b.y == doctest::Approx(a.y + 28 + 6 + 10));
        layout_row(w.h.ctx, 1, nullptr, 0, -1);
        Rect c = layout_next(w.h.ctx);
        CHECK(c.y + c.h == doctest::Approx(8 + 184));  // reaches the bottom of the body
    });
}
