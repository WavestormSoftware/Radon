// Copyright (c) 2026 Wavestorm Software
// Docking: a binary tree of splits whose leaves hold tabbed panels; panels can float and dock again.
#include "internal.hpp"

namespace rd {
using namespace detail;

namespace {

enum Zone { ZoneCenter, ZoneLeft, ZoneRight, ZoneTop, ZoneBottom };
constexpr float kGap = 4;     // splitter thickness
constexpr int kMaxTabs = 16;  // tabs drawn per leaf

bool is_leaf(const DockNode& n) {
    return n.child[0] < 0;
}

int alloc_node(State& s, ID dock, int parent) {
    for (int i = 0; i < s.cfg.max_dock_nodes; i++) {
        if (s.dock_nodes[i].used) continue;
        s.dock_nodes[i] = DockNode{};
        s.dock_nodes[i].used = true;
        s.dock_nodes[i].dock = dock;
        s.dock_nodes[i].parent = parent;
        return i;
    }
    s.error = true;
    return -1;
}

int first_leaf(const State& s, int n) {
    while (n >= 0 && !is_leaf(s.dock_nodes[n])) n = s.dock_nodes[n].child[0];
    return n;
}

int find_root(const State& s, ID dock) {
    for (int i = 0; i < s.cfg.max_dock_nodes; i++) {
        const DockNode& n = s.dock_nodes[i];
        if (n.used && n.dock == dock && n.parent < 0) return i;
    }
    return -1;
}

bool live(const State& s, const DockPanel& p) {
    return p.id != 0 && p.last_frame + 1 >= s.frame;
}

void repoint(State& s, int from, int to) {
    for (int i = 0; i < s.cfg.max_dock_panels; i++)
        if (s.dock_panels[i].id && s.dock_panels[i].node == from) s.dock_panels[i].node = to;
}

// Moves node `from`'s contents (split or leaf) into slot `to`, keeping `to`'s parent.
void move_node(State& s, int from, int to) {
    const int parent = s.dock_nodes[to].parent, main = s.dock_nodes[to].main;
    s.dock_nodes[to] = s.dock_nodes[from];
    s.dock_nodes[to].parent = parent;
    s.dock_nodes[to].main = main;
    for (int c : s.dock_nodes[to].child)
        if (c >= 0) s.dock_nodes[c].parent = to;
    repoint(s, from, to);
}

// Removes leaves whose panels were all closed or moved away: the sibling takes the parent's place.
void collapse(State& s, int root) {
    for (bool again = true; again;) {
        again = false;
        for (int n = 0; n < s.cfg.max_dock_nodes; n++) {
            DockNode& d = s.dock_nodes[n];
            if (!d.used || d.dock != s.dock_nodes[root].dock || !is_leaf(d) || d.parent < 0) continue;
            if (n == s.dock_nodes[root].main) continue;  // the central area stays, even when empty
            bool empty = true;
            for (int i = 0; i < s.cfg.max_dock_panels && empty; i++)
                if (s.dock_panels[i].node == n && live(s, s.dock_panels[i])) empty = false;
            if (!empty) continue;
            const int p = d.parent;
            const int sib =
                s.dock_nodes[p].child[0] == n ? s.dock_nodes[p].child[1] : s.dock_nodes[p].child[0];
            int& main = s.dock_nodes[root].main;
            move_node(s, sib, p);
            if (main == sib || main == n) main = first_leaf(s, p);
            repoint(s, n, first_leaf(s, p));  // closed panels reopen next to where they were
            s.dock_nodes[n].used = s.dock_nodes[sib].used = false;
            again = true;
            break;
        }
    }
}

void layout_node(State& s, int n, Rect r) {
    DockNode& d = s.dock_nodes[n];
    d.rect = r;
    if (is_leaf(d)) {
        const float th = s.cfg.theme.cell_height;
        d.body = {r.x, r.y + th, r.w, std::fmax(0.0f, r.h - th)};
        return;
    }
    d.body = r;
    if (!d.vertical) {
        const float w0 = std::floor((r.w - kGap) * d.ratio);
        layout_node(s, d.child[0], {r.x, r.y, w0, r.h});
        layout_node(s, d.child[1], {r.x + w0 + kGap, r.y, r.w - w0 - kGap, r.h});
    } else {
        const float h0 = std::floor((r.h - kGap) * d.ratio);
        layout_node(s, d.child[0], {r.x, r.y, r.w, h0});
        layout_node(s, d.child[1], {r.x, r.y + h0 + kGap, r.w, r.h - h0 - kGap});
    }
}

// Splits leaf `leaf`: its panels move to a new leaf, and a second new leaf on side `zone` is returned.
int split_leaf(State& s, int leaf, int zone, float share) {
    const ID dock = s.dock_nodes[leaf].dock;
    const int a = alloc_node(s, dock, leaf);
    if (a < 0) return -1;
    const int b = alloc_node(s, dock, leaf);
    if (b < 0) {
        s.dock_nodes[a].used = false;
        return -1;
    }
    s.dock_nodes[a].selected = s.dock_nodes[leaf].selected;
    repoint(s, leaf, a);
    const bool first = zone == ZoneLeft || zone == ZoneTop;
    DockNode& d = s.dock_nodes[leaf];
    d.vertical = zone == ZoneTop || zone == ZoneBottom;
    d.child[0] = first ? b : a;
    d.child[1] = first ? a : b;
    d.ratio = first ? share : 1 - share;
    d.selected = 0;
    const int root = find_root(s, dock);
    if (root >= 0 && s.dock_nodes[root].main == leaf) s.dock_nodes[root].main = a;
    return b;
}

// Gives a new panel its first place: the main leaf, or a new strip along one side of the whole dock.
int place_panel(State& s, int root, DockPlace place, float ratio) {
    int main = s.dock_nodes[root].main;
    if (main < 0 || !s.dock_nodes[main].used || !is_leaf(s.dock_nodes[main])) main = first_leaf(s, root);
    s.dock_nodes[root].main = main;
    if (place == DockPlace::Center) return main;
    const int zone = place == DockPlace::Left    ? ZoneLeft
                     : place == DockPlace::Right ? ZoneRight
                     : place == DockPlace::Top   ? ZoneTop
                                                 : ZoneBottom;
    const int keep = alloc_node(s, s.dock_nodes[root].dock, root);  // the old tree moves down a level
    if (keep < 0) return main;
    move_node(s, root, keep);
    s.dock_nodes[keep].main = -1;
    if (s.dock_nodes[root].main == root) s.dock_nodes[root].main = keep;
    DockNode& r = s.dock_nodes[root];
    r.child[0] = r.child[1] = -1;
    r.selected = 0;
    const int leaf = alloc_node(s, r.dock, root);
    if (leaf < 0) {
        move_node(s, keep, root);
        if (s.dock_nodes[root].main == keep) s.dock_nodes[root].main = root;
        s.dock_nodes[keep].used = false;
        return first_leaf(s, root);
    }
    DockNode& rr = s.dock_nodes[root];
    const bool first = zone == ZoneLeft || zone == ZoneTop;
    rr.vertical = zone == ZoneTop || zone == ZoneBottom;
    rr.child[0] = first ? leaf : keep;
    rr.child[1] = first ? keep : leaf;
    const float share = std::fmin(std::fmax(ratio, 0.1f), 0.9f);
    rr.ratio = first ? share : 1 - share;
    return leaf;
}

DockPanel* find_panel(State& s, ID id) {
    DockPanel* free_slot = nullptr;
    for (int i = 0; i < s.cfg.max_dock_panels; i++) {
        DockPanel& p = s.dock_panels[i];
        if (p.id == id) return &p;
        if (!p.id && !free_slot) free_slot = &p;
    }
    return free_slot;
}

}  // namespace

void dock_space(Context& ctx, std::string_view label, Rect rect) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const ID did = hash_id(top_id(s), label);
    int root = find_root(s, did);
    if (root < 0) {
        root = alloc_node(s, did, -1);
        if (root < 0) return;
        s.dock_nodes[root].main = root;
    }
    s.dock_cur = did;
    s.dock_cur_root = root;
    collapse(s, root);
    layout_node(s, root, rect);
    s.dl.rect(rect, t.background);
    const ID dragging = s.dock_dragging;
    s.dock_dragging = 0;
    s.dock_target = s.dock_zone = -1;

