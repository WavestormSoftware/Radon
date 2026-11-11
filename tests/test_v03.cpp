// Copyright (c) 2026 Wavestorm Software
// Tests the 0.3 features: gestures, mobile patterns, text editing, drag and drop, pickers, charts,
// tables, layout containers, docking, rich text and accessibility.
#include "harness.hpp"

using namespace rd;

namespace {
// One window at the origin: row i spans y = 8 + 34 * i .. + 28, x = 8 .. 392.
struct Ui {
    th::Harness h;
    Rect rect{0, 0, 400, 400};
    uint32_t flags = WindowNone;
    std::function<void()> body;
    explicit Ui(const Config& cfg = {}) : h(cfg) { h.in.screen_size = {400, 400}; }
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
    // A one-finger drag from a to b in `steps` frames, then release.
    void drag(Vec2 a, Vec2 b, int steps = 8, bool touch = true, int hold = 0) {
        h.in.touch = touch;
        h.move(a.x, a.y);
        frame();
        h.in.pointer_down = h.in.pointer_pressed = true;
        frame();
        for (int i = 1; i <= steps; i++) {
            const float k = (float)i / (float)steps;
            h.move(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k);
            frame();
        }
        for (int i = 0; i < hold; i++) frame();  // stay still: no fling
        h.in.pointer_down = false;
        h.in.pointer_released = true;
        frame();
    }
    float row(int i) const { return 8.0f + 34.0f * (float)i + 14.0f; }
};
}  // namespace

TEST_CASE("two fingers spreading report a pinch above 1, a flick reports a swipe") {
    th::Harness h;
    h.in.touch = true;
    h.in.pointer_down = h.in.pointer_pressed = true;
    h.in.touch_count = 2;
    h.in.touches[0] = {1, {100, 100}};
    h.in.touches[1] = {2, {200, 100}};
    h.in.pointer = {100, 100};
    h.frame([] {});
    h.in.touches[0] = {1, {90, 100}};
    h.in.touches[1] = {2, {210, 100}};
    h.in.pointer = {90, 100};
    h.frame([] {});
    CHECK(h.ctx.gesture().pinch == doctest::Approx(1.2f));
    CHECK(h.ctx.gesture().fingers == 2);
    CHECK(h.ctx.gesture().center.x == doctest::Approx(150));
    h.in.pointer_down = false;
    h.in.pointer_released = true;
    h.in.touch_count = 0;
    h.frame([] {});

    // fast flick to the left
    h.move(300, 100);
    h.in.pointer_down = h.in.pointer_pressed = true;
    h.frame([] {});
    for (int i = 1; i <= 4; i++) {
        h.move(300 - 40.0f * (float)i, 100);
        h.frame([] {});
    }
    h.in.pointer_down = false;
    h.in.pointer_released = true;
    h.frame([] {});
    CHECK(h.ctx.gesture().swipe == SwipeDir::Left);
}

TEST_CASE("bottom sheet closes when swiped down") {
    th::Harness h;
    h.in.screen_size = {400, 600};
    bool open_it = true;
    auto ui = [&] {
        if (open_it) open_modal(h.ctx, "sheet");
        open_it = false;
        if (drawer_begin(h.ctx, "sheet", Side::Bottom, 300)) {
            layout_row(h.ctx, 1, nullptr, 0, 0);
            rd::label(h.ctx, "content");
            drawer_end(h.ctx);
        }
    };
    for (int i = 0; i < 30; i++) h.frame(ui);  // slide in
    CHECK(h.ctx.can_dismiss());
    h.in.touch = true;
    h.move(200, 400);
    h.frame(ui);
    h.in.pointer_down = h.in.pointer_pressed = true;
    h.frame(ui);
    for (int i = 1; i <= 10; i++) {
        h.move(200, 400 + 15.0f * (float)i);
        h.frame(ui);
    }
    h.in.pointer_down = false;
    h.in.pointer_released = true;
    h.frame(ui);
    h.frame(ui);
    CHECK_FALSE(h.ctx.can_dismiss());
}

