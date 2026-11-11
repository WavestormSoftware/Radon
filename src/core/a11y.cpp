// Copyright (c) 2026 Wavestorm Software
// Accessibility tree: one node per widget per frame, for a backend to forward to a screen reader.
#include "internal.hpp"

namespace rd::detail {

static void copy_text(char* dst, size_t cap, std::string_view src) {
    const size_t n = utf8_fit(src, cap - 1);
    if (n) std::memcpy(dst, src.data(), n);
    dst[n] = 0;
}

static A11yNode* node_for(State& s, ID id) {
    if (s.a11y_n > 0 && s.a11y[s.a11y_n - 1].id == id) return &s.a11y[s.a11y_n - 1];
    if (s.a11y_n >= s.cfg.max_a11y_nodes) {
        s.error = true;
        return nullptr;
    }
    A11yNode& n = s.a11y[s.a11y_n++];
    n = A11yNode{};
    n.id = id;
    n.parent = s.cur_window;
    return &n;
}

void a11y_control(State& s, ID id, const Rect& r, bool focusable) {
    if (!focusable) return;  // drag handles, title bars and scrollbars are not announced
    A11yNode* n = node_for(s, id);
    if (!n) return;
    n->rect = r;
    n->flags |= A11yNode::Focusable | (s.focus == id ? A11yNode::Focused : 0u) |
                (s.disabled > 0 ? A11yNode::Disabled : 0u);
}

void a11y_node(State& s, A11yRole role, std::string_view label, std::string_view value, uint32_t flags) {
    if (!s.cfg.a11y) return;
    A11yNode* n = node_for(s, s.last_id);
    if (!n) return;
    n->rect = s.last_rect;
    n->role = role;
    n->flags |= flags | (s.disabled > 0 ? A11yNode::Disabled : 0u);
    copy_text(n->label, sizeof n->label, visible_label(label));
    copy_text(n->value, sizeof n->value, value);
}

}  // namespace rd::detail

namespace rd::detail {

void a11y_static(State& s, A11yRole role, const Rect& r, std::string_view label) {
    if (!s.cfg.a11y || label.empty()) return;
    if (s.a11y_n >= s.cfg.max_a11y_nodes) {
        s.error = true;
        return;
    }
    A11yNode& n = s.a11y[s.a11y_n++];
    n = A11yNode{};
    n.id = hash_u32(hash_id(top_id(s), label), 0xA11Eu);
    n.parent = s.cur_window;
    n.role = role;
    n.rect = r;
    copy_text(n.label, sizeof n.label, visible_label(label));
}

}  // namespace rd::detail
