// Copyright (c) 2026 Wavestorm Software
// Counts heap allocations (global operator new, plus malloc/calloc/realloc on glibc) made while the
// UI runs frames after warm-up. The count must be zero.
#include <atomic>
#include <cstdlib>
#include <new>

static std::atomic<long> g_allocs{0};
static std::atomic<bool> g_counting{false};
static void note() {
    if (g_counting.load(std::memory_order_relaxed)) g_allocs.fetch_add(1, std::memory_order_relaxed);
}

void* operator new(std::size_t n) {
    note();
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    return operator new(n);
}
void operator delete(void* p) noexcept {
    std::free(p);
}
void operator delete[](void* p) noexcept {
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    std::free(p);
}

#if defined(__GLIBC__) && !defined(__SANITIZE_ADDRESS__)
#define RD_INTERPOSE_MALLOC 1
extern "C" {
void* __libc_malloc(size_t);
void* __libc_calloc(size_t, size_t);
void* __libc_realloc(void*, size_t);
void* malloc(size_t n) {
    note();
    return __libc_malloc(n);
}
void* calloc(size_t a, size_t b) {
    note();
    return __libc_calloc(a, b);
}
void* realloc(void* p, size_t n) {
    note();
    return __libc_realloc(p, n);
}
}
#else
#define RD_INTERPOSE_MALLOC 0
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "harness.hpp"

using namespace rd;