    for (int n = 0; n < s.cfg.max_dock_nodes; n++) {
        DockNode& d = s.dock_nodes[n];
        if (!d.used || d.dock != did) continue;
        if (!is_leaf(d)) {  // the gap between the two halves resizes them
            const Rect a = s.dock_nodes[d.child[0]].rect;
            const Rect g = d.vertical ? Rect{d.rect.x, a.y + a.h - 2, d.rect.w, kGap + 4}
                                      : Rect{a.x + a.w - 2, d.rect.y, kGap + 4, d.rect.h};
            const ID gid = hash_u32(did, 0xD1000u + (uint32_t)n);
            const Response gr = update_control(ctx, gid, g, false, true);
            const bool drag = s.active == gid && s.in.pointer_down;
            if (gr.hovered || drag) s.cursor = d.vertical ? Cursor::ResizeNS : Cursor::ResizeEW;
            if (drag) {
                const float span = (d.vertical ? d.rect.h : d.rect.w) - kGap;
                if (span > 1)
                    d.ratio = std::fmin(
                        std::fmax(d.ratio + (d.vertical ? s.pointer_delta.y : s.pointer_delta.x) / span,
                                  0.08f),
                        0.92f);
            }
            const float h = animate(s, gid, gr.hovered || drag, false);
            if (h > 0.01f) s.dl.rect({g.x + 2, g.y + 2, g.w - 4, g.h - 4}, with_alpha(t.accent, h), 1);
            continue;
        }
        // A leaf: tab strip and panel background.
        int tabs[kMaxTabs], nt = 0;
        for (int i = 0; i < s.cfg.max_dock_panels && nt < kMaxTabs; i++)
            if (s.dock_panels[i].node == n && live(s, s.dock_panels[i])) tabs[nt++] = i;
        for (int i = 1; i < nt; i++)  // by order of arrival
            for (int k = i; k > 0 && s.dock_panels[tabs[k]].order < s.dock_panels[tabs[k - 1]].order; k--) {
                const int tmp = tabs[k];
                tabs[k] = tabs[k - 1];
                tabs[k - 1] = tmp;
            }
        bool sel_ok = false;
        for (int i = 0; i < nt; i++) sel_ok = sel_ok || s.dock_panels[tabs[i]].id == d.selected;
        if (!sel_ok) d.selected = nt ? s.dock_panels[tabs[0]].id : 0;
        const Rect strip{d.rect.x, d.rect.y, d.rect.w, d.body.y - d.rect.y};
        s.dl.rect(strip, t.surface_hover);
        s.dl.rect(d.body, t.surface);
        s.dl.rect({strip.x, strip.y + strip.h - 1, strip.w, 1}, t.border);
        push_clip(s, strip);
        float x = strip.x;
        for (int i = 0; i < nt; i++) {
            DockPanel& p = s.dock_panels[tabs[i]];
            const float cw = p.closable ? strip.h * 0.7f : 0;
            const float tw = text_size(s, t.font_body, p.title).x + 2 * t.padding + cw;
            const Rect tr{x, strip.y, tw, strip.h};
            x += tw + 1;
            const ID tab = hash_u32(p.id, 0x7AB);
            Response cr;
            const Rect cb{tr.x + tr.w - cw - t.padding * 0.5f, tr.y, cw, tr.h};
            if (p.closable) cr = update_control(ctx, hash_u32(p.id, 0xC105), cb, false, false);
            const Response rs = update_control(ctx, tab, tr, false, true);
            if (rs.pressed) d.selected = p.id;
            if (cr.clicked) p.close_clicked = true;
            const bool on = d.selected == p.id;
            const float h = animate(s, tab, rs.hovered, false);
            if (on) {
                s.dl.rect(tr, t.surface);
                s.dl.rect({tr.x, tr.y, tr.w, 2}, t.accent);
            } else if (h > 0) {
                s.dl.rect(tr, with_alpha(t.surface_active, h * 0.6f));
            }
            draw_text_fit(s, {tr.x + t.padding, tr.y, tr.w - 2 * t.padding - cw, tr.h}, p.title,
                          on ? t.text : t.text_muted, false);
            if (p.closable)
                draw_icon(ctx, Icon::Close,
                          {cb.x + cb.w * 0.15f, cb.y + (cb.h - cb.w * 0.7f) / 2, cb.w * 0.7f, cb.w * 0.7f},
                          cr.hovered ? t.error : t.text_muted);
            // Dragging a tab out of the strip turns the panel into a floating window under the pointer.
            const float dx = s.in.pointer.x - s.press_pos.x, dy = s.in.pointer.y - s.press_pos.y;
            if (s.active == tab && s.in.pointer_down && dx * dx + dy * dy > 64) {
                p.node = -1;
                p.float_rect = {s.in.pointer.x - std::fmin(tw, 120.0f) / 2,
                                s.in.pointer.y - t.title_height / 2, std::fmax(260.0f, d.rect.w * 0.6f),
                                std::fmax(180.0f, d.rect.h * 0.6f)};
                s.active = hash_u32(p.id, 1);  // the floating window's title bar continues the drag
                s.active_captures = true;
            }
        }
        pop_clip(s);
    }

