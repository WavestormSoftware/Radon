// Copyright (c) 2026 Wavestorm Software
// Live ops: a dashboard for a game's servers. Sidebar navigation, an area chart, a donut, KPIs with
// sparklines and a sortable table, in a light and a dark palette.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>

#include "showcase.hpp"

namespace showcase {
namespace {
using namespace rd;

struct Type {
    FontHandle body, medium, title, kpi, big, small, mono;
} ty;

struct Palette {
    Color bg, card, hover, active, line, text, muted, accent, good, warn, bad;
} pal;

Theme base;          // sidebar: white surface
Theme canvas_theme;  // main area: grey surface, white cards

Theme setup(Context& ui, FontHandle fb, bool dark) {
    ty.body = font(ui, "IBMPlexSans-Regular", 14, fb);
    ty.medium = font(ui, "IBMPlexSans-Medium", 14, fb);
    ty.title = font(ui, "IBMPlexSans-SemiBold", 26, fb);
    ty.kpi = font(ui, "IBMPlexSans-SemiBold", 22, fb);
    ty.big = font(ui, "IBMPlexSans-SemiBold", 40, fb);
    ty.small = font(ui, "IBMPlexSans-Regular", 12, fb);
    ty.mono = font(ui, "IBMPlexMono-Regular", 13, fb);
    if (dark)
        pal = {hex(0x0B0C0F), hex(0x15171C), hex(0x1C1F26), hex(0x272B33), hex(0x242830), hex(0xE8EAEE),
               hex(0x8B93A1), hex(0x8B73FF), hex(0x3DD68C), hex(0xF2B648), hex(0xFF6369)};
    else
        pal = {hex(0xEEF0F3), hex(0xFFFFFF), hex(0xF5F6F8), hex(0xE7E9EE), hex(0xE3E6EB), hex(0x14171C),
               hex(0x667085), hex(0x6D4AFF), hex(0x12A150), hex(0xD98E04), hex(0xE5484D)};
    Theme t = dark ? Theme::dark() : Theme::light();
    t.background = pal.bg;
    t.surface = pal.card;
    t.surface_hover = pal.hover;
    t.surface_active = pal.active;
    t.border = pal.line;
    t.text = pal.text;
    t.text_muted = pal.muted;
    t.success = pal.good, t.warning = pal.warn, t.error = pal.bad;
    t = Theme::with_accent(t, pal.accent);
    t.selection = mix(pal.card, pal.accent, dark ? 0.22f : 0.12f);
    if (dark) t.accent_text = pal.bg;  // dark text on the bright badges and buttons: 3:1 or better
    const Color chart[8] = {pal.accent,
                            dark ? hex(0x2DC6D6) : hex(0x0E9FB0),
                            dark ? hex(0xF5B952) : hex(0xEA9A1E),
                            dark ? hex(0xFF7A70) : hex(0xEC5B50),
                            hex(0x8B95A7),
                            pal.good,
                            hex(0xC084FC),
                            hex(0x60A5FA)};
    for (int i = 0; i < 8; i++) t.chart[i] = chart[i];
    t.font_body = t.font_heading = ty.body;
    t.font_small = ty.small;
    t.font_large = ty.kpi;
    t.font_mono = ty.mono;
    t.corner_radius = 7;
    t.window_radius = 12;
    t.padding = 16;
    t.spacing = 8;
    t.cell_height = 30;
    t.shadows = false;  // flat cards with hairlines; depth comes from the grey canvas
    base = t;
    canvas_theme = t;
    canvas_theme.surface = pal.bg;
    canvas_theme.surface_hover = pal.card;
    canvas_theme.padding = 20;
    canvas_theme.spacing = 12;
    return t;
}

void stack(Context& ui, const char* top, FontHandle tf, Color tc, float th, const char* bottom, FontHandle bf,
           Color bc) {
    Theme t = ui.theme();
    t.spacing = 2;
    push_theme(ui, t);
    layout_begin_column(ui);
    layout_row(ui, 1, nullptr, 0, th);
    text(ui, top, tf, tc);
    layout_row(ui, 1, nullptr, 0, 18);
    text(ui, bottom, bf, bc);
    layout_end_column(ui);
    pop_theme(ui);
}

void sidebar(Context& ui, int h) {
    Rect r{0, 0, 236, (float)h};
    if (!begin_window(ui, "##side", r, WindowNone)) return;
    const float brand[] = {30, -1};
    layout_row(ui, 2, brand, 2, 32);
    Canvas logo = canvas_begin(ui, {30, 30});
    logo.fill_rect({0, 1, 30, 30}, pal.accent, 8);
    const Vec2 pulse[] = {{6, 17}, {11, 17}, {14, 9}, {18, 23}, {21, 15}, {24, 15}};
    for (int i = 1; i < 6; i++) logo.line(pulse[i - 1], pulse[i], hex(0xFFFFFF), 2.2f);
    canvas_end(ui);
    text(ui, "Live ops", ty.kpi, pal.text);
    spacing(ui, 8);
    static int env = 0;
    static const char* const envs[] = {"Skyward: production", "Skyward: staging", "Skyward: playtest"};
    layout_row(ui, 1, nullptr, 0, 0);
    combo(ui, "##env", env, envs, 3);
    spacing(ui, 8);
    static int page = 0;
    struct Item {
        const char* name;
        Icon icon;
        int count;
    };
    static const Item items[] = {{"Overview", Icon::Home, 0},    {"Players", Icon::User, 0},
                                 {"Matches", Icon::Grid, 0},     {"Economy", Icon::Bookmark, 0},
                                 {"Incidents", Icon::Bell, 2},   {"Builds", Icon::Archive, 0},
                                 {"Settings", Icon::Settings, 0}};
    const float with_count[] = {-37, 28};
    for (int i = 0; i < 7; i++) {
        if (items[i].count) {
            layout_row(ui, 2, with_count, 2, 0);
            if (list_item(ui, items[i].name, page == i, items[i].icon).clicked) page = i;
            badge(ui, "2", pal.bad);
        } else {
            layout_row(ui, 1, nullptr, 0, 0);
            if (list_item(ui, items[i].name, page == i, items[i].icon).clicked) page = i;
        }
    }
    end_window(ui);
    Rect who{0, (float)h - 76, 236, 76};
    if (begin_window(ui, "##who", who, WindowNone)) {
        const float cols[] = {40, -1};
        layout_row(ui, 2, cols, 2, 40);
        avatar(ui, "Rin Takeda", hex(0x0E9FB0));
        stack(ui, "Rin Takeda", ty.medium, pal.text, 20, "On call until 18:00", ty.small, pal.muted);
        end_window(ui);
    }
}

void card_title(Context& ui, const char* title, const char* note) {
    const float cols[] = {-201, 200};
    layout_row(ui, 2, cols, 2, 22);
    text(ui, title, ty.medium, pal.text);
    text(ui, note, ty.small, pal.muted);
}

void players(Context& ui) {
    card_title(ui, "Concurrent players", "Last 24 hours, by region");
    const float cols[] = {170, -1};
    layout_row(ui, 2, cols, 2, 48);
    text(ui, "48,210", ty.big, pal.text);
    stack(ui, "+6.2%", ty.medium, pal.good, 20, "on the same hour last week", ty.small, pal.muted);
    static float eu[24], na[24], apac[24];
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 24; i++) {
            const float x = (float)i;
            eu[i] = 9000 + 7000 * std::max(0.0f, std::sin((x - 6) / 24 * 6.2832f)) + 400 * std::sin(x * 1.7f);
            na[i] =
                8000 + 8000 * std::max(0.0f, std::sin((x - 13) / 24 * 6.2832f)) + 300 * std::sin(x * 2.3f);
            apac[i] =
                5000 + 6000 * std::max(0.0f, std::sin((x + 1) / 24 * 6.2832f)) + 350 * std::sin(x * 1.3f);
        }
        init = true;
    }
    static const char* const hours[] = {"00", "", "", "03", "", "", "06", "", "", "09", "", "",
                                        "12", "", "", "15", "", "", "18", "", "", "21", "", ""};
    const ChartSeries series[] = {{"Europe", eu, 24}, {"Americas", na, 24}, {"Asia Pacific", apac, 24}};
    layout_row(ui, 1, nullptr, 0, 0);
    chart(ui, "##ccu", ChartType::Line, series, 3, hours, 180);
}