TEST_CASE("zero heap allocations per frame after warm-up") {
    MESSAGE("malloc interposition: " << RD_INTERPOSE_MALLOC);
    const auto& font = th::font_bytes();
    Config cfg;
    cfg.a11y = true;  // the accessibility tree must not allocate either
    th::Harness h(cfg);
    Theme t = Theme::dark();
    if (!font.empty()) {
        FontHandle f = h.ctx.load_font(font.data(), font.size(), 16);
        t.font_body = t.font_heading = t.font_small = f;
        h.ctx.set_default_font(f);
    } else {
        MESSAGE("no font: text paths not exercised");
    }
    h.ctx.set_theme(t);
    h.in.screen_size = {800, 600};

    Rect win{10, 10, 320, 300}, win2{200, 80, 200, 200};
    bool check = true, open = true;
    float sl = 0.3f, dr = 2;
    int sel = 1;
    const char* items[] = {"alpha", "beta", "gamma"};
    int radio = 0, ival = 3, tab = 0;
    Color col{200, 50, 90, 255};
    float plot[8] = {1, 3, 2, 5, 4, 2, 1, 3};
    char text[32] = "hello";
    TextBuffer tb{text, sizeof text, 5};
    bool modal_requested = true, asc = true;
    float lo = 0.2f, hi = 0.7f;
    int year = 2026, month = 11, day = 10, page = 3, sort = 0;
    const Icon nav_icons[] = {Icon::Home, Icon::Search, Icon::User};
    // 0.3 widgets
    bool refreshing = false, multi[] = {true, false, true}, tree_open = true;
    int wheel = 2, hour = 9, minute = 30, csel = 1, acc = 0, order[] = {0, 1, 2}, triggered = -1;
    char ac_text[32] = "a", pad_text[16] = "1";
    TextBuffer ac{ac_text, sizeof ac_text, 1}, padb{pad_text, sizeof pad_text, 1};
    const float series_a[] = {1, 4, 2, 6, 3}, series_b[] = {2, 1, 3, 2, 5};
    const ChartSeries series[] = {{"A", series_a, 5}, {"B", series_b, 5}};
    const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri"};
    TableColumn cols[] = {{"Name", 80}, {"Kind", 0}};
    TableSort tsort;
    const SwipeAction swipe_acts[] = {{"Delete", Icon::Trash, {220, 40, 60, 255}}};
    const std::string sel_text = "selectable\ntext";
    auto ui = [&] {
        if (begin_window(h.ctx, "Main", win, WindowDefault | WindowClosable | WindowScrollable)) {
            pull_to_refresh(h.ctx, "ptr", refreshing);
            const float lcols[] = {80, -1};
            layout_row(h.ctx, 2, lcols, 2, 0);
            rd::label(h.ctx, "Name");
            textbox(h.ctx, "##name", tb);
            layout_row(h.ctx, 1, nullptr, 0, 0);
            button(h.ctx, "Apply");
            tooltip(h.ctx, "tooltip");
            checkbox(h.ctx, "Check", check);
            slider(h.ctx, "##s", sl, 0, 1);
            drag_float(h.ctx, "##d", dr, 0.1f);
            progress_bar(h.ctx, sl);
            collapsing_header(h.ctx, "Header", open);
            combo(h.ctx, "##c", sel, items, 3);
            label_wrapped(h.ctx, "A long paragraph of text that has to wrap onto several lines.");
            layout_begin_column(h.ctx);
            button(h.ctx, "in column");
            layout_end_column(h.ctx);
            separator(h.ctx);
            for (int i = 0; i < 12; i++) {
                push_id(h.ctx, std::to_string(i));
                button(h.ctx, "row");
                pop_id(h.ctx);
            }
            separator_text(h.ctx, "More");
            layout_row(h.ctx, 3, nullptr, 0, 0);
            radio_button(h.ctx, "r0", radio, 0);
            radio_button(h.ctx, "r1", radio, 1);
            toggle(h.ctx, "tg", check);
            layout_row(h.ctx, 1, nullptr, 0, 0);
            slider_int(h.ctx, "##si", ival, 0, 10);
            drag_int(h.ctx, "##di", ival);
            stepper(h.ctx, "##st", sl, 0.1f, 0, 1);
            tab_bar(h.ctx, "##tabs", tab, items, 3);
            listbox(h.ctx, "##lb", sel, items, 3, 70);
            if (tree_node(h.ctx, "tree", open)) {
                bullet_text(h.ctx, "leaf");
                tree_pop(h.ctx);
            }
            color_picker(h.ctx, "##col", col, true);
            plot_lines(h.ctx, plot, 8, 0, 0, 40);
            plot_histogram(h.ctx, plot, 8, 0, 1, 40);
            spinner(h.ctx);
            badge(h.ctx, "badge", {255, 0, 0, 255});
            icon_button(h.ctx, "ic", Icon::Search);
            // third widget family
            password_box(h.ctx, "##pw", tb);
            search_box(h.ctx, "##find", tb);
            text_area(h.ctx, "##area", tb, 60);
            range_slider(h.ctx, "##range", lo, hi, 0, 1);
            segmented(h.ctx, "##seg", tab, items, 3);
            chip(h.ctx, "chip", check);
            rating(h.ctx, "##stars", ival);
            link(h.ctx, "link");
            calendar(h.ctx, "##cal", year, month, day);
            date_picker(h.ctx, "##date", year, month, day);
            pagination(h.ctx, "##pages", page, 30);
            breadcrumbs(h.ctx, "##bc", items, 3);
            heading(h.ctx, "Heading");
            avatar(h.ctx, "Ada Lovelace", col);
            alert(h.ctx, Tone::Warning, "An alert whose text is long enough to wrap onto a second line.");
            progress_ring(h.ctx, sl);
            skeleton(h.ctx);
            table_header(h.ctx, "tbl", items, 3, nullptr, &sort, &asc);
            for (int i = 0; i < 3; i++) {
                table_row(h.ctx, i, i == 1);
                for (int c = 0; c < 3; c++) rd::label(h.ctx, items[c]);
            }
            if (begin_child(h.ctx, "long", 80)) {
                int first = 0, last = 0;
                list_clipper_begin(h.ctx, 5000, 0, first, last);
                for (int i = first; i < last; i++) {
                    layout_row(h.ctx, 1, nullptr, 0, 0);
                    rd::label(h.ctx, "row");
                }
                list_clipper_end(h.ctx);
                end_child(h.ctx);
            }
            layout_row(h.ctx, 1, nullptr, 0, 0);
            if (begin_card(h.ctx, "card")) {
                rd::label(h.ctx, "in a card");
                end_card(h.ctx);
            }
            begin_disabled(h.ctx);
            button(h.ctx, "disabled");
            end_disabled(h.ctx);
            transition_begin(h.ctx, "tr", tab);
            button(h.ctx, "fading");
            if (context_menu_begin(h.ctx, "ctx")) {
                menu_item(h.ctx, "Copy");
                popup_end(h.ctx);
            }
            transition_end(h.ctx);
            layout_row(h.ctx, 1, nullptr, 0, 50);
            nav_bar(h.ctx, "##nav", tab, nav_icons, items, 3);
            animate(h.ctx, "anim", sl);
            push_theme(h.ctx, t);
            button(h.ctx, "scoped", ButtonStyle::Primary);
            pop_theme(h.ctx);
            if (begin_menu_bar(h.ctx)) {
                if (begin_menu(h.ctx, "File")) {
                    menu_item(h.ctx, "Open", "Ctrl+O");
                    end_menu(h.ctx);
                }
                end_menu_bar(h.ctx);
            }
            // 0.3 widgets
            layout_row(h.ctx, 1, nullptr, 0, 0);
            textbox(h.ctx, "##num", tb, "0", TextInputType::Decimal);
            wheel_picker(h.ctx, "##wheel", wheel, items, 3, 3);
            time_picker(h.ctx, "##time", hour, minute, false);
            combo_search(h.ctx, "##cs", csel, items, 3);
            multi_combo(h.ctx, "##mc", multi, items, 3);
            autocomplete(h.ctx, "##ac", ac, items, 3);
            number_pad(h.ctx, "##pad", padb);
            chart(h.ctx, "##line", ChartType::Area, series, 2, days, 100);
            chart(h.ctx, "##bars", ChartType::StackedBar, series, 2, nullptr, 100);
            pie_chart(h.ctx, "##pie", series_a, items, 3, 80, true);
            sparkline(h.ctx, series_a, 5);
            if (table_begin(h.ctx, "##table", cols, 2, 80, &tsort)) {
                table_tree_row(h.ctx, 0, "root", 0, &tree_open);
                rd::label(h.ctx, "dir");
                table_tree_row(h.ctx, 1, "leaf", 1, nullptr);
                rd::label(h.ctx, "file");
                table_end(h.ctx);
            }
            flow_begin(h.ctx);
            for (int i = 0; i < 3; i++) chip(h.ctx, items[i], i == 1);
            flow_end(h.ctx);
            layout_grid(h.ctx, 60);
            button(h.ctx, "g1");
            button(h.ctx, "g2");
            if (carousel_begin(h.ctx, "##car", 80, 120)) {
                button(h.ctx, "c1");
                button(h.ctx, "c2");
                carousel_end(h.ctx);
            }
            if (accordion_begin(h.ctx, "Section", acc, 0)) {
                rd::label(h.ctx, "inside");
                accordion_end(h.ctx);
            }
            layout_row(h.ctx, 1, nullptr, 0, 0);
            rich_text(h.ctx, "**bold** *it* `code` [link](u) {#ff0000}red{/} :star:");
            markdown(h.ctx, "# Title\nA *paragraph*.\n\n- item\n1. one\n> quote\n```\ncode\n```\n---");
            selectable_text(h.ctx, "##seltext", sel_text);
            reorder_list(h.ctx, "##reorder", order, items, 3);
            layout_row(h.ctx, 1, nullptr, 0, 0);
            swipe_item(h.ctx, "swipe", swipe_acts, 1, triggered);
            button(h.ctx, "drag me");
            drag_source(h.ctx, "t", &wheel, sizeof wheel, "dragging");
            drop_target(h.ctx, "t");
            if (modal_requested) open_modal(h.ctx, "m");
            modal_requested = false;
            if (modal_begin(h.ctx, "m", "Modal", 200)) {
                button(h.ctx, "ok");
                modal_end(h.ctx);
            }
            end_window(h.ctx);
        }
        if (begin_window(h.ctx, "Second", win2)) {
            rd::label(h.ctx, "Hello");
            end_window(h.ctx);
        }
        dock_space(h.ctx, "dock", {420, 300, 360, 280});
        if (begin_dock_panel(h.ctx, "Panel one")) {
            rd::label(h.ctx, "docked");
            end_dock_panel(h.ctx);
        }
        if (begin_dock_panel(h.ctx, "Panel two", nullptr, DockPlace::Left)) {
            rd::label(h.ctx, "left");
            end_dock_panel(h.ctx);
        }
        if (begin_dock_panel(h.ctx, "Panel three", nullptr, DockPlace::Floating)) {
            rd::label(h.ctx, "floating");
            end_dock_panel(h.ctx);
        }
        edge_swipe(h.ctx, Side::Left);
    };

    const std::function<void()> fn = ui;  // built once: std::function construction may allocate

    // Warm-up: first use of every glyph, window, edit state and animation slot.
    h.in.pointer = {40, 60};
    for (int i = 0; i < 6; i++) h.frame(fn);
    h.in.pointer = {60, 40};
    h.in.pointer_down = h.in.pointer_pressed = true;
    h.frame(fn);
    h.in.pointer_pressed = false;
    h.frame(fn);
    h.in.pointer_down = false;
    h.in.pointer_released = true;
    h.frame(fn);
    for (int i = 0; i < 4; i++) h.frame(fn);

    g_allocs = 0;
    g_counting = true;
    for (int i = 0; i < 200; i++) {
        h.in.pointer = {30.0f + (float)(i % 50), 40.0f + (float)(i % 30)};
        h.in.wheel = (i % 7 == 0) ? -1.0f : 0.0f;
        h.in.secondary_pressed = i % 23 == 0;
        if (i % 40 == 0) toast(h.ctx, "A toast", Tone::Success, 0.4f);
        if (i % 10 == 0) h.key(KeyA, false, true);
        if (i % 13 == 0) h.type("x");
        if (i % 17 == 0) h.key(KeyTab);
        if (i % 19 == 0) h.key(KeyZ, false, true);
        h.in.touch = i % 50 > 40;
        h.in.touch_count = h.in.touch ? 2 : 0;
        h.in.touches[0] = {1, h.in.pointer};
        h.in.touches[1] = {2, {h.in.pointer.x + 40.0f + (float)(i % 5), h.in.pointer.y}};
        h.frame(fn);
    }
    g_counting = false;
    long n = g_allocs.load();
    std::printf("heap allocations over 200 frames after warm-up: %ld\n", n);
    CHECK(n == 0);
    CHECK_FALSE(h.ctx.error());
}
