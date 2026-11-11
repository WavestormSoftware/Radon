// Copyright (c) 2026 Wavestorm Software
#include "harness.hpp"

using namespace rd;

// Every widget in a 300x300 undecorated window; the first row starts at (8, 8), rows are 34 px apart.
namespace {
struct Ui {
    th::Harness h;
    Rect rect{0, 0, 300, 300};
    uint32_t flags = WindowNone;
    std::function<void()> body;
    void frame() {
        h.frame([&] {
            if (begin_window(h.ctx, "w", rect, flags)) {
                body();
                end_window(h.ctx);
            }
        });
    }
    void click(float x, float y) {
        h.move(x, y);
        frame();
        h.in.pointer_down = h.in.pointer_pressed = true;
        frame();
        h.in.pointer_down = false;
        h.in.pointer_released = true;
        frame();
    }
};
}  // namespace

TEST_CASE("button") {
    Ui u;
    Response r;
    u.body = [&] { r = button(u.h.ctx, "b"); };
    u.frame();
    u.click(20, 20);
    CHECK(r.clicked);
}

TEST_CASE("checkbox toggles on click and on Space when focused") {
    Ui u;
    bool v = false;
    Response r;
    u.body = [&] { r = checkbox(u.h.ctx, "c", v); };
    u.frame();
    u.click(20, 20);
    CHECK(v);
    CHECK(r.changed);
    u.h.key(KeySpace);
    u.frame();
    CHECK_FALSE(v);
}

TEST_CASE("slider follows the pointer and clamps") {
    Ui u;
    float v = 0;
    u.body = [&] { slider(u.h.ctx, "s", v, 0, 10); };
    u.frame();
    u.h.move(8 + 284 / 2, 20);  // middle of the widget: the track ends before the value column
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    CHECK(v > 5.0f);
    CHECK(v < 7.0f);
    u.h.in.pointer_pressed = false;
    u.h.move(1000, 20);  // dragging outside keeps tracking and clamps
    u.frame();
    CHECK(v == doctest::Approx(10));
    u.h.move(-50, 20);
    u.frame();
    CHECK(v == doctest::Approx(0));
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
}

TEST_CASE("slider keyboard control") {
    Ui u;
    float v = 5;
    Response r;
    u.body = [&] { r = slider(u.h.ctx, "s", v, 0, 10); };
    u.frame();
    u.h.key(KeyTab);
    u.frame();
    u.h.key(KeyRight);
    u.frame();
    CHECK(v == doctest::Approx(5.5f));
    CHECK(r.changed);
}

TEST_CASE("drag_float changes by pointer movement times speed") {
    Ui u;
    float v = 1;
    u.body = [&] { drag_float(u.h.ctx, "d", v, 0.5f); };
    u.frame();
    u.h.move(50, 20);
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    u.h.in.pointer_pressed = false;
    u.h.move(70, 20);
    u.frame();
    CHECK(v == doctest::Approx(11));
}

TEST_CASE("collapsing_header toggles") {
    Ui u;
    bool open = false;
    u.body = [&] { collapsing_header(u.h.ctx, "h", open); };
    u.frame();
    u.click(20, 20);
    CHECK(open);
    u.click(20, 20);
    CHECK_FALSE(open);
}

TEST_CASE("combo opens a popup and selects an item") {
    Ui u;
    int sel = 0;
    bool changed = false;
    const char* items[] = {"zero", "one", "two"};
    u.body = [&] { changed = combo(u.h.ctx, "c", sel, items, 3) || changed; };
    u.frame();
    u.click(20, 20);  // open
    u.frame();
    u.frame();  // popup rect known
    // popup: below the combo (y = 36 + 2), rows 28 px high with 6 px padding => item 2 at ~y 38+8+68
    u.click(20, 38 + 8 + 2 * 34 + 10);
    CHECK(sel == 2);
    CHECK(changed);
    // popup closed after selection: clicking the same spot again hits nothing
    u.frame();
    u.click(20, 38 + 8 + 10);
    CHECK(sel == 2);
}

