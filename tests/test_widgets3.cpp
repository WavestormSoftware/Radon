// Copyright (c) 2026 Wavestorm Software
// Tests the third widget family: text variants, range slider, segmented, rating, calendar, pagination,
// tables, clipper, toasts, drawers, context menus, disabled sections, transitions and touch scrolling.
#include "harness.hpp"

using namespace rd;

namespace {
// One window at the origin: row i spans y = 8 + 34 * i .. + 28, x = 8 .. 292.
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
    float row(int i) const { return 8.0f + 34.0f * (float)i + 14.0f; }
};

int max_alpha(const Context& ctx) {
    int a = 0;
    const DrawList& dl = ctx.draw_list();
    for (int i = 0; i < dl.vertex_count(); i++) a = std::max(a, (int)dl.vertices()[i].color.a);
    return a;
}
}  // namespace

TEST_CASE("disabled sections ignore input and draw faded") {
    Ui u;
    int clicks = 0;
    bool off = true;
    u.body = [&] {
        begin_disabled(u.h.ctx, off);
        if (button(u.h.ctx, "b").clicked) clicks++;
        end_disabled(u.h.ctx);
    };
    u.rect = {0, 0, 0, 0};  // no window background: only the button draws
    u.frame();
    CHECK(max_alpha(u.h.ctx) < 128);
    u.rect = {0, 0, 300, 300};
    u.click(20, u.row(0));
    CHECK(clicks == 0);
    off = false;
    u.click(20, u.row(0));
    CHECK(clicks == 1);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("password box edits but never copies") {
    Ui u;
    char data[32] = "";
    TextBuffer buf{data, sizeof data, 0};
    static int copies;
    copies = 0;
    u.h.ctx.set_clipboard_callbacks(nullptr, [](void*, const char*, int) { copies++; });
    u.body = [&] { password_box(u.h.ctx, "pw", buf); };
    u.click(100, u.row(0));
    u.h.type("secret");
    u.frame();
    CHECK(std::string(data) == "secret");
    u.h.key(KeyA, false, true);
    u.h.key(KeyC, false, true);
    u.frame();
    CHECK(copies == 0);
    CHECK(u.h.ctx.wants_text_input());
    CHECK(u.h.ctx.text_input_rect().w > 0);
}

TEST_CASE("text area: Enter breaks the line, Up/Down/Home/End move by line") {
    Ui u;
    char data[64] = "";
    TextBuffer buf{data, sizeof data, 0};
    Response r;
    u.body = [&] { r = text_area(u.h.ctx, "ta", buf, 90); };
    u.click(100, 30);
    u.h.type("ab");
    u.frame();  // keys are applied before text within one frame, so Enter gets its own frame
    u.h.key(KeyEnter);
    u.frame();
    CHECK_FALSE(r.clicked);  // Enter is a line break here, not a submit
    u.h.type("cd");
    u.frame();
    CHECK(std::string(data) == "ab\ncd");
    u.h.key(KeyUp);  // to line 1
    u.h.key(KeyHome);
    u.frame();
    u.h.type("X");
    u.frame();
    CHECK(std::string(data) == "Xab\ncd");
    u.h.key(KeyDown);
    u.h.key(KeyEnd);
    u.frame();
    u.h.type("Y");
    u.frame();
    CHECK(std::string(data) == "Xab\ncdY");
    u.h.key(KeyHome);
    u.h.key(KeyBackspace);  // joins the lines
    u.frame();
    CHECK(std::string(data) == "XabcdY");
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("search box clear button empties the text") {
    Ui u;
    char data[32] = "hello";
    TextBuffer buf{data, sizeof data, 5};
    Response r;
    bool changed = false;
    u.body = [&] {
        r = search_box(u.h.ctx, "s", buf);
        changed = changed || r.changed;
    };
    u.frame();
    u.click(292 - 14, u.row(0));
    CHECK(buf.length == 0);
    CHECK(changed);
}

TEST_CASE("range slider moves the nearer thumb and never crosses") {
    Ui u;
    float lo = 20, hi = 80;
    u.body = [&] { range_slider(u.h.ctx, "r", lo, hi, 0, 100); };
    u.frame();
    u.h.move(40, u.row(0));  // near the low thumb
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    u.h.in.pointer_pressed = false;
    CHECK(lo < 20);
    CHECK(hi == 80);
    u.h.move(290, u.row(0));  // drag past the high thumb
    u.frame();
    CHECK(lo == 80);
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    u.h.key(KeyTab);
    u.frame();
    u.h.key(KeyRight);
    u.frame();
    CHECK(hi == 85);
}

TEST_CASE("segmented, chip, rating, link and breadcrumbs respond to clicks") {
    Ui u;
    int seg = 0, stars = 0, crumb = -1;
    bool chip_on = false, linked = false;
    const char* items[] = {"A", "B", "C"};
    u.body = [&] {
        segmented(u.h.ctx, "seg", seg, items, 3);
        if (chip(u.h.ctx, "chip", chip_on).clicked) chip_on = !chip_on;
        rating(u.h.ctx, "stars", stars);
        if (link(u.h.ctx, "docs").clicked) linked = true;
        int c = breadcrumbs(u.h.ctx, "bc", items, 3);
        if (c >= 0) crumb = c;
    };
    u.frame();
    u.click(150, u.row(0));  // middle third
    CHECK(seg == 1);
    u.click(12, u.row(1));
    CHECK(chip_on);
    u.click(8 + 28 * 2 + 14, u.row(2));  // third star (28 px each)
    CHECK(stars == 3);
    u.click(8 + 28 * 2 + 14, u.row(2));  // again: clears
    CHECK(stars == 0);
    if (!th::font_bytes().empty()) {
        FontHandle f = u.h.ctx.load_font(th::font_bytes().data(), th::font_bytes().size(), 16);
        u.h.ctx.set_default_font(f);
        u.frame();
        u.click(12, u.row(3));
        CHECK(linked);
        CHECK(u.h.ctx.cursor() == Cursor::Hand);
        u.click(10, u.row(4));
        CHECK(crumb == 0);
    }
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("calendar: month arrows wrap the year and clamp the day; clicking picks a day") {
    Ui u;
    int y = 2023, m = 12, d = 31;
    bool changed = false;
    u.body = [&] { changed = calendar(u.h.ctx, "cal", y, m, d) || changed; };
    u.frame();
    u.click(292 - 14, u.row(0));  // next month
    CHECK(y == 2024);
    CHECK(m == 1);
    u.click(292 - 14, u.row(0));
    CHECK(m == 2);
    CHECK(d == 29);  // 2024 is a leap year
    CHECK(changed);
    // 1 Feb 2024 is a Thursday: column 3 of the first week. Rows: nav, weekdays (19.6 px), weeks.
    const float cw = (284.0f - 6 * 6) / 7, week0 = 8 + 34 + 19.6f + 6 + 14;
    u.click(8 + 3 * (cw + 6) + cw / 2, week0);
    CHECK(d == 1);
    u.click(8 + 0 * (cw + 6) + cw / 2, week0 + 34);  // Monday of week 2 = the 5th
    CHECK(d == 5);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("date picker opens a calendar popup and closes on pick") {
    Ui u;
    int y = 2024, m = 2, d = 10;
    u.body = [&] { date_picker(u.h.ctx, "date", y, m, d); };
    u.frame();
    u.click(100, u.row(0));
    u.frames(2);
    const int before = u.h.ctx.draw_list().vertex_count();
    // popup starts 2 px under the field; its first week row is nav + weekday rows below
    u.click(8 + 8 + 3 * 40.0f + 10, 8 + 28 + 2 + 8 + 34 + 19.6f + 6 + 14);
    CHECK(d != 10);
    u.frames(3);
    CHECK(u.h.ctx.draw_list().vertex_count() < before);  // popup gone
}

TEST_CASE("pagination steps and clamps") {
    Ui u;
    int page = 0;
    bool changed = false;
    u.body = [&] { changed = pagination(u.h.ctx, "p", page, 20); };
    u.frame();
    u.click(8 + 14, u.row(0));  // previous on the first page: nothing
    CHECK(page == 0);
    // on page 1 of 20: prev, 1 2 3 4, "...", 20, next -> next is the 8th cell (28 px + 4 px gap each)
    u.click(8 + 7 * 32 + 14, u.row(0));
    CHECK(page == 1);
    page = 99;
    u.frame();
    CHECK(page == 19);
}

TEST_CASE("pagination: page button 1001 does not share focus with next") {
    Ui u;
    int page = 1002;
    u.body = [&] { pagination(u.h.ctx, "p", page, 3000); };
    u.frame();
    // cells: prev, 1, "...", 1001, 1002, 1003, "...", 3000, next; each 28 px wide plus a 4 px gap
    u.click(8 + 3 * 32 + 14, u.row(0));  // the button for page index 1001
    CHECK(page == 1001);
    u.h.key(KeyEnter);  // the focused page button activates; Next must not fire with it
    u.frame();
    CHECK(page == 1001);
}

TEST_CASE("table header sorts and rows select") {
    Ui u;
    int sort = 0, selected = -1;
    bool asc = true;
    const char* cols[] = {"Name", "Size"};
    u.body = [&] {
        table_header(u.h.ctx, "t", cols, 2, nullptr, &sort, &asc);
        for (int i = 0; i < 3; i++) {
            if (table_row(u.h.ctx, i, selected == i).clicked) selected = i;
            rd::label(u.h.ctx, "file");
            rd::label(u.h.ctx, "1 KB");
        }
    };
    u.frame();
    u.click(200, u.row(0));
    CHECK(sort == 1);
    CHECK(asc);
    u.click(200, u.row(0));
    CHECK_FALSE(asc);
    u.click(100, u.row(2));
    CHECK(selected == 1);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("list clipper emits only the visible rows of a long list") {
    Ui u;
    u.flags = WindowScrollable;
    int emitted = 0, first = 0, last = 0;
    u.body = [&] {
        emitted = 0;
        list_clipper_begin(u.h.ctx, 100000, 0, first, last);
        for (int i = first; i < last; i++) {
            layout_row(u.h.ctx, 1, nullptr, 0, 0);
            rd::label(u.h.ctx, "row");
            emitted++;
        }
        list_clipper_end(u.h.ctx);
    };
    u.frames(2);
    CHECK(first == 0);
    CHECK(emitted > 5);
    CHECK(emitted < 12);
    u.h.move(100, 100);
    u.frame();
    u.h.in.wheel = -100;  // 100 rows down
    u.frame();
    u.frame();
    CHECK(first > 50);
    CHECK(emitted < 12);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("toast shows for its duration, then goes away") {
    Ui u;
    u.rect = {0, 0, 0, 0};
    u.body = [] {};
    u.frame();
    const int empty = u.h.ctx.draw_list().vertex_count();
    toast(u.h.ctx, "Saved", Tone::Success, 0.5f);
    u.frames(5);
    CHECK(u.h.ctx.draw_list().vertex_count() > empty);
    CHECK(u.h.ctx.animating());
    u.frames(40);  // 0.67 s at 60 fps
    CHECK(u.h.ctx.draw_list().vertex_count() == empty);
    CHECK_FALSE(u.h.ctx.animating());
}

TEST_CASE("drawer opens with open_modal and a tap outside dismisses it") {
    Ui u;
    int inside = 0;
    bool open = false;
    u.body = [&] {
        if (button(u.h.ctx, "open").clicked) open_modal(u.h.ctx, "sheet");
        open = drawer_begin(u.h.ctx, "sheet", Side::Bottom, 120);
        if (open) {
            if (button(u.h.ctx, "in").clicked) inside++;
            drawer_end(u.h.ctx);
        }
    };
    u.frame();
    u.click(20, u.row(0));
    u.frames(30);  // slide in
    CHECK(open);
    u.click(20, 300 - 120 + 8 + 14);
    CHECK(inside == 1);
    CHECK(open);
    u.click(20, 50);  // on the dimmed area
    u.frame();
    CHECK_FALSE(open);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("context menu opens on right click and on a touch long-press") {
    Ui u;
    bool open = false;
    int clicks = 0;
    u.body = [&] {
        if (button(u.h.ctx, "target").clicked) clicks++;
        open = context_menu_begin(u.h.ctx, "ctx");
        if (open) {
            menu_item(u.h.ctx, "Copy");
            popup_end(u.h.ctx);
        }
    };
    u.frame();
    u.h.move(50, u.row(0));
    u.frame();
    u.h.in.secondary_pressed = true;
    u.frame();
    u.h.in.secondary_pressed = false;
    CHECK(open);
    u.h.key(KeyEscape);
    u.frame();
    u.frame();
    CHECK_FALSE(open);

    u.h.in.touch = true;
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    u.h.in.pointer_pressed = false;
    u.frames(35);  // held still for more than half a second
    CHECK(open);
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    CHECK(clicks == 0);  // the long-press did not also click
}

TEST_CASE("popups fade in, and the context goes idle when nothing moves") {
    Ui u;
    u.rect = {0, 0, 0, 0};
    u.body = [&] {
        if (popup_begin(u.h.ctx, "p")) {
            rd::label(u.h.ctx, "x");
            popup_end(u.h.ctx);
        }
    };
    u.frame();
    u.h.frame([&] {
        if (begin_window(u.h.ctx, "w", u.rect, 0)) {
            open_popup(u.h.ctx, "p");
            u.body();
            end_window(u.h.ctx);
        }
    });
    const int a0 = max_alpha(u.h.ctx);
    CHECK(a0 < 255);
    CHECK(u.h.ctx.animating());
    u.frames(60);
    CHECK(max_alpha(u.h.ctx) == 255);
    CHECK_FALSE(u.h.ctx.animating());

    Config cfg;
    cfg.animations = false;
    th::Harness still(cfg);
    CHECK(animate(still.ctx, "v", 1.0f) == 1.0f);
}

TEST_CASE("animate eases toward the target and transition restarts on a new key") {
    th::Harness h;
    float v = -1;
    int key = 0;
    Rect r{0, 0, 200, 200};
    auto ui = [&] {
        v = animate(h.ctx, "v", 1.0f, 0.2f);
        if (begin_window(h.ctx, "w", r, WindowNone)) {
            transition_begin(h.ctx, "page", key);
            rd::label(h.ctx, "content");
            transition_end(h.ctx);
            end_window(h.ctx);
        }
    };
    h.frame(ui);
    CHECK(v > 0);
    CHECK(v < 0.5f);
    for (int i = 0; i < 60; i++) h.frame(ui);
    CHECK(v == 1.0f);
    CHECK_FALSE(h.ctx.animating());
    key = 1;
    h.frame(ui);
    CHECK(h.ctx.animating());
    CHECK_FALSE(h.ctx.error());
}

TEST_CASE("a touch flick keeps scrolling after release and then stops") {
    Ui u;
    u.flags = WindowScrollable;
    u.body = [&] {
        for (int i = 0; i < 60; i++) {
            char n[8];
            std::snprintf(n, sizeof n, "b%d", i);
            button(u.h.ctx, n);
        }
    };
    auto scroll = [&] { return th::window_scroll(u.h.ctx, "w"); };
    u.h.in.touch = true;
    u.frames(2);
    u.h.move(20, 200);
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    u.h.in.pointer_pressed = false;
    for (int y = 180; y >= 100; y -= 20) {
        u.h.move(20, (float)y);
        u.frame();
    }
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    const float at_release = scroll();
    CHECK(at_release >= 100);
    u.frames(20);
    CHECK(scroll() > at_release + 50);
    u.frames(600);
    const float rest = scroll();
    u.frames(5);
    CHECK(scroll() == rest);
    CHECK_FALSE(u.h.ctx.animating());
}

TEST_CASE("text cursor over text boxes; touch theme has finger-sized rows; safe rect") {
    Ui u;
    char data[8] = "";
    TextBuffer buf{data, sizeof data, 0};
    u.body = [&] { textbox(u.h.ctx, "t", buf); };
    u.frame();
    u.h.move(100, u.row(0));
    u.frames(2);
    CHECK(u.h.ctx.cursor() == Cursor::Text);
    u.h.move(100, 200);
    u.frames(2);
    CHECK(u.h.ctx.cursor() == Cursor::Arrow);

    const Theme t = Theme::touch(Theme::dark());
    CHECK(t.cell_height >= 44);
    CHECK(Theme::scaled(Theme::dark(), 2).padding == 2 * Theme::dark().padding);

    Input in;
    in.screen_size = {400, 800};
    in.safe_area = {0, 40, 0, 20};
    CHECK(in.safe_rect().y == 40);
    CHECK(in.safe_rect().h == 740);
}

TEST_CASE("card grows to fit its content") {
    Ui u;
    int rows = 1;
    u.body = [&] {
        if (begin_card(u.h.ctx, "card")) {
            for (int i = 0; i < rows; i++) rd::label(u.h.ctx, "line");
            end_card(u.h.ctx);
        }
        button(u.h.ctx, "below");
    };
    u.frames(3);
    const float y1 = th::state(u.h.ctx).last_rect.y;
    rows = 4;
    u.frames(3);
    CHECK(th::state(u.h.ctx).last_rect.y == y1 + 3 * 34);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("nav bar, splitter and display widgets run without errors") {
    Ui u;
    int tab = 0;
    float split = 100;
    const Icon icons[] = {Icon::Home, Icon::Search, Icon::User};
    const char* labels[] = {"Home", "Search", "Me"};
    u.body = [&] {
        layout_row(u.h.ctx, 1, nullptr, 0, 56);
        nav_bar(u.h.ctx, "nav", tab, icons, labels, 3);
        const float cols[] = {split, 8, -1};
        layout_row(u.h.ctx, 3, cols, 3, 0);
        rd::label(u.h.ctx, "left");
        splitter(u.h.ctx, "sp", split, 50, 200);
        rd::label(u.h.ctx, "right");
        layout_row(u.h.ctx, 1, nullptr, 0, 0);
        heading(u.h.ctx, "Heading");
        avatar(u.h.ctx, "Ada Lovelace", {200, 60, 120, 255});
        alert(u.h.ctx, Tone::Warning, "Careful");
        progress_ring(u.h.ctx, 0.6f);
        skeleton(u.h.ctx, 0.7f);
        for (int i = 0; i <= (int)Icon::Share; i++) draw_icon(u.h.ctx, (Icon)i, layout_next(u.h.ctx), {});
    };
    u.frame();
    u.click(250, 8 + 28);
    CHECK(tab == 2);
    u.h.move(8 + 100 + 6 + 4, 8 + 56 + 6 + 14);
    u.frame();
    CHECK(u.h.ctx.cursor() == Cursor::ResizeEW);
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    u.h.in.pointer_pressed = false;
    u.h.move(8 + 100 + 6 + 4 + 30, 8 + 56 + 6 + 14);
    u.frame();
    CHECK(split == 130);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("knob turns with vertical drag, keys and clamps") {
    Ui u;
    float v = 0.5f;
    Response r;
    u.body = [&] { r = knob(u.h.ctx, "k", v, 0, 1); };
    u.frame();
    u.h.move(30, 30);  // inside the two-row knob
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    u.h.in.pointer_pressed = false;
    u.h.move(30, 10);  // up 20 px: +0.1
    u.frame();
    CHECK(v == doctest::Approx(0.6f));
    CHECK(r.changed);
    u.h.move(30, -1000);
    u.frame();
    CHECK(v == doctest::Approx(1.0f));
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    u.h.in.pointer_released = false;
    u.h.key(KeyHome);
    u.frame();
    CHECK(v == doctest::Approx(0.0f));
}

TEST_CASE("steps returns a finished step when clicked, ignores the rest") {
    Ui u;
    static const char* const items[] = {"A", "B", "C"};
    int clicked = -2;
    u.body = [&] {
        const int c = steps(u.h.ctx, "s", items, 3, 1);
        if (c != -1) clicked = c;
    };
    u.frame();
    u.click(8 + 284 * 2 / 3.0f + 5, u.row(0));  // step C: not reached yet
    CHECK(clicked == -2);
    u.click(8 + 10, u.row(0));  // step A: done
    CHECK(clicked == 0);
}

TEST_CASE("indeterminate progress bar keeps animating") {
    Ui u;
    u.body = [&] { progress_bar(u.h.ctx, -1); };
    u.frames(2);
    CHECK(u.h.ctx.animating());
}