void platforms(Context& ui) {
    card_title(ui, "Platforms", "Players right now");
    static const float share[] = {19770, 11570, 8200, 5300, 3370};  // the total is the 48,210 on the left
    static const char* const names[] = {"PC", "PlayStation", "Xbox", "Switch", "Steam Deck"};
    layout_row(ui, 1, nullptr, 0, 0);
    pie_chart(ui, "##platforms", share, names, 5, 242, true);
}

void kpis(Context& ui) {
    struct Kpi {
        const char* name;
        const char* value;
        const char* delta;
        bool good;
        float spark[10];
    };
    static const Kpi k[] = {
        {"Matches per minute", "1,284", "+3.1%", true, {9, 10, 11, 10, 12, 13, 12, 14, 15, 15}},
        {"Median queue", "38 s", "+9 s", false, {30, 29, 31, 33, 32, 35, 34, 36, 37, 38}},
        {"Crash-free sessions", "99.72%", "+0.04 pts", true, {6, 7, 6, 8, 7, 8, 9, 8, 9, 9}},
        {"Revenue today", "$84,310", "+11.8%", true, {3, 4, 4, 5, 6, 6, 7, 8, 8, 9}}};
    layout_row(ui, 4, nullptr, 0, 76);
    for (const Kpi& x : k) {
        layout_begin_column(ui);
        layout_row(ui, 1, nullptr, 0, 18);
        text(ui, x.name, ty.small, pal.muted);
        const float cols[] = {-81, 80};
        layout_row(ui, 2, cols, 2, 32);
        text(ui, x.value, ty.kpi, pal.text);
        text(ui, x.delta, ty.medium, x.good ? pal.good : pal.warn);
        layout_row(ui, 1, nullptr, 0, 30);
        sparkline(ui, x.spark, 10, x.good ? Color{0, 0, 0, 0} : pal.warn);
        layout_end_column(ui);
    }
}