TEST_CASE("combo popup closes on outside click") {
    Ui u;
    int sel = 1;
    const char* items[] = {"zero", "one", "two"};
    u.body = [&] { combo(u.h.ctx, "c", sel, items, 3); };
    u.frame();
    u.click(20, 20);
    u.frame();
    u.frame();
    u.click(250, 250);
    u.frame();
    u.click(20, 38 + 8 + 10);  // would have been item 0 if the popup were still open
    CHECK(sel == 1);
}

TEST_CASE("popup_begin / open_popup") {
    Ui u;
    bool inside = false;
    Response b;
    u.body = [&] {
        b = button(u.h.ctx, "open");
        if (b.clicked) open_popup(u.h.ctx, "p");
        inside = false;
        if (popup_begin(u.h.ctx, "p")) {
            inside = true;
            rd::label(u.h.ctx, "hello");
            popup_end(u.h.ctx);
        }
    };
    u.frame();
    CHECK_FALSE(inside);
    u.click(20, 20);
    u.frame();
    CHECK(inside);
    u.click(250, 250);
    u.frame();
    CHECK_FALSE(inside);
}

TEST_CASE("tooltip appears on the overlay layer only while hovered") {
    Ui u;
    u.body = [&] {
        button(u.h.ctx, "b");
        tooltip(u.h.ctx, "tip");
    };
    auto max_layer = [&] {
        uint32_t m = 0;
        for (int i = 0; i < u.h.ctx.draw_list().command_count(); i++)
            m = std::max(m, u.h.ctx.draw_list().commands()[i].layer);
        return m;
    };
    u.frame();
    uint32_t base = max_layer();
    u.h.move(20, 20);
    u.frame();
    CHECK(max_layer() > base);
    u.h.move(250, 250);
    u.frame();
    CHECK(max_layer() == base);
}

TEST_CASE("label, wrapped label, progress, image allocate layout cells") {
    Ui u;
    Rect cells[4];
    u.body = [&] {
        rd::label(u.h.ctx, "l");
        label_wrapped(u.h.ctx, "some words that should wrap");
        progress_bar(u.h.ctx, 0.5f);
        image(u.h.ctx, 5, {40, 60});
        cells[0] = u.h.ctx.draw_list().vertex_count() ? Rect{} : Rect{};
        Rect after = layout_next(u.h.ctx);
        cells[1] = after;
    };
    u.frame();
    // rd::label(34) + wrapped(34, no font => one line, height >= cell) + progress(34) + image(60+6) => next
    // >= 8+34*3+66
    CHECK(cells[1].y >= 8 + 34 * 3 + 66 - 0.01f);
    bool has_image = false;
    for (int i = 0; i < u.h.ctx.draw_list().command_count(); i++)
        if (u.h.ctx.draw_list().commands()[i].texture == 5) has_image = true;
    CHECK(has_image);
}

TEST_CASE("scrolling: wheel and scrollbar") {
    Ui u;
    u.rect = {0, 0, 200, 100};
    u.flags = WindowScrollable;
    int clicked = -1;
    u.body = [&] {
        for (int i = 0; i < 10; i++) {
            char n[8];
            std::snprintf(n, sizeof n, "b%d", i);
            if (button(u.h.ctx, n).clicked) clicked = i;
        }
    };
    u.frame();
    u.frame();
    u.click(20, 20);
    CHECK(clicked == 0);
    u.h.move(20, 50);
    u.frame();
    u.h.in.wheel = -1;  // one line down = cell_height (28)
    u.frame();
    u.frame();
    clicked = -1;
    u.click(20, 20);
    CHECK(clicked == 1);  // button 1 now sits where button 0 was
    // wheel up returns to the top and clamps
    u.h.in.wheel = 10;
    u.frame();
    u.frame();
    clicked = -1;
    u.click(20, 20);
    CHECK(clicked == 0);
}

