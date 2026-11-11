// Copyright (c) 2026 Wavestorm Software
// Tilekit: a 2D level editor. Docking panels, a tile-map scene drawn with Canvas, a tree, an inspector.
#include <cmath>

#include "showcase.hpp"

namespace showcase {
namespace {
using namespace rd;

struct Type {
    FontHandle body, medium, title, small, mono;
} ty;

constexpr Color kInk = hex(0x0B0D11), kPanel = hex(0x12161C), kRaised = hex(0x1A1F27), kLine = hex(0x232A33);
constexpr Color kText = hex(0xE6EAF0), kMuted = hex(0x8A94A3), kPink = hex(0xEC4899), kTeal = hex(0x2DD4BF);

Theme setup(Context& ui, FontHandle fb, bool) {
    ty.body = font(ui, "IBMPlexSans-Regular", 14, fb);
    ty.medium = font(ui, "IBMPlexSans-Medium", 14, fb);
    ty.title = font(ui, "IBMPlexSans-SemiBold", 15, fb);
    ty.small = font(ui, "IBMPlexSans-Regular", 12, fb);
    ty.mono = font(ui, "IBMPlexMono-Regular", 12, fb);
    Theme t = Theme::dark();
    t.background = kInk;
    t.surface = kPanel;
    t.surface_hover = kRaised;
    t.surface_active = hex(0x262D37);
    t.border = kLine;
    t.text = kText;
    t.text_muted = kMuted;
    t = Theme::with_accent(t, kPink);
    t.selection = hex(0x1F3B3A);  // teal-tinted rows: selection is about the level, pink is about actions
    t.font_body = t.font_heading = ty.body;
    t.font_small = ty.small;
    t.font_large = ty.title;
    t.font_mono = ty.mono;
    t.corner_radius = 5;
    t.window_radius = 8;
    t.cell_height = 28;
    t.spacing = 6;
    t.padding = 10;
    return t;
}

// ---- pixel-art sprites, drawn from rectangles so the example needs no image files ----
enum class Sprite { Grass, Stone, Plank, Coin, Bat, Hero, Lantern, Flag, Spikes, Crate };

void sprite(Canvas& c, Sprite s, float x, float y, float T, float alpha = 1) {
    const float p = T / 16;  // one art pixel
    auto px = [&](float ax, float ay, float aw, float ah, uint32_t rgb) {
        c.fill_rect({x + ax * p, y + ay * p, aw * p, ah * p}, hex(rgb, (uint8_t)(255 * alpha)));
    };
    switch (s) {
        case Sprite::Grass:
            px(0, 0, 16, 16, 0x6B3F2A);
            px(2, 8, 2, 2, 0x5A3322), px(10, 11, 3, 2, 0x5A3322), px(6, 13, 2, 2, 0x7D4C33);
            px(0, 0, 16, 5, 0x4FB85E), px(0, 0, 16, 2, 0x86E38A), px(3, 5, 2, 2, 0x4FB85E),
                px(11, 5, 3, 1, 0x4FB85E);
            break;
        case Sprite::Stone:
            px(0, 0, 16, 16, 0x464B66), px(0, 0, 16, 1, 0x5E6485), px(0, 7, 16, 1, 0x34384F);
            px(7, 0, 1, 7, 0x34384F), px(3, 8, 1, 8, 0x34384F), px(12, 8, 1, 8, 0x34384F);
            break;
        case Sprite::Plank:
            px(0, 3, 16, 9, 0xB7773F), px(0, 3, 16, 2, 0xD69456), px(0, 10, 16, 2, 0x8A5528);
            px(2, 6, 1, 1, 0x5C3418), px(13, 6, 1, 1, 0x5C3418);
            break;
        case Sprite::Coin:
            px(5, 3, 6, 10, 0xC98A12), px(4, 4, 8, 8, 0xC98A12), px(5, 4, 6, 8, 0xFFD24A),
                px(6, 5, 2, 5, 0xFFF0A0);
            break;
        case Sprite::Bat:
            px(6, 6, 4, 4, 0x6A4C93), px(1, 5, 5, 2, 0x8460B5), px(10, 5, 5, 2, 0x8460B5),
                px(2, 7, 3, 1, 0x8460B5);
            px(11, 7, 3, 1, 0x8460B5), px(7, 7, 1, 1, 0xFFD1E6), px(9, 7, 1, 1, 0xFFD1E6);
            break;
        case Sprite::Hero:
            px(5, 2, 6, 5, 0x2DD4BF), px(6, 4, 4, 3, 0xFFE2C6), px(7, 5, 1, 1, 0x1B1F2A),
                px(9, 5, 1, 1, 0x1B1F2A);
            px(5, 7, 6, 2, 0xEC4899), px(5, 9, 6, 4, 0x1E8C80), px(5, 13, 2, 3, 0x2A2F3A),
                px(9, 13, 2, 3, 0x2A2F3A);
            break;
        case Sprite::Lantern:
            px(7, 4, 2, 12, 0x2A2F3A), px(5, 1, 6, 5, 0x2A2F3A), px(6, 2, 4, 3, 0xFFC27A);
            break;
        case Sprite::Flag:
            px(3, 1, 1, 15, 0xD5D9E2), px(4, 1, 8, 5, 0xEC4899), px(4, 1, 8, 1, 0xF77AB8);
            break;
        case Sprite::Spikes:
            for (int i = 0; i < 4; i++) {
                const float sx = x + (float)i * 4 * p;
                c.triangle({sx, y + T}, {sx + 2 * p, y + 8 * p}, {sx + 4 * p, y + T}, hex(0xC9CDD8));
            }
            break;
        case Sprite::Crate:
            px(1, 3, 14, 13, 0x9C6233), px(1, 3, 14, 2, 0xC27E45), px(3, 7, 10, 1, 0x6E4220),
                px(3, 11, 10, 1, 0x6E4220);
            break;
    }
}

void glow(Canvas& c, Vec2 at, float r, Color col) {
    for (int i = 5; i >= 1; i--)
        c.circle(at, r * (float)i / 5, Color{col.r, col.g, col.b, (uint8_t)(18 + 6 * (5 - i))});
}

void scene(Context& ui) {
    layout_row(ui, 1, nullptr, 0, -1);
    Canvas c = canvas_begin(ui, {0, 0}, kInk);
    const float W = c.rect.w, H = c.rect.h, T = 40;
    // Dusk sky in bands, a sun, and two layers of hills.
    const Color top = hex(0x161A33), mid = hex(0x4A2A5E), low = hex(0xD9567F), horizon = hex(0xF7A26B);
    for (int i = 0; i < 48; i++) {
        const float f = (float)i / 47;
        const Color col = f < 0.5f ? mix(top, mid, f * 2)
                                   : (f < 0.85f ? mix(mid, low, (f - 0.5f) / 0.35f)
                                                : mix(low, horizon, (f - 0.85f) / 0.15f));
        c.fill_rect({0, H * 0.72f * f, W, H * 0.72f / 47 + 1}, col);
    }
    c.fill_rect({0, H * 0.72f, W, H}, hex(0x2B1838));
    glow(c, {W * 0.68f, H * 0.58f}, 120, hex(0xFFB27A));
    c.circle({W * 0.68f, H * 0.58f}, 46, hex(0xFFD1A1));
    for (int i = 0; i < 9; i++) {
        const float bx = (float)i * W / 7 - 80;
        c.triangle({bx, H * 0.76f}, {bx + 150, H * 0.46f + (float)(i % 3) * 22}, {bx + 300, H * 0.76f},
                   hex(0x3A2150));
    }
    for (int i = 0; i < 12; i++) {
        const float bx = (float)i * W / 10 - 60;
        c.triangle({bx, H * 0.8f}, {bx + 90, H * 0.6f + (float)(i % 4) * 14}, {bx + 180, H * 0.8f},
                   hex(0x2A1740));
    }
    for (int i = 0; i < 26; i++)  // stars
        c.circle({std::fmod((float)i * 137.0f, W), std::fmod((float)i * 71.0f, H * 0.35f)},
                 i % 3 ? 1.0f : 1.6f, hex(0xFFFFFF, i % 2 ? 140 : 200));

    // The level: ground heights per column (0 = a pit with spikes).
    static const int ground[] = {4, 4, 4, 5, 5, 5, 0, 0, 3, 3, 3, 3, 6, 6, 6, 0, 0, 0, 0, 0, 4, 4, 4,
                                 4, 5, 5, 7, 7, 7, 4, 4, 4, 4, 4, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3};
    const int cols = (int)(W / T) + 1, rows = (int)(H / T) + 1;
    const float oy = H - (float)rows * T;  // align the grid to the bottom edge
    for (int x = 0; x < cols; x++) {
        const int g = ground[x % 45];
        if (g == 0) {
            sprite(c, Sprite::Spikes, (float)x * T, oy + (float)(rows - 1) * T, T);
            continue;
        }
        for (int y = rows - g; y < rows; y++)
            sprite(c, y == rows - g ? Sprite::Grass : Sprite::Stone, (float)x * T, oy + (float)y * T, T);
    }
    auto at = [&](int tx, int ty) { return Vec2{(float)tx * T, oy + (float)(rows - ty) * T}; };
    // Props, pickups and enemies.
    sprite(c, Sprite::Flag, at(2, 5).x, at(2, 5).y, T);
    sprite(c, Sprite::Hero, at(4, 6).x, at(4, 6).y, T);
    sprite(c, Sprite::Crate, at(9, 4).x, at(9, 4).y, T);
    for (int i = 0; i < 4; i++) sprite(c, Sprite::Coin, at(16 + i, 9).x, at(16 + i, 9).y, T);
    for (int i = 0; i < 3; i++) sprite(c, Sprite::Coin, at(25 + i, 10).x, at(25 + i, 10).y, T);
    const Vec2 lamp1 = at(13, 7), lamp2 = at(22, 5);
    glow(c, {lamp1.x + T / 2, lamp1.y + T * 0.2f}, 46, hex(0xFFC27A));
    glow(c, {lamp2.x + T / 2, lamp2.y + T * 0.2f}, 46, hex(0xFFC27A));
    sprite(c, Sprite::Lantern, lamp1.x, lamp1.y, T);
    sprite(c, Sprite::Lantern, lamp2.x, lamp2.y, T);
    sprite(c, Sprite::Bat, at(18, 12).x, at(18, 12).y, T);
    sprite(c, Sprite::Bat, at(29, 11).x, at(29, 11).y, T);
    // The moving platform being edited: its path, its end position as a ghost, a selection frame.
    const Vec2 a = at(15, 8), b = at(19, 8);
    for (float d = a.x + 3 * T; d < b.x; d += 12)
        c.line({d, a.y + T / 2}, {d + 6, a.y + T / 2}, hex(0x2DD4BF, 200), 2);
    for (int i = 0; i < 3; i++) sprite(c, Sprite::Plank, b.x + (float)i * T, b.y, T, 0.35f);
    c.circle({b.x + 1.5f * T, b.y + T / 2}, 4, kTeal);
    for (int i = 0; i < 3; i++) sprite(c, Sprite::Plank, a.x + (float)i * T, a.y, T);
    const Rect sel{a.x - 3, a.y + T * 0.1f, 3 * T + 6, T * 0.75f};
    c.stroke_rect(sel, kTeal, 2, 3);
    for (Vec2 h : {Vec2{sel.x, sel.y}, Vec2{sel.x + sel.w, sel.y}, Vec2{sel.x, sel.y + sel.h},
                   Vec2{sel.x + sel.w, sel.y + sel.h}})
        c.fill_rect({h.x - 4, h.y - 4, 8, 8}, kTeal, 2);
    // Grid and the camera bounds.
    for (int x = 0; x <= cols; x++) c.line({(float)x * T, 0}, {(float)x * T, H}, hex(0xFFFFFF, 12), 1);
    for (int y = 0; y <= rows; y++)
        c.line({0, oy + (float)y * T}, {W, oy + (float)y * T}, hex(0xFFFFFF, 12), 1);
    const Rect cam{T * 1.5f, T * 1.0f, W * 0.62f, H * 0.62f};
    for (float d = cam.x; d < cam.x + cam.w; d += 16) {
        c.line({d, cam.y}, {d + 8, cam.y}, hex(0xFFFFFF, 90), 1);
        c.line({d, cam.y + cam.h}, {d + 8, cam.y + cam.h}, hex(0xFFFFFF, 90), 1);
    }
    for (float d = cam.y; d < cam.y + cam.h; d += 16) {
        c.line({cam.x, d}, {cam.x, d + 8}, hex(0xFFFFFF, 90), 1);
        c.line({cam.x + cam.w, d}, {cam.x + cam.w, d + 8}, hex(0xFFFFFF, 90), 1);
    }
    canvas_end(ui);
}

void top_bar(Context& ui, int w) {
    Rect r{0, 0, (float)w, 52};
    if (!begin_window(ui, "##top", r, WindowNone)) return;
    static int mode = 0, tool = 1;
    const float cols[] = {26, 70, -475, 34, 34, 34, 34, 34, 22, 150, 84};  // fill, then 474 px
    layout_row(ui, 11, cols, 11, 32);
    Canvas logo = canvas_begin(ui, {26, 26});
    logo.fill_rect({0, 3, 26, 26}, kPink, 7);
    logo.fill_rect({6, 9, 6, 6}, hex(0xFFFFFF), 1.5f),
        logo.fill_rect({14, 9, 6, 6}, hex(0xFFFFFF, 160), 1.5f);
    logo.fill_rect({6, 17, 6, 6}, hex(0xFFFFFF, 160), 1.5f),
        logo.fill_rect({14, 17, 6, 6}, hex(0xFFFFFF), 1.5f);
    canvas_end(ui);
    text(ui, "Tilekit", ty.title, kText);
    static const char* const path[] = {"Skyward", "World 2", "Cliffs at dusk"};
    breadcrumbs(ui, "##path", path, 3);
    static const Icon tools[] = {Icon::Grid, Icon::Pencil, Icon::Copy, Icon::Trash, Icon::Search};
    static const char* const names[] = {"Select", "Paint", "Stamp", "Erase", "Zoom"};
    for (int i = 0; i < 5; i++) {
        if (icon_button(ui, names[i], tools[i], tool == i).clicked) tool = i;
        tooltip(ui, names[i]);
    }
    label(ui, "");
    static const char* const modes[] = {"Edit", "Simulate"};
    segmented(ui, "##mode", mode, modes, 2);
    if (button(ui, "Play", ButtonStyle::Primary).clicked)
        toast(ui, "Playing Cliffs at dusk from the flag", Tone::Info);
    end_window(ui);
}

void hierarchy(Context& ui) {
    static char find_text[64];
    static TextBuffer find{find_text, sizeof find_text, 0};
    layout_row(ui, 1, nullptr, 0, 0);
    search_box(ui, "##find", find, "Find in level");
    spacing(ui, 4);
    static bool open[5] = {true, true, true, false, false};
    static int selected = 2;
    struct Node {
        const char* name;
        int group;
        Icon icon;
    };
    static const Node nodes[] = {{"Player start", -1, Icon::User},
                                 {"Moving platform 01", 1, Icon::List},
                                 {"Moving platform 02", 1, Icon::List},
                                 {"Crumbling ledge", 1, Icon::List},
                                 {"Bat", 2, Icon::Heart},
                                 {"Bat", 2, Icon::Heart},
                                 {"Coins (24)", 3, Icon::Star},
                                 {"Lanterns (6)", 4, Icon::Sun}};
    static const char* const groups[] = {"Terrain", "Platforms", "Enemies", "Pickups", "Lights"};
    if (list_item(ui, nodes[0].name, selected == 0, nodes[0].icon).clicked) selected = 0;
    for (int g = 0; g < 5; g++) {
        push_id(ui, groups[g]);
        if (tree_node(ui, groups[g], open[g])) {
            for (int i = 0; i < 8; i++) {
                if (nodes[i].group != g) continue;
                push_id(ui, i == 4 ? "a" : (i == 5 ? "b" : ""));
                if (list_item(ui, nodes[i].name, selected == i, nodes[i].icon).clicked) selected = i;
                pop_id(ui);
            }
            if (g == 0) label_colored(ui, "4 tile layers", kMuted);
            tree_pop(ui);
        }
        pop_id(ui);
    }
}

// A muted name in an 84 px column; the caller adds the value widget(s) in the rest of the row.
void field(Context& ui, const char* name) {
    const float cols[] = {84, -1};
    layout_row(ui, 2, cols, 2, 0);
    label_colored(ui, name, kMuted);
}

void inspector(Context& ui) {
    static float px = 15, py = 8, speed = 2.4f, wait = 0.5f, rot = 0;
    static int motion = 1, layer = 1;
    static bool ease = true, carry = true, oneway = false;
    static Color tint = hex(0xFFE2B8);
    layout_row(ui, 1, nullptr, 0, 0);
    text(ui, "Moving platform 02", ty.title, kText);
    label_colored(ui, "Prefab: platform_wood_3", kMuted);
    spacing(ui, 4);
    separator_text(ui, "Transform");
    field(ui, "Position");
    layout_begin_column(ui);
    layout_row(ui, 2, nullptr, 0, 0);
    drag_float(ui, "##px", px, 0.1f);
    drag_float(ui, "##py", py, 0.1f);
    layout_end_column(ui);
    field(ui, "Rotation");
    slider(ui, "##rot", rot, -180, 180, "%.0f°");
    const float one[] = {84, -1};
    separator_text(ui, "Path");
    layout_row(ui, 1, nullptr, 0, 0);
    static const char* const motions[] = {"Loop", "Ping-pong", "Once"};
    segmented(ui, "##motion", motion, motions, 3);
    layout_row(ui, 2, one, 2, 0);
    label_colored(ui, "Speed", kMuted);
    slider(ui, "##speed", speed, 0, 6, "%.1f t/s");
    label_colored(ui, "Wait", kMuted);
    slider(ui, "##wait", wait, 0, 3, "%.1f s");
    layout_row(ui, 1, nullptr, 0, 0);
    toggle(ui, "Ease in and out", ease);
    toggle(ui, "Carry the player", carry);
    toggle(ui, "One-way from below", oneway);
    separator_text(ui, "Look");
    const float swatch[] = {84, 28, -1};
    layout_row(ui, 3, swatch, 3, 0);
    label_colored(ui, "Tint", kMuted);
    color_button(ui, "##tint", tint);
    label(ui, "#FFE2B8");
    layout_row(ui, 2, one, 2, 0);
    label_colored(ui, "Layer", kMuted);
    static const char* const layers[] = {"Background", "Gameplay", "Foreground"};
    combo(ui, "##layer", layer, layers, 3);
    layout_row(ui, 1, nullptr, 0, 0);
    flow_begin(ui);
    chip(ui, "solid", true);
    chip(ui, "moving", true);
    chip(ui, "wood", false);
    chip(ui, "+ tag", false);
    flow_end(ui);
}

void assets(Context& ui) {
    static int picked = 2;
    static const Sprite kinds[] = {Sprite::Grass, Sprite::Stone,   Sprite::Plank, Sprite::Crate,
                                   Sprite::Coin,  Sprite::Lantern, Sprite::Flag,  Sprite::Spikes,
                                   Sprite::Bat,   Sprite::Hero};
    static const char* const names[] = {"grass",   "stone", "plank",  "crate", "coin",
                                        "lantern", "flag",  "spikes", "bat",   "hero"};
    const float head[] = {-1, 200};
    layout_row(ui, 2, head, 2, 0);
    static const char* const tabs[] = {"Tiles", "Props", "Enemies", "Audio"};
    static int tab = 0;
    tab_bar(ui, "##assettabs", tab, tabs, 4);
    static char filter_text[32];
    static TextBuffer filter{filter_text, sizeof filter_text, 0};
    search_box(ui, "##filter", filter, "Filter");
    layout_grid(ui, 76, 92);  // ten tiles fit one row of a 1440 px window
    for (int i = 0; i < 10; i++) {
        layout_begin_column(ui);
        layout_row(ui, 1, nullptr, 0, 64);
        Canvas c = canvas_begin(ui, {0, 64}, i == picked ? hex(0x1F3B3A) : kRaised);
        if (c.response.clicked) picked = i;
        const float x0 = (c.rect.w - 48) / 2;
        sprite(c, kinds[i], x0, 8, 48);
        if (i == picked) c.stroke_rect({1, 1, c.rect.w - 2, 62}, kTeal, 2, 4);
        canvas_end(ui);
        layout_row(ui, 1, nullptr, 0, 18);
        text(ui, names[i], ty.small, i == picked ? kText : kMuted);
        layout_end_column(ui);
    }
}

void status_bar(Context& ui, int w, int h) {
    Rect r{0, (float)h - 28, (float)w, 28};
    if (!begin_window(ui, "##status", r, WindowNone)) return;
    const float cols[] = {16, 150, 130, 120, -77, 70};
    layout_row(ui, 6, cols, 6, 16);
    Canvas dot = canvas_begin(ui, {10, 16});
    dot.circle({5, 8}, 4, hex(0x3DD68C));
    canvas_end(ui);
    text(ui, "Saved 2 min ago", ty.mono, kMuted);
    text(ui, "Tile 34, 12", ty.mono, kMuted);
    text(ui, "Zoom 200%", ty.mono, kMuted);
    label(ui, "");
    text(ui, "60 fps", ty.mono, kMuted);
    end_window(ui);
}

void frame(Context& ui, const Input&, int w, int h) {
    top_bar(ui, w);
    status_bar(ui, w, h);
    dock_space(ui, "dock", {0, 52, (float)w, (float)h - 80});
    if (begin_dock_panel(ui, "Assets", nullptr, DockPlace::Bottom, 0.27f)) {
        assets(ui);
        end_dock_panel(ui);
    }
    if (begin_dock_panel(ui, "Hierarchy", nullptr, DockPlace::Left, 0.17f)) {
        hierarchy(ui);
        end_dock_panel(ui);
    }
    if (begin_dock_panel(ui, "Inspector", nullptr, DockPlace::Right, 0.22f)) {
        inspector(ui);
        end_dock_panel(ui);
    }
    if (begin_dock_panel(ui, "Scene")) {
        scene(ui);
        end_dock_panel(ui);
    }
}
}  // namespace

const App tilekit = {setup, frame, "Tilekit"};

}  // namespace showcase
