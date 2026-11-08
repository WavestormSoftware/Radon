// Copyright (c) 2026 Wavestorm Software
#include "internal.hpp"

namespace rd {

ID hash_id(ID seed, std::string_view s) {
    ID h = seed ? seed : 2166136261u;
    for (unsigned char c : s) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

void push_id(Context& ctx, std::string_view id) {
    auto& s = detail::state(ctx);
    if (s.id_n >= s.cfg.max_id_stack) {
        s.error = true;
        return;
    }
    s.id_stack[s.id_n++] = hash_id(detail::top_id(s), id);
}

void pop_id(Context& ctx) {
    auto& s = detail::state(ctx);
    if (s.id_n > 0) s.id_n--;
}

namespace detail {

ID hash_u32(ID seed, uint32_t v) {
    ID h = seed ? seed : 2166136261u;
    for (int i = 0; i < 4; i++) {
        h ^= (v >> (8 * i)) & 0xff;
        h *= 16777619u;
    }
    return h;
}

ID top_id(const State& s) { return s.id_n > 0 ? s.id_stack[s.id_n - 1] : 0; }
ID get_id(State& s, std::string_view label) { return hash_id(top_id(s), label); }

std::string_view visible_label(std::string_view l) {
    size_t p = l.find("##");
    return p == std::string_view::npos ? l : l.substr(0, p);
}

}  // namespace detail
}  // namespace rd