    if (!dragging) return;
    // Drop zones over the leaf under the pointer: its middle adds a tab, its edges split it.
    const Vec2 p = s.in.pointer;
    for (int n = 0; n < s.cfg.max_dock_nodes; n++) {
        const DockNode& d = s.dock_nodes[n];
        if (!d.used || d.dock != did || !is_leaf(d) || !d.rect.contains(p)) continue;
        const Rect r = d.rect;
        const float fx = (p.x - r.x) / r.w, fy = (p.y - r.y) / r.h;
        int zone = ZoneCenter;
        float edge = std::fmin(std::fmin(fx, 1 - fx), std::fmin(fy, 1 - fy));
        if (edge < 0.28f) {
            zone = edge == fx ? ZoneLeft : edge == 1 - fx ? ZoneRight : edge == fy ? ZoneTop : ZoneBottom;
        }
        Rect pr = r;
        if (zone == ZoneLeft) pr.w *= 0.5f;
        if (zone == ZoneRight) pr.x += r.w * 0.5f, pr.w *= 0.5f;
        if (zone == ZoneTop) pr.h *= 0.5f;
        if (zone == ZoneBottom) pr.y += r.h * 0.5f, pr.h *= 0.5f;
        s.dock_target = n;
        s.dock_zone = zone;
        const uint32_t layer = s.cur_layer;
        s.dl.set_layer(kOverlayLayer + 2);
        s.dl.break_command();
        s.dl.set_clip({-1e6f, -1e6f, 2e6f, 2e6f});
        s.dl.rect({pr.x + 4, pr.y + 4, pr.w - 8, pr.h - 8}, with_alpha(t.accent, 0.22f), t.window_radius);
        s.dl.rect({pr.x + 4, pr.y + 4, pr.w - 8, pr.h - 8}, t.accent, t.window_radius, 2);
        s.dl.set_layer(layer);
        s.dl.break_command();
        s.dl.set_clip(s.clip_n ? s.clips[s.clip_n - 1] : Rect{-1e6f, -1e6f, 2e6f, 2e6f});
        break;
    }
}

