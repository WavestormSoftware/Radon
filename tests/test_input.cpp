// Copyright (c) 2026 Wavestorm Software
#include "harness.hpp"

using namespace rd;

namespace {
struct One {
    th::Harness h;
    Rect rect{0, 0, 300, 200};
    Response r;
    void ui() {
        if (begin_window(h.ctx, "w", rect, WindowNone)) {
            r = button(h.ctx, "go");  // occupies (8,8)-(292,36)
            end_window(h.ctx);
        }
    }
    void frame() { h.frame([&] { ui(); }); }
    void press() { h.press([&] { ui(); }); }
    void release() { h.release([&] { ui(); }); }
};
}  // namespace

TEST_CASE("hover, press and click transitions") {
    One t;
    t.frame();
    CHECK_FALSE(t.r.hovered);
    t.h.move(20, 20);
    t.frame();
    CHECK(t.r.hovered);
    t.press();
    CHECK(t.r.pressed);
    CHECK(t.r.focused);
    CHECK_FALSE(t.r.clicked);
    t.release();
    CHECK(t.r.clicked);
    t.frame();
    CHECK_FALSE(t.r.clicked);
    CHECK_FALSE(t.r.pressed);
}

TEST_CASE("release outside the widget does not click") {
    One t;
    t.frame();
    t.h.move(20, 20);
    t.frame();
    t.press();
    t.h.move(250, 150);
    t.release();
    CHECK_FALSE(t.r.clicked);
}

TEST_CASE("press outside is not a click and drops focus") {
    One t;
    t.frame();
    t.h.move(20, 20);
    t.frame();
    t.press();
    t.release();
    t.frame();
    CHECK(t.r.focused);
    t.h.move(400, 400);
    t.frame();
    t.press();
    t.release();
    CHECK_FALSE(t.r.focused);
}

TEST_CASE("overlapped windows: only the top one is hovered; clicking raises") {
    th::Harness h;
    Rect ra{0, 0, 100, 100}, rb{50, 50, 100, 100};
    Response a, b;
    auto ui = [&] {
        if (begin_window(h.ctx, "A", ra, WindowNone)) {
            layout_row(h.ctx, 1, nullptr, 0, -1);
            a = button(h.ctx, "a");
            end_window(h.ctx);
        }
        if (begin_window(h.ctx, "B", rb, WindowNone)) {
            layout_row(h.ctx, 1, nullptr, 0, -1);
            b = button(h.ctx, "b");
            end_window(h.ctx);
        }
    };
    h.frame(ui);
    h.move(60, 60);  // overlap region
    h.frame(ui);
    CHECK(b.hovered);
    CHECK_FALSE(a.hovered);
    h.click(20, 20, ui);  // A-only area raises A
    h.move(60, 60);
    h.frame(ui);
    h.frame(ui);
    CHECK(a.hovered);
    CHECK_FALSE(b.hovered);
}

TEST_CASE("title bar drags a window; resize grip resizes with a minimum") {
    th::Harness h;
    Rect r{10, 10, 200, 150};
    auto ui = [&] {
        if (begin_window(h.ctx, "w", r, WindowDefault)) end_window(h.ctx);
    };
    h.frame(ui);
    h.move(50, 20);
    h.frame(ui);
    h.press(ui);
    h.move(70, 35);
    h.frame(ui);
    h.release(ui);
    CHECK(r.x == doctest::Approx(30));
    CHECK(r.y == doctest::Approx(25));
    // grip at bottom-right
    h.move(r.x + r.w - 5, r.y + r.h - 5);
    h.frame(ui);
    h.press(ui);
    h.move(r.x + 5, r.y + 5);  // shrink far past the minimum
    h.frame(ui);
    h.release(ui);
    CHECK(r.w == doctest::Approx(80));
    CHECK(r.h == doctest::Approx(60));
}

TEST_CASE("close button closes; open_window reopens") {
    th::Harness h;
    Rect r{10, 10, 200, 150};
    bool shown = true;
    auto ui = [&] {
        shown = begin_window(h.ctx, "w", r, WindowDefault | WindowClosable);
        if (shown) end_window(h.ctx);
    };
    h.frame(ui);
    CHECK(shown);
    h.click(r.x + r.w - 15, r.y + 15, ui);
    h.frame(ui);
    CHECK_FALSE(shown);
    open_window(h.ctx, "w");
    h.frame(ui);
    CHECK(shown);
}

TEST_CASE("Tab / Shift+Tab move focus, Enter activates") {
    th::Harness h;
    Rect r{0, 0, 300, 200};
    Response a, b;
    auto ui = [&] {
        if (begin_window(h.ctx, "w", r, WindowNone)) {
            a = button(h.ctx, "a");
            b = button(h.ctx, "b");
            end_window(h.ctx);
        }
    };
    h.frame(ui);
    h.key(KeyTab);
    h.frame(ui);
    CHECK(a.focused);
    h.key(KeyTab);
    h.frame(ui);
    h.frame(ui);
    CHECK(b.focused);
    CHECK_FALSE(a.focused);
    h.key(KeyTab);  // wraps
    h.frame(ui);
    h.frame(ui);
    CHECK(a.focused);
    h.key(KeyTab, true);  // shift-tab wraps back
    h.frame(ui);
    h.frame(ui);
    CHECK(b.focused);
    h.key(KeyEnter);
    h.frame(ui);
    CHECK(b.clicked);
}

TEST_CASE("touch: no hover, tap clicks, drag past slop does not") {
    One t;
    t.h.in.touch = true;
    t.frame();
    t.h.move(20, 20);
    t.frame();
    CHECK_FALSE(t.r.hovered);
    t.press();
    t.h.move(23, 21);  // inside the 8px slop
    t.release();
    CHECK(t.r.clicked);

    t.h.move(20, 20);
    t.frame();
    t.press();
    t.h.move(20, 80);  // dragged away
    t.frame();
    t.h.move(20, 22);  // back over the button, but the gesture already exceeded the slop
    t.release();
    CHECK_FALSE(t.r.clicked);
}

TEST_CASE("animations are cosmetic: results match with them disabled") {
    for (bool anim : {true, false}) {
        Config cfg;
        cfg.animations = anim;
        th::Harness h(cfg);
        Rect rect{0, 0, 300, 200};
        Response r;
        auto ui = [&] {
            if (begin_window(h.ctx, "w", rect, WindowNone)) {
                r = button(h.ctx, "go");
                end_window(h.ctx);
            }
        };
        h.frame(ui);
        h.click(20, 20, ui);
        CHECK(r.clicked);
    }
}
