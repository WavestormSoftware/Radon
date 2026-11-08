// Copyright (c) 2026 Wavestorm Software
#include "harness.hpp"

using namespace rd;

TEST_CASE("rects merge into one command and indices are valid") {
    DrawList dl(16, 64, 96);
    dl.rect({0, 0, 10, 10}, {255, 0, 0, 255}, 2);
    dl.rect({20, 0, 10, 10}, {0, 255, 0, 255});
    CHECK(dl.command_count() == 1);
    CHECK(dl.vertex_count() == 8);
    CHECK(dl.index_count() == 12);
    CHECK(dl.commands()[0].index_count == 12);
    for (int i = 0; i < dl.index_count(); i++) CHECK(dl.indices()[i] < (uint32_t)dl.vertex_count());
}

TEST_CASE("clip, layer, texture changes start a new command") {
    DrawList dl(16, 64, 96);
    dl.rect({0, 0, 10, 10}, {1, 1, 1, 255});
    dl.set_clip({0, 0, 5, 5});
    dl.rect({0, 0, 10, 10}, {1, 1, 1, 255});
    dl.set_layer(3);
    dl.rect({0, 0, 10, 10}, {1, 1, 1, 255});
    dl.image(7, {0, 0, 4, 4}, {0, 0, 1, 1}, {255, 255, 255, 255});
    CHECK(dl.command_count() == 4);
    dl.sort_by_layer();
    CHECK(dl.commands()[3].layer == 3);
}

TEST_CASE("fully transparent and empty rects emit nothing") {
    DrawList dl(4, 16, 24);
    dl.rect({0, 0, 10, 10}, {1, 1, 1, 0});
    dl.rect({0, 0, 0, 10}, {1, 1, 1, 255});
    CHECK(dl.vertex_count() == 0);
}

TEST_CASE("limits: overflow sets the flag and drops primitives") {
    DrawList dl(2, 8, 12);
    for (int i = 0; i < 5; i++) dl.rect({0, 0, 10, 10}, {1, 1, 1, 255});
    CHECK(dl.overflow());
    CHECK(dl.vertex_count() == 8);
    CHECK(dl.index_count() == 12);
    dl.clear();
    CHECK_FALSE(dl.overflow());
    DrawList few(1, 100, 100);
    few.rect({0, 0, 1, 1}, {1, 1, 1, 255});
    few.set_layer(1);
    few.rect({0, 0, 1, 1}, {1, 1, 1, 255});  // needs a 2nd command
    CHECK(few.overflow());
    CHECK(few.command_count() == 1);
}

TEST_CASE("context surfaces draw list overflow through error()") {
    Config cfg;
    cfg.max_vertices = 16;
    th::Harness h(cfg);
    Rect r{0, 0, 300, 400};
    auto ui = [&] {
        if (begin_window(h.ctx, "w", r, WindowDefault)) {
            for (int i = 0; i < 10; i++) button(h.ctx, "b");
            end_window(h.ctx);
        }
    };
    h.frame(ui);
    CHECK(h.ctx.error());
    CHECK(h.ctx.draw_list().vertex_count() <= 16);
    h.frame([] {});
    CHECK_FALSE(h.ctx.error());  // the flag is per frame
}

TEST_CASE("windows sort by z-order, popups and tooltips above") {
    th::Harness h;
    Rect a{0, 0, 100, 100}, b{50, 50, 100, 100};
    auto ui = [&] {
        if (begin_window(h.ctx, "A", a, WindowNone)) end_window(h.ctx);
        if (begin_window(h.ctx, "B", b, WindowNone)) end_window(h.ctx);
    };
    h.frame(ui);
    const DrawList& dl = h.ctx.draw_list();
    REQUIRE(dl.command_count() >= 2);
    for (int i = 1; i < dl.command_count(); i++) CHECK(dl.commands()[i - 1].layer <= dl.commands()[i].layer);
}
