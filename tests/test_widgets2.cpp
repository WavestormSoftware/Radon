// Copyright (c) 2026 Wavestorm Software
// Tests for the second widget family: radios, toggles, numeric fields, tabs, lists, trees, menus,
// modals, color picker, plots, icons and scoped themes.
#include "harness.hpp"

using namespace rd;

namespace {
struct Ui {
    th::Harness h;
    Rect rect{0, 0, 300, 300};
    uint32_t flags = WindowNone;
    std::function<void()> body;
    Ui() { h.in.screen_size = {300, 300}; }
    void frame() {
        h.frame([&] {
            if (begin_window(h.ctx, "w", rect, flags)) {
                body();
                end_window(h.ctx);
            }
        });
    }
    void frames(int n) {
        for (int i = 0; i < n; i++) frame();
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

TEST_CASE("button styles all click") {
    for (ButtonStyle st :
         {ButtonStyle::Default, ButtonStyle::Primary, ButtonStyle::Ghost, ButtonStyle::Danger}) {
        Ui u;
        Response r;
        u.body = [&] { r = button(u.h.ctx, "b", st); };
        u.frame();
        u.click(20, 20);
        CHECK(r.clicked);
    }
}

TEST_CASE("radio buttons select exclusively") {
    Ui u;
    int v = 0;
    Response r1;
    u.body = [&] {
        radio_button(u.h.ctx, "zero", v, 0);
        r1 = radio_button(u.h.ctx, "one", v, 1);
    };
    u.frame();
    u.click(20, 55);
    CHECK(v == 1);
    CHECK(r1.changed);
    u.click(20, 20);
    CHECK(v == 0);
}

TEST_CASE("toggle flips on click and Space") {
    Ui u;
    bool v = false;
    u.body = [&] { toggle(u.h.ctx, "t", v); };
    u.frame();
    u.click(15, 20);
    CHECK(v);
    u.h.key(KeySpace);
    u.frame();
    CHECK_FALSE(v);
}

TEST_CASE("slider_int and drag_int produce integers") {
    Ui u;
    int a = 0, b = 0;
    Response ra;
    u.body = [&] {
        ra = slider_int(u.h.ctx, "a", a, 0, 10);
        drag_int(u.h.ctx, "b", b, 0.25f);
    };
    u.frame();
    u.h.move(8 + 142, 20);
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    CHECK(a == 5);
    CHECK(ra.changed);
    u.h.in.pointer_pressed = false;
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    // drag_int: 40 px at 0.25 per px = +10, accumulated across frames
    u.h.move(50, 55);
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    u.h.in.pointer_pressed = false;
    for (int i = 0; i < 8; i++) {
        u.h.move(50 + 5.0f * (float)(i + 1), 55);
        u.frame();
    }
    CHECK(b == 10);
}

TEST_CASE("stepper buttons and clamping") {
    Ui u;
    float v = 1;
    u.body = [&] { stepper(u.h.ctx, "s", v, 0.5f, 0, 2); };
    u.frame();
    u.click(270, 20);
    CHECK(v == doctest::Approx(1.5f));
    u.click(270, 20);
    u.click(270, 20);
    CHECK(v == doctest::Approx(2.0f));  // clamped
    u.click(15, 20);
    CHECK(v == doctest::Approx(1.5f));
}

TEST_CASE("tab_bar selects a tab") {
    Ui u;
    int sel = 0;
    bool changed = false;
    const char* tabs[] = {"A", "B", "C"};
    u.body = [&] { changed = tab_bar(u.h.ctx, "tabs", sel, tabs, 3) || changed; };
    u.frame();
    u.click(30, 20);  // no font: each tab is 16 px wide (2 x padding)
    CHECK(sel == 1);
    CHECK(changed);
}

TEST_CASE("listbox selects and scrolls") {
    Ui u;
    int sel = -1;
    const char* items[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    u.body = [&] { listbox(u.h.ctx, "list", sel, items, 10, 100); };
    u.frames(3);
    u.click(30, 20);
    CHECK(sel == 0);
    u.click(30, 60);
    CHECK(sel == 1);
    u.h.move(30, 50);
    u.frame();
    u.h.in.wheel = -1;  // one line = 28 px
    u.frame();
    u.frames(2);
    u.click(30, 20);
    CHECK(sel == 1);  // item 1 now sits at the top (12 - 28 + 34 = 18 .. 46)
}

TEST_CASE("child regions nest and keep the layout balanced") {
    Ui u;
    bool in = false;
    u.body = [&] {
        in = begin_child(u.h.ctx, "c", 80);
        if (in) {
            button(u.h.ctx, "x");
            end_child(u.h.ctx);
        }
        button(u.h.ctx, "after");
    };
    u.frames(2);
    CHECK(in);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("tree_node indents its children") {
    Ui u;
    bool open = false;
    Response inner;
    u.body = [&] {
        if (tree_node(u.h.ctx, "node", open)) {
            inner = button(u.h.ctx, "child");
            tree_pop(u.h.ctx);
        }
    };
    u.frame();
    u.click(20, 20);
    CHECK(open);
    u.frames(2);
    u.click(10, 55);  // left of the indented button
    CHECK_FALSE(inner.clicked);
    u.click(30, 55);
    CHECK(inner.clicked);
}

TEST_CASE("menus open from the bar, switch by hover, and choose items") {
    Ui u;
    bool chosen = false, other = false;
    u.body = [&] {
        begin_menu_bar(u.h.ctx);
        if (begin_menu(u.h.ctx, "File")) {
            chosen = menu_item(u.h.ctx, "Open", "Ctrl+O") || chosen;
            end_menu(u.h.ctx);
        }
        if (begin_menu(u.h.ctx, "Edit")) {
            other = menu_item(u.h.ctx, "Undo") || other;
            end_menu(u.h.ctx);
        }
        end_menu_bar(u.h.ctx);
    };
    u.frame();
    u.click(15, 20);  // "File" spans x 10..26
    u.frames(2);
    u.click(30, 60);  // first item
    CHECK(chosen);
    CHECK_FALSE(other);
    u.click(35, 20);  // "Edit" spans x 26..42
    u.frames(2);
    u.click(40, 60);
    CHECK(other);
}

TEST_CASE("modal dims and blocks the content behind it") {
    Ui u;
    Response behind, inside;
    bool shown = false, want_open = false;
    u.body = [&] {
        behind = button(u.h.ctx, "behind");
        if (want_open) open_modal(u.h.ctx, "dlg");
        want_open = false;
        shown = modal_begin(u.h.ctx, "dlg", "Dialog", 200);
        if (shown) {
            inside = button(u.h.ctx, "ok");
            modal_end(u.h.ctx);
        }
    };
    u.frame();
    CHECK_FALSE(shown);
    want_open = true;  // open_modal must be called in the same ID scope as modal_begin
    u.frames(3);
    CHECK(shown);
    u.click(20, 20);
    CHECK_FALSE(behind.clicked);
    u.click(150, 165);
    CHECK(inside.clicked);
    u.h.key(KeyEscape);
    u.frames(2);
    CHECK_FALSE(shown);
    u.click(20, 20);
    CHECK(behind.clicked);
}

TEST_CASE("color picker edits by dragging and keeps outside values") {
    Ui u;
    Color c{255, 0, 0, 255};
    bool changed = false;
    u.body = [&] { changed = color_picker(u.h.ctx, "c", c) || changed; };
    u.frames(2);
    CHECK_FALSE(changed);
    CHECK(c.r == 255);
    CHECK(c.g == 0);
    u.h.move(8 + 116, 8);  // saturation 0.5, value 1
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    CHECK(changed);
    CHECK(c.r == 255);
    CHECK(c.g > 120);
    CHECK(c.g < 140);
    u.h.in.pointer_pressed = false;
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    // change from outside is respected
    c = {10, 20, 30, 255};
    u.frame();
    CHECK(c.r == 10);
    CHECK(c.b == 30);
}

TEST_CASE("color picker hue bar and alpha bar") {
    Ui u;
    Color c{255, 0, 0, 255};
    u.body = [&] { color_picker(u.h.ctx, "c", c, true); };
    u.frames(2);
    u.h.move(8 + 232 + 8 + 9, 8 + 140 * 0.5f);  // middle of the hue bar: cyan
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    CHECK(c.r < 30);
    CHECK(c.g > 200);
    u.h.in.pointer_pressed = false;
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    u.h.move(8 + 232 + 8 + 26 + 9, 8 + 140 * 0.5f);  // alpha bar, halfway
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    CHECK(c.a > 110);
    CHECK(c.a < 145);
}

TEST_CASE("display widgets and plots draw and allocate cells") {
    Ui u;
    float data[] = {1, 3, 2, 5, 4};
    Rect after;
    u.body = [&] {
        label_colored(u.h.ctx, "red", {255, 0, 0, 255});
        badge(u.h.ctx, "new", {0, 128, 255, 255});
        bullet_text(u.h.ctx, "point");
        separator_text(u.h.ctx, "Group");
        spinner(u.h.ctx);
        plot_lines(u.h.ctx, data, 5, 0, 0, 60);
        plot_histogram(u.h.ctx, data, 5, 0, 6, 40);
        plot_lines(u.h.ctx, nullptr, 0, 0, 1);
        after = layout_next(u.h.ctx);
    };
    u.frame();
    // 5 rows of 28 + 6 spacing, then plots of 60, 40 and 48 (+6 each) => next row at 8 + 170 + 66 + 46 + 54
    CHECK(after.y >= 8 + 5 * 34 + 66 + 46 + 54 - 0.01f);
    CHECK(u.h.ctx.draw_list().vertex_count() > 100);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("every icon draws something and icon buttons click") {
    Ui u;
    int n = 0;
    Response r;
    u.body = [&] { r = icon_button(u.h.ctx, "btn", Icon::Settings); };
    u.frame();
    u.click(20, 20);
    CHECK(r.clicked);
    for (int i = (int)Icon::Plus; i <= (int)Icon::Redo; i++) {
        th::Harness h;
        h.frame([&] { draw_icon(h.ctx, (Icon)i, {0, 0, 32, 32}, {255, 255, 255, 255}); });
        CHECK(h.ctx.draw_list().vertex_count() > 0);
        n++;
    }
    CHECK(n == 22);
}

TEST_CASE("push_theme scopes a theme and unbalanced pushes are reported") {
    th::Harness h;
    Theme pink = Theme::dark(), blue = Theme::midnight();
    h.ctx.set_theme(pink);
    h.frame([&] {
        push_theme(h.ctx, blue);
        CHECK(h.ctx.theme().accent.b == 0xff);
        pop_theme(h.ctx);
        CHECK(h.ctx.theme().accent.b == pink.accent.b);
    });
    CHECK_FALSE(h.ctx.error());
    h.frame([&] { push_theme(h.ctx, blue); });
    CHECK(h.ctx.error());
    CHECK(h.ctx.theme().accent.b == pink.accent.b);  // restored by end_frame
}

TEST_CASE("textbox hint is drawn only while empty") {
    const auto& f = th::font_bytes();
    if (f.empty()) {
        MESSAGE("no font found: skipped");
        return;
    }
    Ui u;
    FontHandle font = u.h.ctx.load_font(f.data(), f.size(), 16);
    Theme t = Theme::dark();
    t.font_body = t.font_heading = font;
    u.h.ctx.set_theme(t);
    char data[16] = "";
    TextBuffer tb{data, sizeof data, 0};
    auto glyphs = [&] {
        int n = 0;
        const DrawList& dl = u.h.ctx.draw_list();
        for (int i = 0; i < dl.vertex_count(); i += 4)
            if (dl.vertices()[i].shape.z == (float)ShapeGlyph) n++;
        return n;
    };
    u.body = [&] { textbox(u.h.ctx, "t", tb, "Search"); };
    u.frame();
    CHECK(glyphs() == 6);
    std::strcpy(data, "ab");
    tb.length = 2;
    u.frame();
    CHECK(glyphs() == 2);
}

TEST_CASE("indent and unindent are symmetric") {
    Ui u;
    float x0 = 0, x1 = 0, x2 = 0;
    u.body = [&] {
        layout_row(u.h.ctx, 1, nullptr, 0, 0);
        x0 = layout_next(u.h.ctx).x;
        indent(u.h.ctx);
        layout_row(u.h.ctx, 1, nullptr, 0, 0);
        x1 = layout_next(u.h.ctx).x;
        unindent(u.h.ctx);
        layout_row(u.h.ctx, 1, nullptr, 0, 0);
        x2 = layout_next(u.h.ctx).x;
    };
    u.frame();
    CHECK(x1 == doctest::Approx(x0 + 16));
    CHECK(x2 == doctest::Approx(x0));
}