struct Region {
    const char* name;
    int servers, players;
    float load;
    int status;  // 0 healthy, 1 busy, 2 degraded
    float trend[8];
};
Region regions[] = {{"Frankfurt", 64, 14210, 0.72f, 0, {6, 7, 7, 8, 8, 9, 9, 8}},
                    {"Virginia", 58, 12940, 0.81f, 1, {5, 6, 7, 7, 8, 9, 9, 10}},
                    {"Oregon", 32, 6120, 0.55f, 0, {4, 4, 5, 5, 6, 5, 6, 6}},
                    {"Tokyo", 40, 7480, 0.93f, 2, {6, 7, 9, 8, 9, 10, 9, 10}},
                    {"Sydney", 18, 2310, 0.38f, 0, {3, 3, 4, 3, 4, 4, 3, 4}},
                    {"Sao Paulo", 22, 5150, 0.67f, 0, {4, 5, 5, 6, 5, 6, 6, 7}}};

void table(Context& ui) {
    card_title(ui, "Regions", "6 regions, 234 servers");
    static TableColumn cols[] = {{"Region", 170}, {"Servers", 100}, {"Players", 110},
                                 {"Load", 240},   {"Status", 120},  {"Last 8 hours", -1}};
    static TableSort sort{2, false};
    if (sort.changed || sort.column >= 0) {
        std::stable_sort(std::begin(regions), std::end(regions), [](const Region& a, const Region& b) {
            float x = 0, y = 0;
            switch (sort.column) {
                case 1:
                    x = (float)a.servers, y = (float)b.servers;
                    break;
                case 2:
                    x = (float)a.players, y = (float)b.players;
                    break;
                case 3:
                    x = a.load, y = b.load;
                    break;
                case 4:
                    x = (float)a.status, y = (float)b.status;
                    break;
                default:
                    return sort.ascending ? std::string_view(a.name) < b.name
                                          : std::string_view(b.name) < a.name;
            }
            return sort.ascending ? x < y : y < x;
        });
    }
    layout_row(ui, 1, nullptr, 0, 0);
    Theme dense = ui.theme();
    dense.spacing = 4;
    push_theme(ui, dense);
    if (!table_begin(ui, "##regions", cols, 6, 6 * 34 + 10, &sort)) {
        pop_theme(ui);
        return;
    }
    static int selected = 3;
    static const char* const status[] = {"Healthy", "Busy", "Degraded"};
    for (int i = 0; i < 6; i++) {
        const Region& r = regions[i];
        if (table_row(ui, i, selected == i).clicked) selected = i;
        label(ui, r.name);
        char n[32];
        std::snprintf(n, sizeof n, "%d", r.servers);
        text(ui, n, ty.mono, pal.text);
        std::snprintf(n, sizeof n, "%d,%03d", r.players / 1000, r.players % 1000);
        text(ui, n, ty.mono, pal.text);
        progress_bar(ui, r.load);
        badge(ui, status[r.status], r.status == 0 ? pal.good : (r.status == 1 ? pal.warn : pal.bad));
        sparkline(ui, r.trend, 8);
    }
    table_end(ui);
    pop_theme(ui);
}

void frame(Context& ui, const Input&, int w, int h) {
    sidebar(ui, h);
    push_theme(ui, canvas_theme);
    Rect r{236, 0, (float)w - 236, (float)h};
    if (begin_window(ui, "##main", r, WindowScrollable)) {
        static int range = 1;
        static char find_text[64];
        static TextBuffer find{find_text, sizeof find_text, 0};
        static const char* const ranges[] = {"1 hour", "24 hours", "7 days", "30 days"};
        const float head[] = {-(300 + 280 + 40 + 3 * 14 + 1), 300, 280, 40};
        layout_row(ui, 4, head, 4, 40);
        stack(ui, "Overview", ty.title, pal.text, 32, "All regions. Updated 12 seconds ago", ty.small,
              pal.muted);
        segmented(ui, "##range", range, ranges, 4);
        search_box(ui, "##find", find, "Search players and matches");
        icon_button(ui, "Alerts", Icon::Bell);
        const float split[] = {-421, 420};
        layout_row(ui, 2, split, 2, 0);
        layout_begin_column(ui);
        if (begin_card(ui, "##players")) {
            players(ui);
            end_card(ui);
        }
        layout_end_column(ui);
        layout_begin_column(ui);
        if (begin_card(ui, "##platforms")) {
            platforms(ui);
            end_card(ui);
        }
        layout_end_column(ui);
        layout_row(ui, 1, nullptr, 0, 0);
        if (begin_card(ui, "##kpis")) {
            kpis(ui);
            end_card(ui);
        }
        layout_row(ui, 1, nullptr, 0, 0);
        if (begin_card(ui, "##table")) {
            table(ui);
            end_card(ui);
        }
        end_window(ui);
    }
    pop_theme(ui);
}
}  // namespace

const App liveops = {setup, frame, "Live ops"};

}  // namespace showcase
