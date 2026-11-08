// Copyright (c) 2026 Wavestorm Software
#include "harness.hpp"

TEST_CASE("FNV-1a known values") {
    CHECK(rd::hash_id(0, "") == 2166136261u);
    CHECK(rd::hash_id(0, "a") == 0xE40C292Cu);
    CHECK(rd::hash_id(0, "foobar") == 0xBF9CF968u);
}

TEST_CASE("## makes labels unique but is hashed") {
    CHECK(rd::hash_id(0, "Save##1") != rd::hash_id(0, "Save##2"));
    CHECK(rd::hash_id(0, "Save##1") != rd::hash_id(0, "Save"));
    CHECK(rd::detail::visible_label("Save##1") == "Save");
    CHECK(rd::detail::visible_label("##hidden") == "");
    CHECK(rd::detail::visible_label("Plain") == "Plain");
}

TEST_CASE("ID stack scopes identical labels") {
    th::Harness h;
    h.frame([&] {
        auto& s = rd::detail::state(h.ctx);
        rd::ID a = rd::detail::get_id(s, "x");
        rd::push_id(h.ctx, "row1");
        rd::ID b = rd::detail::get_id(s, "x");
        rd::pop_id(h.ctx);
        rd::push_id(h.ctx, "row2");
        rd::ID c = rd::detail::get_id(s, "x");
        rd::pop_id(h.ctx);
        CHECK(a != b);
        CHECK(b != c);
        CHECK(rd::detail::get_id(s, "x") == a);
    });
}

TEST_CASE("ID stack overflow sets error and does not crash") {
    rd::Config cfg;
    cfg.max_id_stack = 2;
    th::Harness h(cfg);
    h.frame([&] {
        for (int i = 0; i < 5; i++) rd::push_id(h.ctx, "a");
        for (int i = 0; i < 9; i++) rd::pop_id(h.ctx);
    });
    CHECK(h.ctx.error());
}