TEST_CASE("touch drag scrolls the window") {
    Ui u;
    u.rect = {0, 0, 200, 100};
    u.flags = WindowScrollable;
    int clicked = -1;
    u.body = [&] {
        for (int i = 0; i < 10; i++) {
            char n[8];
            std::snprintf(n, sizeof n, "b%d", i);
            if (button(u.h.ctx, n).clicked) clicked = i;
        }
    };
    u.h.in.touch = true;
    u.h.ctx.set_animations(false);  // no flick glide: the scroll is exactly the drag distance
    u.frame();
    u.frame();
    u.h.move(20, 60);
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    u.h.in.pointer_pressed = false;
    u.h.move(20, 30);  // drag up by 30
    u.frame();
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    CHECK(clicked == -1);  // the drag was a scroll, not a tap
    u.frame();
    clicked = -1;
    u.h.move(20, 20);
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.h.in.pointer_released = true;  // a quick tap in one frame
    u.frame();
    CHECK(clicked == 1);  // content moved up by 30 -> second button under (20, 20)
}

TEST_CASE("themes: dark and light differ, accent and background as (Wavestorm palette)") {
    Theme d = Theme::dark(), l = Theme::light();
    CHECK(d.background.r == 0x0b);
    CHECK(d.accent.r == 0xec);
    CHECK(d.accent.g == 0x48);
    CHECK(d.accent.b == 0x99);
    CHECK(Theme::midnight().accent.b == 0xff);
    Theme custom = Theme::with_accent(d, {10, 200, 30, 255});
    CHECK(custom.accent.g == 200);
    CHECK(custom.focus_ring.g == 200);
    CHECK(l.background.r != d.background.r);
    th::Harness h;
    h.ctx.set_theme(l);
    CHECK(h.ctx.theme().background.r == l.background.r);
}

TEST_CASE("pool overflow never crashes") {
    Config cfg;
    cfg.max_windows = 1;
    th::Harness h(cfg);
    Rect a{0, 0, 100, 100}, b{0, 0, 100, 100};
    h.frame([&] {
        if (begin_window(h.ctx, "a", a)) end_window(h.ctx);
        bool ok = begin_window(h.ctx, "b", b);
        CHECK_FALSE(ok);
    });
    CHECK(h.ctx.error());
}

TEST_CASE("viewport: click focuses, drag outside keeps reporting, keys only when focused") {
    Ui u;
    ViewportResponse v;
    u.body = [&] { v = viewport(u.h.ctx, 7, {100, 60}); };
    u.frame();
    CHECK(v.keys == nullptr);
    CHECK(v.rect.w == 100);
    u.h.move(20, 20);
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    CHECK(v.pressed);
    CHECK(v.down);
    CHECK(v.focused);
    CHECK(v.keys != nullptr);
    u.h.in.pointer_pressed = false;
    u.h.move(250, 250);  // drag far outside the viewport
    u.frame();
    CHECK(v.down);
    CHECK(v.local.x == doctest::Approx(250 - v.rect.x));
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    CHECK(v.released);
    u.click(280, 280);  // click elsewhere drops focus
    CHECK(v.keys == nullptr);
}

TEST_CASE("canvas: offsets shapes, clips them, reports pointer") {
    Ui u;
    Canvas c;
    u.body = [&] {
        c = canvas_begin(u.h.ctx, {100, 50}, {10, 10, 10, 255});
        c.circle({10, 10}, 5, {255, 0, 0, 255});
        c.sprite(9, {0, 0, 8, 8});
        canvas_end(u.h.ctx);
    };
    u.h.move(30, 30);
    u.frame();
    CHECK(c.rect.w == 100);
    CHECK(c.local.x == doctest::Approx(30 - c.rect.x));
    bool sprite = false;
    const DrawList& dl = u.h.ctx.draw_list();
    for (int i = 0; i < dl.command_count(); i++)
        if (dl.commands()[i].texture == 9) {
            sprite = true;
            CHECK(dl.commands()[i].clip.w == doctest::Approx(100));  // clipped to the canvas
        }
    CHECK(sprite);
}
