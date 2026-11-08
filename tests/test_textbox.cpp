// Copyright (c) 2026 Wavestorm Software
#include "harness.hpp"

using namespace rd;

namespace {
struct Clip {
    std::string text;
    static int get(void* u, char* out, int cap) {
        auto* c = static_cast<Clip*>(u);
        int n = (int)c->text.size() < cap ? (int)c->text.size() : cap;
        std::memcpy(out, c->text.data(), (size_t)n);
        return n;
    }
    static void set(void* u, const char* t, int n) { static_cast<Clip*>(u)->text.assign(t, (size_t)n); }
};

struct Tb {
    th::Harness h;
    Rect rect{0, 0, 300, 200};
    char data[16] = "";
    TextBuffer buf{data, sizeof data, 0};
    Response r;
    Clip clip;
    Tb() { h.ctx.set_clipboard_callbacks(Clip::get, Clip::set, &clip); }
    void frame() {
        h.frame([&] {
            if (begin_window(h.ctx, "w", rect, WindowNone)) {
                r = textbox(h.ctx, "t", buf);
                end_window(h.ctx);
            }
        });
    }
    void focus() {
        frame();
        h.move(20, 20);
        frame();
        h.in.pointer_down = h.in.pointer_pressed = true;
        frame();
        h.in.pointer_down = false;
        h.in.pointer_released = true;
        frame();
        h.move(-100, -100);
        frame();
        REQUIRE(r.focused);
    }
    std::string text() const { return std::string(buf.data, (size_t)buf.length); }
    void type(const char* s) {
        h.type(s);
        frame();
    }
    void key(int k, bool shift = false, bool ctrl = false) {
        h.key(k, shift, ctrl);
        frame();
    }
};
}  // namespace

TEST_CASE("insert and delete") {
    Tb t;
    t.focus();
    t.type("abc");
    CHECK(t.text() == "abc");
    CHECK(t.r.changed);
    t.key(KeyBackspace);
    CHECK(t.text() == "ab");
    t.key(KeyLeft);
    t.key(KeyDelete);
    CHECK(t.text() == "a");
    t.key(KeyHome);
    t.type("X");
    CHECK(t.text() == "Xa");
    t.key(KeyEnd);
    t.type("Y");
    CHECK(t.text() == "XaY");
    CHECK(t.buf.data[t.buf.length] == 0);
}

TEST_CASE("selection with shift, replace on typing, collapse with arrows") {
    Tb t;
    t.focus();
    t.type("hello");
    t.key(KeyLeft, true);
    t.key(KeyLeft, true);
    t.type("p!");
    CHECK(t.text() == "help!");
    t.key(KeyLeft, true);
    t.key(KeyBackspace);
    CHECK(t.text() == "help");
    t.key(KeyHome, true);  // select all to the left
    t.key(KeyRight);       // collapses to the right edge of the selection
    t.type("_");
    CHECK(t.text() == "help_");
}

TEST_CASE("select all, copy, cut, paste through the clipboard mock") {
    Tb t;
    t.focus();
    t.type("abc");
    t.key(KeyA, false, true);
    t.key(KeyC, false, true);
    CHECK(t.clip.text == "abc");
    t.key(KeyX, false, true);
    CHECK(t.text() == "");
    t.clip.text = "x\ny";  // control characters are filtered
    t.key(KeyV, false, true);
    CHECK(t.text() == "xy");
    t.key(KeyV, false, true);
    CHECK(t.text() == "xyxy");
}

TEST_CASE("capacity is respected and no code point is split") {
    Tb t;  // capacity 16 => 15 bytes
    t.focus();
    t.type("0123456789abcd");  // 14 bytes
    t.type("\xC3\xA9");        // 2 bytes do not fit in the one remaining byte
    CHECK(t.text() == "0123456789abcd");
    t.type("Z");
    CHECK(t.text() == "0123456789abcdZ");
    t.type("Q");
    CHECK(t.buf.length == 15);
}

TEST_CASE("UTF-8: cursor and delete move by whole code points") {
    Tb t;
    t.focus();
    t.type("a\xC3\xA9");  // a, é
    CHECK(t.buf.length == 3);
    t.key(KeyBackspace);
    CHECK(t.text() == "a");
    t.type("\xE2\x82\xAC");  // €
    t.key(KeyLeft);
    t.key(KeyDelete);
    CHECK(t.text() == "a");
}

TEST_CASE("Enter submits, Escape drops focus, editing needs focus") {
    Tb t;
    t.focus();
    t.type("hi");
    t.key(KeyEnter);
    CHECK(t.r.clicked);
    t.key(KeyEscape);
    t.frame();
    CHECK_FALSE(t.r.focused);
    t.type("zzz");
    CHECK(t.text() == "hi");
}

TEST_CASE("wants_text_input follows focus (soft keyboard)") {
    Tb t;
    t.frame();
    CHECK_FALSE(t.h.ctx.wants_text_input());
    t.focus();
    t.frame();
    CHECK(t.h.ctx.wants_text_input());
    t.key(KeyEscape);
    t.frame();
    t.frame();
    CHECK_FALSE(t.h.ctx.wants_text_input());
}

TEST_CASE("cursor blink is driven by dt and reset by typing") {
    Tb t;
    t.focus();
    t.type("a");
    auto count_caret = [&] { return t.h.ctx.draw_list().vertex_count(); };
    int with = count_caret();
    for (int i = 0; i < 40; i++) t.frame();  // 40/60 s: caret is in its off phase
    CHECK(count_caret() < with);
    t.type("b");
    CHECK(count_caret() >= with);
}

TEST_CASE("invalid buffers are reported, not dereferenced") {
    th::Harness h;
    Rect rect{0, 0, 200, 100};
    TextBuffer bad{nullptr, 0, 0};
    h.frame([&] {
        if (begin_window(h.ctx, "w", rect, WindowNone)) {
            textbox(h.ctx, "t", bad);
            end_window(h.ctx);
        }
    });
    CHECK(h.ctx.error());
}

TEST_CASE("Tab moves focus between text boxes") {
    th::Harness h;
    Rect rect{0, 0, 300, 200};
    char d1[8] = "", d2[8] = "";
    TextBuffer b1{d1, 8, 0}, b2{d2, 8, 0};
    Response r1, r2;
    auto ui = [&] {
        if (begin_window(h.ctx, "w", rect, WindowNone)) {
            r1 = textbox(h.ctx, "a", b1);
            r2 = textbox(h.ctx, "b", b2);
            end_window(h.ctx);
        }
    };
    h.frame(ui);
    h.key(KeyTab);
    h.frame(ui);
    h.key(KeyTab);
    h.frame(ui);
    h.frame(ui);
    CHECK(r2.focused);
    h.type("q");
    h.frame(ui);
    CHECK(b1.length == 0);
    CHECK(b2.length == 1);
}