TEST_CASE("pull to refresh sets refreshing after a long pull") {
    Ui u;
    u.flags = WindowScrollable;
    bool refreshing = false;
    int started = 0;
    u.body = [&] {
        if (pull_to_refresh(u.h.ctx, "ptr", refreshing)) started++;
        for (int i = 0; i < 3; i++) {
            layout_row(u.h.ctx, 1, nullptr, 0, 0);
            rd::label(u.h.ctx, "item");
        }
    };
    u.drag({200, 50}, {200, 250}, 12);
    CHECK(refreshing);
    CHECK(started == 1);
    refreshing = false;
    u.frames(60);  // springs back
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("swipe_item reveals actions and reports the tapped one") {
    Ui u;
    const SwipeAction acts[] = {{"Delete", Icon::Trash, {220, 40, 60, 255}}};
    int triggered = -1, last = -1;
    u.body = [&] {
        layout_row(u.h.ctx, 1, nullptr, 0, 44);
        swipe_item(u.h.ctx, "row", acts, 1, triggered);
        if (triggered >= 0) last = triggered;
    };
    u.drag({300, 30}, {150, 30}, 10);
    u.frames(30);
    // the action sits at the right end of the row
    u.h.in.touch = true;
    u.click(370, 30);
    CHECK(last == 0);
}

TEST_CASE("edge swipe fires once per drag from the left edge") {
    th::Harness h;
    h.in.screen_size = {400, 600};
    int fired = 0;
    auto ui = [&] {
        if (edge_swipe(h.ctx, Side::Left)) fired++;
    };
    h.in.touch = true;
    h.move(5, 300);
    h.in.pointer_down = h.in.pointer_pressed = true;
    h.frame(ui);
    for (int i = 1; i <= 8; i++) {
        h.move(5 + 12.0f * (float)i, 300);
        h.frame(ui);
    }
    h.in.pointer_down = false;
    h.in.pointer_released = true;
    h.frame(ui);
    CHECK(fired == 1);
}

TEST_CASE("undo and redo restore the text, typing is one step") {
    Ui u;
    char data[64] = "";
    TextBuffer buf{data, sizeof data, 0};
    u.body = [&] { textbox(u.h.ctx, "t", buf); };
    u.click(100, u.row(0));
    u.h.type("hello");
    u.frame();
    u.h.type(" world");
    u.frame();
    CHECK(std::string(data) == "hello world");
    u.h.key(KeyBackspace);
    u.frame();
    CHECK(std::string(data) == "hello worl");
    u.h.key(KeyZ, false, true);
    u.frame();
    CHECK(std::string(data) == "hello world");
    u.h.key(KeyZ, false, true);
    u.frame();
    CHECK(std::string(data) == "");
    u.h.key(KeyY, false, true);
    u.frame();
    CHECK(std::string(data) == "hello world");
    u.h.key(KeyZ, true, true);  // Ctrl+Shift+Z is redo too
    u.frame();
    CHECK(std::string(data) == "hello worl");
}

TEST_CASE("ctrl+backspace and ctrl+arrows work by word") {
    Ui u;
    char data[64] = "";
    TextBuffer buf{data, sizeof data, 0};
    u.body = [&] { textbox(u.h.ctx, "t", buf); };
    u.click(100, u.row(0));
    u.h.type("foo bar baz");
    u.frame();
    u.h.key(KeyBackspace, false, true);
    u.frame();
    CHECK(std::string(data) == "foo bar ");
    u.h.key(KeyLeft, false, true);
    u.h.key(KeyDelete, false, true);
    u.frame();
    CHECK(std::string(data) == "foo  ");  // the word is gone, the spaces around it stay
    u.h.key(KeyHome);
    u.h.key(KeyRight, true, true);  // shift+ctrl+right selects "foo"
    u.h.type("X");
    u.frame();
    CHECK(std::string(data) == "X  ");
}

TEST_CASE("text box reports its keyboard type") {
    Ui u;
    char data[16] = "";
    TextBuffer buf{data, sizeof data, 0};
    u.body = [&] { textbox(u.h.ctx, "n", buf, nullptr, TextInputType::Number); };
    u.click(100, u.row(0));
    CHECK(u.h.ctx.wants_text_input());
    CHECK(u.h.ctx.text_input_type() == TextInputType::Number);
}

TEST_CASE("drag and drop delivers the payload to the target") {
    Ui u;
    int got = -1;
    u.body = [&] {
        layout_row(u.h.ctx, 1, nullptr, 0, 0);
        button(u.h.ctx, "source");
        const int v = 42;
        drag_source(u.h.ctx, "num", &v, sizeof v, "42");
        spacing(u.h.ctx, 40);
        layout_row(u.h.ctx, 1, nullptr, 0, 0);
        button(u.h.ctx, "target");
        int n = 0;
        if (const void* p = drop_target(u.h.ctx, "num", &n)) {
            CHECK(n == (int)sizeof(int));
            std::memcpy(&got, p, sizeof got);
        }
    };
    u.drag({100, u.row(0)}, {100, 8 + 34 + 40 + 14}, 6, false);
    CHECK(got == 42);
    CHECK_FALSE(dragging(u.h.ctx));
}

TEST_CASE("reorder_list moves a row") {
    Ui u;
    int order[] = {0, 1, 2, 3};
    const char* labels[] = {"a", "b", "c", "d"};
    u.body = [&] { reorder_list(u.h.ctx, "list", order, labels, 4); };
    u.drag({100, u.row(0)}, {100, u.row(2) + 14}, 8, false);  // drop "a" between c and d
    CHECK(order[0] == 1);
    CHECK(order[1] == 2);
    CHECK(order[2] == 0);
    CHECK(order[3] == 3);
}

TEST_CASE("wheel picker snaps to the row a drag ends on") {
    Ui u;
    const char* items[] = {"0", "1", "2", "3", "4", "5", "6", "7"};
    int sel = 0;
    u.body = [&] { wheel_picker(u.h.ctx, "wheel", sel, items, 8, 5); };
    u.frames(2);
    // 5 rows of 28 px (y 8..148): the band is the middle row. Drag up by three rows, slowly (no fling).
    u.drag({100, 130}, {100, 130 - 84}, 40, true, 15);
    u.frames(60);
    CHECK(sel == 3);
    u.click(100, 8 + 28 * 3 + 14);  // tap the row below the band
    u.frames(30);
    CHECK(sel == 4);
}

TEST_CASE("time picker shows hour and minute wheels") {
    Ui u;
    int hour = 9, minute = 30;
    u.body = [&] { time_picker(u.h.ctx, "time", hour, minute); };
    u.click(100, u.row(0));
    u.frames(3);
    CHECK_FALSE(u.h.ctx.error());
    CHECK(u.h.ctx.can_dismiss());  // the popup is open
    CHECK(hour == 9);
}

TEST_CASE("autocomplete picks a suggestion with the keyboard") {
    Ui u;
    char data[32] = "";
    TextBuffer buf{data, sizeof data, 0};
    const char* fruit[] = {"apple", "apricot", "banana"};
    u.body = [&] { autocomplete(u.h.ctx, "fruit", buf, fruit, 3); };
    u.click(100, u.row(0));
    u.h.type("ap");
    u.frame();
    u.frame();
    u.h.key(KeyDown);
    u.frame();
    u.h.key(KeyEnter);
    u.frame();
    CHECK(std::string(data) == "apple");
}

TEST_CASE("number pad edits the buffer") {
    Ui u;
    char data[16] = "";
    TextBuffer buf{data, sizeof data, 0};
    u.body = [&] { number_pad(u.h.ctx, "pad", buf, true); };
    const float w = (384 - 2 * 6) / 3.0f, h = 28 * 1.2f;
    auto key = [&](int col, int row) {
        u.click(8 + w * (float)col + w / 2 + 6 * (float)col, 8 + (h + 6) * (float)row + h / 2);
    };
    key(0, 0);  // 1
    key(1, 3);  // 0
    key(0, 3);  // .
    key(0, 3);  // second . ignored
    key(1, 0);  // 2
    CHECK(std::string(data) == "10.2");
    key(2, 3);  // backspace
    CHECK(std::string(data) == "10.");
}

TEST_CASE("combo_search and multi_combo open without errors") {
    Ui u;
    const char* items[] = {"red", "green", "blue"};
    int sel = 0;
    bool multi[] = {true, false, true};
    u.body = [&] {
        combo_search(u.h.ctx, "cs", sel, items, 3);
        multi_combo(u.h.ctx, "mc", multi, items, 3);
    };
    u.click(100, u.row(0));
    u.h.type("bl");
    u.frame();
    u.h.key(KeyEnter);
    u.frame();
    CHECK(sel == 2);
    u.click(100, u.row(1));
    u.frames(2);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("chart axis steps are 1, 2 or 5 times a power of ten") {
    CHECK(detail::nice_step(0.9f) == doctest::Approx(1.0f));
    CHECK(detail::nice_step(2.6f) == doctest::Approx(2.0f));
    CHECK(detail::nice_step(37) == doctest::Approx(50.0f));
    CHECK(detail::nice_step(0.025f) == doctest::Approx(0.02f));
    CHECK(detail::nice_step(0.031f) == doctest::Approx(0.05f));
    char buf[32];
    detail::format_tick(buf, sizeof buf, 2500, 500);
    CHECK(std::string(buf) == "2.5k");
    detail::format_tick(buf, sizeof buf, 0.25f, 0.05f);
    CHECK(std::string(buf) == "0.25");
}

TEST_CASE("charts, pies and sparklines draw inside their cell") {
    Ui u;
    const float a[] = {1, 4, 2, 6, 3}, b[] = {2, 1, 3, 2, 5};
    const ChartSeries series[] = {{"A", a, 5}, {"B", b, 5}};
    const char* xl[] = {"Mon", "Tue", "Wed", "Thu", "Fri"};
    u.body = [&] {
        for (ChartType ty :
             {ChartType::Line, ChartType::Area, ChartType::Bar, ChartType::StackedBar, ChartType::Scatter})
            chart(u.h.ctx, "c", ty, series, 2, xl, 120);
        pie_chart(u.h.ctx, "pie", a, xl, 5, 100, true);
        sparkline(u.h.ctx, a, 5);
    };
    u.h.move(150, 60);
    u.frames(3);
    CHECK_FALSE(u.h.ctx.error());
    CHECK(u.h.ctx.draw_list().vertex_count() > 200);
}

TEST_CASE("table columns resize by dragging the header edge, headers sort") {
    Ui u;
    TableColumn cols[] = {{"Name", 120}, {"Size", 80}, {"Kind", 0}};
    TableSort sort;
    bool open = false;
    u.body = [&] {
        if (table_begin(u.h.ctx, "t", cols, 3, 150, &sort)) {
            table_tree_row(u.h.ctx, 0, "folder", 0, &open);
            rd::label(u.h.ctx, "-");
            rd::label(u.h.ctx, "dir");
            if (open) {
                table_tree_row(u.h.ctx, 1, "file", 1, nullptr);
                rd::label(u.h.ctx, "1 KB");
                rd::label(u.h.ctx, "txt");
            }
            table_end(u.h.ctx);
        }
    };
    // the edge between Name and Size is at x = 8 + 120 + 3
    u.drag({131, u.row(0)}, {171, u.row(0)}, 4, false);
    CHECK(cols[0].width == doctest::Approx(160));
    u.click(8 + 160 + 6 + 20, u.row(0));  // the Size header
    CHECK(sort.column == 1);
    CHECK(sort.ascending);
    u.click(8 + 10, u.row(1));  // the expand arrow of the first row
    CHECK(open);
}

TEST_CASE("flow layout wraps sized widgets, grid picks a column count") {
    Ui u;
    Rect first{}, last{};
    int cols = 0;
    u.body = [&] {
        auto& s = th::state(u.h.ctx);
        flow_begin(u.h.ctx);
        for (int i = 0; i < 20; i++) {
            push_id(u.h.ctx, std::to_string(i));
            button(u.h.ctx, "b");
            pop_id(u.h.ctx);
            if (i == 0) first = s.last_rect;
            last = s.last_rect;
        }
        flow_end(u.h.ctx);
        cols = layout_grid(u.h.ctx, 100);
    };
    u.frames(2);
    CHECK(last.y > first.y);
    CHECK(first.w < 100);
    CHECK(cols == 3);  // (384 + 6) / 106
}

TEST_CASE("carousel snaps to an item after a swipe") {
    Ui u;
    u.body = [&] {
        if (carousel_begin(u.h.ctx, "car", 120, 200)) {
            for (int i = 0; i < 5; i++) {
                push_id(u.h.ctx, std::to_string(i));
                button(u.h.ctx, "card");
                pop_id(u.h.ctx);
            }
            carousel_end(u.h.ctx);
        }
    };
    u.frames(2);
    u.drag({300, 50}, {180, 50}, 30);  // slow drag left by 120 of a 206 px step
    u.frames(60);
    auto& s = th::state(u.h.ctx);
    float scroll = -1;
    for (int i = 0; i < s.cfg.max_children; i++)
        if (s.children[i].id == hash_id(hash_id(0, "w"), "car")) scroll = s.children[i].scroll;
    CHECK(scroll == doctest::Approx(206));
}

TEST_CASE("accordion opens one section at a time") {
    Ui u;
    int open = -1;
    int shown = 0;
    u.body = [&] {
        shown = 0;
        for (int i = 0; i < 3; i++) {
            push_id(u.h.ctx, std::to_string(i));
            if (accordion_begin(u.h.ctx, "section", open, i)) {
                rd::label(u.h.ctx, "body");
                shown++;
                accordion_end(u.h.ctx);
            }
            pop_id(u.h.ctx);
        }
    };
    u.click(100, u.row(1));
    u.frames(30);
    CHECK(open == 1);
    CHECK(shown == 1);
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("dock: side panels split, tabs undock and dock again") {
    th::Harness h;
    h.in.screen_size = {800, 600};
    bool b_open = true;
    int shown_a = 0, shown_b = 0;
    auto ui = [&] {
        dock_space(h.ctx, "dock", {0, 0, 800, 600});
        shown_a = shown_b = 0;
        if (begin_dock_panel(h.ctx, "A")) {
            shown_a++;
            end_dock_panel(h.ctx);
        }
        if (b_open && begin_dock_panel(h.ctx, "B", &b_open, DockPlace::Left, 0.3f)) {
            shown_b++;
            end_dock_panel(h.ctx);
        }
    };
    for (int i = 0; i < 3; i++) h.frame(ui);
    CHECK(shown_a == 1);
    CHECK(shown_b == 1);
    auto& s = th::state(h.ctx);
    auto panel = [&](const char* n) -> detail::DockPanel& {
        for (int i = 0; i < s.cfg.max_dock_panels; i++)
            if (s.dock_panels[i].id == hash_id(0, n)) return s.dock_panels[i];
        return s.dock_panels[0];
    };
    const int bn = panel("B").node;
    REQUIRE(bn >= 0);
    CHECK(s.dock_nodes[bn].rect.x == 0);
    CHECK(s.dock_nodes[bn].rect.w < 300);
    // drag B's tab out: it floats
    h.move(10, 10);
    h.frame(ui);
    h.in.pointer_down = h.in.pointer_pressed = true;
    h.frame(ui);
    for (int i = 1; i <= 5; i++) {
        h.move(10 + 60.0f * (float)i, 10 + 50.0f * (float)i);
        h.frame(ui);
    }
    CHECK(panel("B").node == -1);
    CHECK(shown_b == 1);
    // drop it on the middle of A's area: B becomes a tab next to A
    h.move(500, 300);
    h.frame(ui);
    h.frame(ui);
    h.in.pointer_down = false;
    h.in.pointer_released = true;
    h.frame(ui);
    h.frame(ui);
    CHECK(panel("B").node == panel("A").node);
    CHECK(shown_a + shown_b == 1);  // one tab visible
    // closing B collapses nothing (it shares A's leaf) and leaves A visible
    b_open = false;
    for (int i = 0; i < 3; i++) h.frame(ui);
    CHECK(shown_a == 1);
    CHECK_FALSE(h.ctx.error());
}

TEST_CASE("rich text and markdown lay out and report link clicks") {
    const auto& font = th::font_bytes();
    if (font.empty()) {
        MESSAGE("no font: skipped");
        return;
    }
    Ui u;
    FontHandle f = u.h.ctx.load_font(font.data(), font.size(), 16);
    u.h.ctx.set_default_font(f);
    std::string_view clicked;
    Rect r1{}, r2{};
    u.body = [&] {
        auto& s = th::state(u.h.ctx);
        layout_row(u.h.ctx, 1, nullptr, 0, 0);
        RichTextResponse rr = rich_text(u.h.ctx, "[link](https://x.y) **bold** {#ff0000}red{/} :star:");
        if (!rr.link.empty()) clicked = rr.link;
        r1 = s.last_rect;
        markdown(u.h.ctx,
                 "# Title\n\nSome *text* that is long enough to wrap onto a second line in a narrow "
                 "window.\n\n- one\n- two\n1. first\n> quote\n```\ncode\n```\n---\n");
        layout_row(u.h.ctx, 1, nullptr, 0, 0);
        button(u.h.ctx, "after");
        r2 = s.last_rect;
    };
    u.click(12, u.row(0));
    CHECK(clicked == "https://x.y");
    CHECK(r2.y > r1.y + 150);  // markdown took several rows
    CHECK_FALSE(u.h.ctx.error());
}

TEST_CASE("selectable text never writes to its source") {
    Ui u;
    const std::string text = "line one\nline two";
    u.body = [&] { selectable_text(u.h.ctx, "sel", text); };
    u.click(100, u.row(0));
    u.h.type("zzz");
    u.h.key(KeyBackspace);
    u.frame();
    CHECK(text == "line one\nline two");
    CHECK_FALSE(u.h.ctx.wants_text_input());
}

TEST_CASE("accessibility tree lists widgets and screen readers can press them") {
    Config cfg;
    cfg.a11y = true;
    Ui u(cfg);
    bool on = false;
    int clicks = 0;
    u.body = [&] {
        rd::label(u.h.ctx, "Settings");
        if (button(u.h.ctx, "Save").clicked) clicks++;
        checkbox(u.h.ctx, "Wifi", on);
    };
    u.frame();
    int n = 0;
    const A11yNode* nodes = u.h.ctx.a11y_nodes(&n);
    REQUIRE(n >= 4);  // window? label, button, checkbox
    const A11yNode* save = nullptr;
    const A11yNode* wifi = nullptr;
    bool text = false;
    for (int i = 0; i < n; i++) {
        if (std::string(nodes[i].label) == "Save") save = &nodes[i];
        if (std::string(nodes[i].label) == "Wifi") wifi = &nodes[i];
        text = text || (nodes[i].role == A11yRole::Text && std::string(nodes[i].label) == "Settings");
    }
    CHECK(text);
    REQUIRE(save);
    REQUIRE(wifi);
    CHECK(save->role == A11yRole::Button);
    CHECK(wifi->role == A11yRole::Checkbox);
    CHECK(std::string(wifi->value) == "off");
    const uint32_t wid = wifi->id, sid = save->id;
    u.h.in.a11y_action = A11yAction::Activate;
    u.h.in.a11y_target = wid;
    u.frame();
    u.h.in.a11y_action = A11yAction::None;
    CHECK(on);
    u.h.in.a11y_action = A11yAction::Activate;
    u.h.in.a11y_target = sid;
    u.frame();
    u.h.in.a11y_action = A11yAction::None;
    CHECK(clicks == 1);
}

TEST_CASE("font fallback rejects bad handles") {
    th::Harness h;
    h.ctx.set_font_fallback(0, 1);
    CHECK(h.ctx.error());
}

TEST_CASE("haptics are requested on toggles") {
    Ui u;
    bool v = false;
    u.body = [&] { toggle(u.h.ctx, "t", v); };
    u.h.move(20, u.row(0));
    u.frame();
    u.h.in.pointer_down = u.h.in.pointer_pressed = true;
    u.frame();
    u.h.in.pointer_down = false;
    u.h.in.pointer_released = true;
    u.frame();
    CHECK(v);
    CHECK(u.h.ctx.haptic() == Haptic::Light);
    u.frame();
    CHECK(u.h.ctx.haptic() == Haptic::None);
}

TEST_CASE("a focused field scrolls above the soft keyboard") {
    Ui u;
    u.flags = WindowScrollable;
    char data[16] = "";
    TextBuffer buf{data, sizeof data, 0};
    u.body = [&] {
        for (int i = 0; i < 10; i++) {
            layout_row(u.h.ctx, 1, nullptr, 0, 0);
            rd::label(u.h.ctx, "filler");
        }
        textbox(u.h.ctx, "field", buf);
    };
    u.frame();
    u.click(100, u.row(10));      // the field, at y 348..376 of a 400 px screen
    u.h.in.keyboard_inset = 160;  // the keyboard covers y >= 240
    u.frames(3);
    CHECK(th::window_scroll(u.h.ctx, "w") >= 376 - 240);
}
