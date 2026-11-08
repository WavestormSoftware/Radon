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
    th::Harness h;
    Theme t = Theme::dark();
    if (!font.empty()) {
        FontHandle f = h.ctx.load_font(font.data(), font.size(), 16);
        t.font_body = t.font_heading = t.font_small = f;
        h.ctx.set_default_font(f);
    } else {
        MESSAGE("no font: text paths not exercised");
    }
    h.ctx.set_theme(t);

    Rect win{10, 10, 320, 300}, win2{200, 80, 200, 200};
    bool check = true, open = true;
    float sl = 0.3f, dr = 2;
    int sel = 1;
    const char* items[] = {"alpha", "beta", "gamma"};
    char text[32] = "hello";
    TextBuffer tb{text, sizeof text, 5};
    auto ui = [&] {
        if (begin_window(h.ctx, "Main", win, WindowDefault | WindowClosable | WindowScrollable)) {
            const float cols[] = {80, -1};
            layout_row(h.ctx, 2, cols, 2, 0);
            label(h.ctx, "Name");
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
            end_window(h.ctx);
        }
        if (begin_window(h.ctx, "Second", win2)) {
            label(h.ctx, "Hello");
            end_window(h.ctx);
        }
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
        if (i % 10 == 0) h.key(KeyA, false, true);
        if (i % 13 == 0) h.type("x");
        if (i % 17 == 0) h.key(KeyTab);
        h.frame(fn);
    }
    g_counting = false;
    long n = g_allocs.load();
    std::printf("heap allocations over 200 frames after warm-up: %ld\n", n);
    CHECK(n == 0);
    CHECK_FALSE(h.ctx.error());
}