bool begin_dock_panel(Context& ctx, std::string_view title, bool* open, DockPlace place, float ratio) {
    State& s = state(ctx);
    const ID pid = get_id(s, title);
    DockPanel* p = find_panel(s, pid);
    if (!p) {
        s.error = true;
        return false;
    }
    if (p->id == 0) {  // first appearance
        *p = DockPanel{};
        p->id = pid;
        p->dock = s.dock_cur;
        p->order = ++s.dock_order;
        const int k = (int)(p->order % 8);
        p->float_rect = {80.0f + 32.0f * (float)k, 80.0f + 32.0f * (float)k, 320, 240};
        p->last_frame = s.frame;
        if (s.dock_cur_root >= 0 && place != DockPlace::Floating)
            p->node = place_panel(s, s.dock_cur_root, place, ratio);
        if (p->node >= 0 && s.dock_nodes[p->node].selected == 0) s.dock_nodes[p->node].selected = pid;
    }
    const std::string_view vis = visible_label(title);
    const size_t tn = utf8_fit(vis, sizeof p->title - 1);
    if (tn) std::memcpy(p->title, vis.data(), tn);
    p->title[tn] = 0;
    p->last_frame = s.frame;
    p->closable = open != nullptr;
    if (p->close_clicked) {
        p->close_clicked = false;
        if (open) *open = false;
        return false;
    }
    if (p->node >= 0) {  // still a leaf of its dock?
        const DockNode& n = s.dock_nodes[p->node];
        if (!n.used || !is_leaf(n) || n.dock != p->dock) {
            const int root = find_root(s, p->dock);
            p->node = root >= 0 ? first_leaf(s, root) : -1;
        }
    }

    const ID tid = hash_u32(pid, 1);
    if (p->node < 0 && s.active == tid && !s.in.pointer_down && s.dock_target >= 0 &&
        s.dock_nodes[s.dock_target].dock == p->dock) {  // dropped on a zone: dock it
        const int target = s.dock_target, zone = s.dock_zone;
        int leaf = target;
        if (zone != ZoneCenter) leaf = split_leaf(s, target, zone, 0.4f);
        if (leaf < 0) leaf = target;
        p->node = leaf;
        p->order = ++s.dock_order;
        s.dock_nodes[leaf].selected = pid;
        s.active = 0;
        s.dock_target = -1;
        if (zone != ZoneCenter) {  // the new leaf has no rect yet: lay the tree out again
            const int root = find_root(s, p->dock);
            if (root >= 0) layout_node(s, root, s.dock_nodes[root].rect);
        }
        haptic(s, Haptic::Light);
    }

    if (p->node >= 0) {
        DockNode& n = s.dock_nodes[p->node];
        if (n.selected == 0) n.selected = pid;
        if (n.selected != pid) return false;
        Rect r = n.body;
        return begin_window_impl(ctx, pid, title, r, WindowScrollable | kWindowDocked, false);
    }
    const uint32_t flags = WindowDefault | WindowScrollable | (open ? WindowClosable : 0u);
    if (!begin_window_impl(ctx, pid, title, p->float_rect, flags, false)) {
        for (int i = 0; i < s.cfg.max_windows; i++)  // closed with its X: report it, and reopen next time
            if (s.wins[i].used && s.wins[i].id == pid && !s.wins[i].open) {
                s.wins[i].open = true;
                if (open) *open = false;
            }
        return false;
    }
    if (s.active == tid && s.in.pointer_down) s.dock_dragging = pid;
    return true;
}

void end_dock_panel(Context& ctx) {
    end_window(ctx);
}

}  // namespace rd
