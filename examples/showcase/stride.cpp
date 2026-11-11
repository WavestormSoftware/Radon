// Copyright (c) 2026 Wavestorm Software
// Stride: a running app for phones. A cobalt hero over a white sheet, a progress ring, a bar chart, a
// training plan with steps, a route map drawn with Canvas, and a bottom navigation bar.
#include "showcase.hpp"

namespace showcase {
namespace {
using namespace rd;

struct Type {
    FontHandle body, medium, small, title, big, display;
} ty;

constexpr Color kCobalt = hex(0x2448FF), kCobaltHi = hex(0x4766FF), kSun = hex(0xFFD84D),
                kInk = hex(0x0D1430);
constexpr Color kWhite = hex(0xFFFFFF), kHaze = hex(0xC9D3FF), kMuted = hex(0x67708A);

Theme sheet;  // the app theme: its background is the cobalt behind everything
Theme paper;  // inside the white sheet: tracks (segmented, progress) on a pale grey
Theme hero;   // white on cobalt

Theme setup(Context& ui, FontHandle fb, bool) {
    ty.body = font(ui, "IBMPlexSans-Regular", 15, fb);
    ty.medium = font(ui, "IBMPlexSans-Medium", 15, fb);
    ty.small = font(ui, "IBMPlexSans-Regular", 13, fb);
    ty.title = font(ui, "BricolageGrotesque-Bold", 22, fb);
    ty.big = font(ui, "BricolageGrotesque-Bold", 28, fb);
    ty.display = font(ui, "BricolageGrotesque-Bold", 60, fb);
    Theme t = Theme::light();
    t.background = kCobalt;
    t.surface = kWhite;
    t.surface_hover = hex(0xF1F3FA);
    t.surface_active = hex(0xE2E7F5);
    t.border = hex(0xE3E7F1);
    t.text = kInk;
    t.text_muted = kMuted;
    t = Theme::with_accent(t, kCobalt);
    t.chart[0] = kCobalt;
    t.font_body = t.font_heading = ty.body;
    t.font_small = ty.small;
    t.font_large = ty.title;
    t = Theme::touch(t);
    t.corner_radius = 12;
    t.window_radius = 28;
    t.padding = 20;
    t.spacing = 10;
    sheet = t;
    paper = t;
    paper.background = hex(0xEEF1F8);
    paper.spacing = 8;
    hero = t;
    hero.surface = kCobalt;
    hero.surface_hover = kCobaltHi;
    hero.surface_active = hex(0x3E5DFF);
    hero.border = kCobaltHi;
    hero.text = kWhite;
    hero.text_muted = kHaze;
    hero.accent = hero.focus_ring = kSun;
    hero.accent_text = kInk;
    return sheet;
}

// Two lines set tight: a value over its caption (stats, list rows, greetings).
void stack(Context& ui, const char* top, FontHandle tf, Color tc, float th, const char* bottom, Color bc) {
    Theme t = ui.theme();
    t.spacing = 2;
    push_theme(ui, t);
    layout_begin_column(ui);
    layout_row(ui, 1, nullptr, 0, th);
    text(ui, top, tf, tc);
    layout_row(ui, 1, nullptr, 0, 18);
    text(ui, bottom, ty.small, bc);
    layout_end_column(ui);
    pop_theme(ui);
}

void nav(Context& ui, int w, int h, int selected) {
    Rect r{0, (float)h - 76, (float)w, 76};
    if (!begin_window(ui, "##nav", r, WindowNone)) return;
    static const Icon icons[] = {Icon::Home, Icon::Calendar, Icon::Pin, Icon::User};
    static const char* const names[] = {"Today", "Plan", "Routes", "You"};
    layout_row(ui, 1, nullptr, 0, 52);
    nav_bar(ui, "##nav", selected, icons, names, 4);
    end_window(ui);
}

// An icon in a soft cobalt circle (list leading icons).
void badge_icon(Context& ui, Icon icon) {
    Canvas c = canvas_begin(ui, {44, 44});
    c.circle({22, 22}, 22, hex(0xE8ECFF));
    canvas_end(ui);
    draw_icon(ui, icon, {c.rect.x + 10, c.rect.y + 10, 24, 24}, kCobalt);
}

void today(Context& ui, const Input&, int w, int h) {
    const float W = (float)w;
    push_theme(ui, hero);
    Rect top{0, 0, W, 340};
    if (begin_window(ui, "##hero", top, WindowNone)) {
        const float head[] = {44, -53, 44};
        layout_row(ui, 3, head, 3, 44);
        avatar(ui, "Noor Haddad", kSun);
        stack(ui, "Good morning, Noor", ty.medium, kWhite, 22, "Thursday, 12 November", kHaze);
        icon_button(ui, "Notifications", Icon::Bell);
        spacing(ui, 14);
        const float ring[] = {128, -1};
        layout_row(ui, 2, ring, 2, 128);
        progress_ring(ui, 32.4f / 40);
        layout_begin_column(ui);
        layout_row(ui, 1, nullptr, 0, 70);
        text(ui, "32.4", ty.display, kWhite);
        layout_row(ui, 1, nullptr, 0, 20);
        text(ui, "of 40 km this week", ty.body, kHaze);
        layout_row(ui, 1, nullptr, 0, 22);
        text(ui, "4.1 km more than last week", ty.medium, kSun);
        layout_end_column(ui);
        spacing(ui, 10);
        layout_row(ui, 3, nullptr, 0, 52);
        stack(ui, "5:12", ty.big, kWhite, 32, "avg pace per km", kHaze);
        stack(ui, "2h 48m", ty.big, kWhite, 32, "moving time", kHaze);
        stack(ui, "4", ty.big, kWhite, 32, "runs", kHaze);
        end_window(ui);
    }
    pop_theme(ui);

    push_theme(ui, paper);
    Rect body{0, 314, W, (float)h - 314 + 40};  // the bottom corners hide under the nav bar
    if (begin_window(ui, "##sheet", body, WindowNone)) {
        static int unit = 0;
        static const char* const units[] = {"Distance", "Time"};
        const float head[] = {-171, 170};
        layout_row(ui, 2, head, 2, 40);
        text(ui, "This week", ty.title, kInk);
        segmented(ui, "##unit", unit, units, 2);
        static const float km[] = {6.2f, 0, 9.8f, 8.2f, 0, 0, 0};
        static const float mins[] = {33, 0, 52, 41, 0, 0, 0};
        static const char* const days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        const ChartSeries s{unit ? "Minutes" : "Kilometres", unit ? mins : km, 7, kCobalt};
        layout_row(ui, 1, nullptr, 0, 0);
        chart(ui, "##week", ChartType::Bar, &s, 1, days, 150);
        layout_row(ui, 1, nullptr, 0, 34);
        text(ui, "Recent runs", ty.title, kInk);
        struct Run {
            const char* name;
            const char* detail;
            Icon icon;
            float pace[8];
        };
        static const Run runs[] = {
            {"Canal tempo", "8.2 km in 41:06", Icon::Pin, {5.4f, 5.1f, 5.0f, 4.9f, 5.0f, 4.8f, 4.9f, 5.1f}},
            {"Hill repeats",
             "9.8 km in 54:20",
             Icon::ArrowUp,
             {6.0f, 5.2f, 6.1f, 5.1f, 6.0f, 5.0f, 5.9f, 5.6f}},
            {"Easy loop",
             "6.2 km in 34:48",
             Icon::Refresh,
             {5.7f, 5.6f, 5.6f, 5.5f, 5.6f, 5.5f, 5.6f, 5.5f}}};
        const float row[] = {44, -91, 90};
        for (const Run& r : runs) {
            push_id(ui, r.name);
            layout_row(ui, 3, row, 3, 48);
            badge_icon(ui, r.icon);
            stack(ui, r.name, ty.medium, kInk, 22, r.detail, kMuted);
            sparkline(ui, r.pace, 8, kCobalt);
            pop_id(ui);
        }
        end_window(ui);
    }
    nav(ui, w, h, 0);
    pop_theme(ui);
}

void route(Context& ui, float h) {
    layout_row(ui, 1, nullptr, 0, h);
    Canvas c = canvas_begin(ui, {0, h});
    const float W = c.rect.w;
    c.fill_rect({0, 0, W, h}, hex(0xEEF1F6), 16);
    c.fill_rect({W * 0.05f, h * 0.1f, W * 0.32f, h * 0.38f}, hex(0xD3EBD6), 18);  // parks
    c.fill_rect({W * 0.62f, h * 0.58f, W * 0.3f, h * 0.32f}, hex(0xD3EBD6), 18);
    c.line({-10, h * 0.95f}, {W + 10, h * 0.35f}, hex(0xC9DCFF), 22);  // the river
    for (int i = 0; i < 6; i++) {                                      // streets
        const float x = W * (0.08f + 0.17f * (float)i);
        c.line({x, 0}, {x + 30, h}, hex(0xDDE2EC), 9);
        c.line({x, 0}, {x + 30, h}, kWhite, 6);
    }
    for (int i = 0; i < 3; i++) {
        const float y = h * (0.22f + 0.28f * (float)i);
        c.line({0, y}, {W, y - 18}, hex(0xDDE2EC), 9);
        c.line({0, y}, {W, y - 18}, kWhite, 6);
    }
    const Vec2 path[] = {{W * 0.12f, h * 0.78f}, {W * 0.2f, h * 0.62f}, {W * 0.35f, h * 0.58f},
                         {W * 0.42f, h * 0.36f}, {W * 0.58f, h * 0.3f}, {W * 0.66f, h * 0.5f},
                         {W * 0.8f, h * 0.44f},  {W * 0.86f, h * 0.22f}};
    for (int pass = 0; pass < 2; pass++)
        for (int i = 1; i < 8; i++) {
            c.line(path[i - 1], path[i], pass ? kCobalt : kWhite, pass ? 5.0f : 10.0f);
            c.circle(path[i], pass ? 2.5f : 5.0f, pass ? kCobalt : kWhite);
        }
    c.circle(path[0], 8, kWhite), c.circle(path[0], 5, hex(0x12A150));
    c.circle(path[7], 9, kWhite), c.circle(path[7], 6, kCobalt);
    canvas_end(ui);
}

void plan(Context& ui, const Input&, int w, int h) {
    const float W = (float)w;
    push_theme(ui, hero);
    Rect top{0, 0, W, 248};
    if (begin_window(ui, "##hero", top, WindowNone)) {
        const float head[] = {44, -53, 44};
        layout_row(ui, 3, head, 3, 44);
        icon_button(ui, "Back", Icon::ChevronLeft);
        label(ui, "");
        icon_button(ui, "More", Icon::Dots);
        layout_row(ui, 1, nullptr, 0, 36);
        text(ui, "Spring half marathon", ty.big, kWhite);
        layout_row(ui, 1, nullptr, 0, 22);
        text(ui, "Race day in 6 weeks, on Sunday 22 March", ty.body, kHaze);
        spacing(ui, 10);
        static const char* const phases[] = {"Base", "Build", "Peak", "Taper"};
        layout_row(ui, 1, nullptr, 0, 36);
        steps(ui, "##phase", phases, 4, 1);
        end_window(ui);
    }
    pop_theme(ui);

    push_theme(ui, paper);
    Rect body{0, 224, W, (float)h - 224 + 40};
    if (begin_window(ui, "##sheet", body, WindowNone)) {
        const float head[] = {-111, 110};
        layout_row(ui, 2, head, 2, 34);
        text(ui, "Today: hill repeats", ty.title, kInk);
        badge(ui, "Build, week 3", kCobalt);
        route(ui, 176);
        layout_row(ui, 3, nullptr, 0, 48);
        const char* const values[] = {"9.6 km", "6 x 400 m", "+148 m"};
        const char* const captions[] = {"distance", "repeats", "climb"};
        for (int i = 0; i < 3; i++) {
            stack(ui, values[i], ty.medium, kInk, 22, captions[i], kMuted);
        }
        static int view = 0;
        static const char* const views[] = {"Target splits", "Heart rate"};
        layout_row(ui, 1, nullptr, 0, 0);
        segmented(ui, "##view", view, views, 2);
        static const char* const pace[] = {"5:40", "4:25", "4:20", "5:35"};
        static const float effort[] = {0.45f, 0.88f, 0.92f, 0.5f};
        static const char* const name[] = {"Warm-up 2 km", "Repeats 1-3", "Repeats 4-6", "Cool-down 2 km"};
        const float cols[] = {124, -61, 60};
        for (int i = 0; i < 4; i++) {
            push_id(ui, name[i]);
            layout_row(ui, 3, cols, 3, 26);
            text(ui, name[i], ty.small, kMuted);
            progress_bar(ui, effort[i]);
            text(ui, pace[i], ty.medium, kInk);
            pop_id(ui);
        }
        layout_row(ui, 1, nullptr, 0, 48);
        if (button(ui, "Start session", ButtonStyle::Primary).clicked)
            toast(ui, "Session started", Tone::Success);
        end_window(ui);
    }
    nav(ui, w, h, 1);
    pop_theme(ui);
}

}  // namespace

const App stride_today = {setup, today, "Stride"};
const App stride_plan = {setup, plan, "Stride plan"};

}  // namespace showcase
