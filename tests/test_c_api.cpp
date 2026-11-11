// Copyright (c) 2026 Wavestorm Software
// The C API runs a frame and exposes the same draw list as the C++ API.
#include <ostream>  // MSVC: doctest streams string_view and needs the full ostream
#include <doctest.h>
#include <radon/radon_c.h>

TEST_CASE("C API: frame, button click, draw list") {
    rd_context* c = rd_create();
    rd_rect win{0, 0, 200, 120};
    rd_input in{};
    in.screen_w = in.screen_h = 300;
    int clicked = 0, checked = 0;
    auto frame = [&] {
        rd_begin_frame(c, &in, 0.016f);
        if (rd_begin_window(c, "w", &win)) {
            rd_layout_row(c, 1, nullptr, 0, 0);
            clicked += rd_button(c, "ok");
            rd_checkbox(c, "c", &checked);
            static const char* const items[] = {"a", "b"};
            static int sel = 0, open = 1;
            static float f = 0.5f, d = 1;
            rd_label_wrapped(c, "wrapped text");
            rd_progress_bar(c, f);
            rd_drag_float(c, "d", &d, 0.1f);
            rd_button_styled(c, "p", 1);
            if (rd_collapsing_header(c, "h", &open)) rd_combo(c, "combo", &sel, items, 2);
            rd_end_window(c);
        }
        rd_end_frame(c);
    };
    frame();
    int nv = 0, ni = 0, nc = 0;
    rd_vertices(c, &nv);
    rd_indices(c, &ni);
    rd_commands(c, &nc);
    CHECK(nv > 0);
    CHECK(ni > 0);
    CHECK(nc > 0);
    in.pointer_x = 40, in.pointer_y = 40;
    frame();
    in.pointer_down = in.pointer_pressed = 1;
    frame();
    in.pointer_pressed = 0, in.pointer_down = 0, in.pointer_released = 1;
    frame();
    CHECK(clicked == 1);
    rd_destroy(c);
}
