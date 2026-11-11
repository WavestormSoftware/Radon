// Copyright (c) 2026 Wavestorm Software
// Halcyon: a synthesizer plugin. Knobs on an aluminium panel, LCD insets drawn with Canvas, a keyboard.
#include <cmath>

#include "showcase.hpp"

namespace showcase {
namespace {
using namespace rd;

struct Type {
    FontHandle body, medium, title, brand, small, mono;
} ty;

constexpr Color kChassis = hex(0xA9AEB3), kPanel = hex(0xD5D8DB), kPlate = hex(0xC9CDD1),
                kTrack = hex(0xB3B8BD);
constexpr Color kInk = hex(0x1B1D20), kMuted = hex(0x5D636B), kOrange = hex(0xFF5A1F), kKnob = hex(0x1E2125);
constexpr Color kLcd = hex(0x0E2422), kLcdGrid = hex(0x1C3D38), kTrace = hex(0x8AF0CF);

Theme setup(Context& ui, FontHandle fb, bool) {
    ty.body = font(ui, "BricolageGrotesque-Regular", 14, fb);
    ty.medium = font(ui, "BricolageGrotesque-Medium", 16, fb);
    ty.title = font(ui, "BricolageGrotesque-Bold", 18, fb);
    ty.brand = font(ui, "BricolageGrotesque-Bold", 30, fb);
    ty.small = font(ui, "BricolageGrotesque-Medium", 12, fb);
    ty.mono = font(ui, "IBMPlexMono-Medium", 12, fb);
    Theme t = Theme::light();
    t.background = kChassis;
    t.surface = kPanel;
    t.surface_hover = kPlate;
    t.surface_active = kTrack;
    t.border = hex(0xB0B5BA);
    t.text = kInk;
    t.text_muted = kMuted;
    t.shadow = {0, 0, 0, 70};
    t = Theme::with_accent(t, kOrange);
    t.font_body = t.font_heading = ty.body;
    t.font_small = ty.small;
    t.font_large = ty.title;
    t.font_mono = ty.mono;
    t.corner_radius = 6;
    t.window_radius = 16;
    t.padding = 14;
    t.spacing = 10;
    t.cell_height = 28;
    return t;
}

// Black knobs: a theme just for the dials (disc, indicator, caption).
void knobs_begin(Context& ui) {
    Theme t = ui.theme();
    t.surface_hover = kKnob;
    t.surface_active = hex(0xA3A9AF);
    t.border = hex(0x0B0C0E);
    t.text = hex(0xFFFFFF);
    t.text_muted = hex(0x41474F);
    t.shadow = {0, 0, 0, 110};
    push_theme(ui, t);
}

void lcd_line(Canvas& c, const Vec2* p, int n, Color col) {
    for (int i = 1; i < n; i++)
        c.line(p[i - 1], p[i], Color{col.r, col.g, col.b, 50}, 6);  // soft phosphor glow
    for (int i = 1; i < n; i++) c.line(p[i - 1], p[i], col, 2);
}

Canvas lcd_begin(Context& ui, float h) {
    layout_row(ui, 1, nullptr, 0, h);
    Canvas c = canvas_begin(ui, {0, h});
    c.fill_rect({0, 0, c.rect.w, h}, hex(0x0A0B0C), 8);
    c.fill_rect({2, 2, c.rect.w - 4, h - 4}, kLcd, 6);
    for (int i = 1; i < 8; i++)
        c.line({c.rect.w * (float)i / 8, 4}, {c.rect.w * (float)i / 8, h - 4}, kLcdGrid, 1);
    for (int i = 1; i < 4; i++) c.line({4, h * (float)i / 4}, {c.rect.w - 4, h * (float)i / 4}, kLcdGrid, 1);
    return c;
}

void module_title(Context& ui, const char* name, const char* note) {
    const float cols[] = {-121, 120};
    layout_row(ui, 2, cols, 2, 24);
    text(ui, name, ty.title, kInk);
    text(ui, note, ty.mono, kMuted);
}

void knob_row(Context& ui, const char* const* names, float* values, const char* const* formats) {
    knobs_begin(ui);
    layout_row(ui, 4, nullptr, 0, 0);
    for (int i = 0; i < 4; i++) knob(ui, names[i], values[i], 0, 1, formats[i]);
    pop_theme(ui);
}

void oscillators(Context& ui) {
    static int wave[2] = {1, 2};
    static float a[4] = {0.5f, 0.32f, 0.6f, 0.8f}, b[4] = {0.25f, 0.55f, 0.4f, 0.62f};
    static const char* const waves[] = {"Sine", "Saw", "Square", "Noise"};
    static const char* const names[] = {"Octave", "Detune", "Shape", "Level"};
    static const char* const fmt[] = {"%.2f", "%.2f", "%.2f", "%.2f"};
    module_title(ui, "Oscillators", "2 voices each");
    const float cols[] = {52, -1};
    for (int o = 0; o < 2; o++) {
        push_id(ui, o ? "b" : "a");
        layout_row(ui, 2, cols, 2, 0);
        text(ui, o ? "Osc B" : "Osc A", ty.medium, kInk);
        segmented(ui, "##wave", wave[o], waves, 4);
        knob_row(ui, names, o ? b : a, fmt);
        pop_id(ui);
    }
}

void filter(Context& ui) {
    static int type = 0;
    static float k[4] = {0.62f, 0.45f, 0.2f, 0.7f};
    static const char* const types[] = {"Low-pass", "Band", "High-pass"};
    static const char* const names[] = {"Cutoff", "Resonance", "Drive", "Envelope"};
    static const char* const fmt[] = {"%.2f", "%.2f", "%.2f", "%.2f"};
    module_title(ui, "Filter", "24 dB/oct");
    layout_row(ui, 1, nullptr, 0, 0);
    segmented(ui, "##type", type, types, 3);
    Canvas c = lcd_begin(ui, 106);
    const float W = c.rect.w, H = 106, fc = 0.15f + 0.7f * k[0], q = k[1];
    Vec2 pts[96];
    for (int i = 0; i < 96; i++) {
        const float x = (float)i / 95, d = (x - fc) / 0.06f;
        float y = x < fc ? 0.0f : -(x - fc) * 3.2f;  // flat pass band, then the slope
        y += q * 0.55f * std::exp(-d * d);           // the resonance peak at the cutoff
        if (type == 2) y = x > fc ? 0.0f : -(fc - x) * 3.2f + q * 0.55f * std::exp(-d * d);
        if (type == 1) y = -std::fabs(x - fc) * 3.2f + q * 0.55f * std::exp(-d * d);
        pts[i] = {6 + x * (W - 12), H * 0.42f - y * H * 0.45f};
        if (pts[i].y > H - 6) pts[i].y = H - 6;
    }
    lcd_line(c, pts, 96, kTrace);
    const float cx = 6 + fc * (W - 12);
    c.circle({cx, H * 0.42f - q * 0.55f * H * 0.45f}, 5, kOrange);
    canvas_end(ui);
    knob_row(ui, names, k, fmt);
}

void envelope(Context& ui) {
    static int which = 0;
    static float k[4] = {0.18f, 0.35f, 0.62f, 0.45f};
    static const char* const names[] = {"Attack", "Decay", "Sustain", "Release"};
    static const char* const fmt[] = {"%.2f", "%.2f", "%.2f", "%.2f"};
    static const char* const whichs[] = {"Amp", "Filter", "Mod"};
    module_title(ui, "Envelope", "4.2 s total");
    layout_row(ui, 1, nullptr, 0, 0);
    segmented(ui, "##which", which, whichs, 3);
    Canvas c = lcd_begin(ui, 106);
    const float W = c.rect.w - 16, H = 106, base = H - 12, top = 14;
    const float a = 8 + W * 0.25f * k[0], d = a + W * 0.25f * k[1], s = base - (base - top) * k[2];
    const float r0 = 8 + W * 0.72f, r1 = r0 + W * 0.28f * k[3];
    const Vec2 pts[] = {{8, base}, {a, top}, {d, s}, {r0, s}, {r1, base}};
    for (int i = 1; i < 5; i++) {  // a translucent fill under the curve
        c.triangle(pts[i - 1], pts[i], {pts[i].x, base}, Color{kTrace.r, kTrace.g, kTrace.b, 28});
        c.triangle(pts[i - 1], {pts[i].x, base}, {pts[i - 1].x, base},
                   Color{kTrace.r, kTrace.g, kTrace.b, 28});
    }
    lcd_line(c, pts, 5, kTrace);
    for (int i = 1; i < 4; i++) c.circle(pts[i], 5, kOrange);
    canvas_end(ui);
    knob_row(ui, names, k, fmt);
}

void scope(Context& ui) {
    module_title(ui, "Scope", "C3 E3 G3");
    Canvas c = lcd_begin(ui, 112);
    const float W = c.rect.w, H = 112;
    Vec2 pts[180];
    for (int i = 0; i < 180; i++) {
        const float x = (float)i / 179 * 6.2831853f * 3;
        const float y =
            0.5f * std::sin(x) + 0.28f * std::sin(x * 1.26f + 0.6f) + 0.18f * std::sin(x * 1.5f + 1.1f);
        pts[i] = {6 + (float)i / 179 * (W - 12), H / 2 - y * H * 0.4f};
    }
    lcd_line(c, pts, 180, kTrace);
    canvas_end(ui);
}

void effects(Context& ui) {
    static float k[4] = {0.3f, 0.45f, 0.55f, 0.78f};
    static const char* const names[] = {"Chorus", "Delay", "Reverb", "Volume"};
    static const char* const fmt[] = {"%.2f", "%.2f", "%.2f", "%.2f"};
    static bool mono = false, glide = true;
    module_title(ui, "Output", "-6.0 dB");
    layout_row(ui, 2, nullptr, 0, 0);
    toggle(ui, "Mono", mono);
    toggle(ui, "Glide", glide);
    knob_row(ui, names, k, fmt);
}

void keyboard(Context& ui) {
    const float cols[] = {34, 34, -1};
    layout_row(ui, 3, cols, 3, -1);  // the rest of the panel
    for (int w = 0; w < 2; w++) {    // pitch and mod wheels
        Canvas c = canvas_begin(ui, {34, 0});
        const float h = c.rect.h;
        c.fill_rect({0, 0, 34, h}, hex(0x0B0C0E), 8);
        c.fill_rect({7, 8, 20, h - 16}, kKnob, 6);
        const float y = w == 0 ? h / 2 : h * 0.68f;
        for (int i = -3; i <= 3; i++) c.line({9, y + (float)i * 7}, {25, y + (float)i * 7}, hex(0x3A3F46), 2);
        c.fill_rect({9, y - 3, 16, 6}, w == 0 ? hex(0xD5D8DB) : kOrange, 2);
        canvas_end(ui);
    }
    Canvas c = canvas_begin(ui, {0, 0});
    const float h = c.rect.h;
    const int whites = 36;  // five octaves and a C
    const float kw = c.rect.w / (float)whites;
    c.fill_rect({0, 0, c.rect.w, h}, hex(0x0B0C0E), 8);
    auto pressed_white = [](int i) { return i == 14 || i == 16 || i == 18; };  // the C E G on the scope
    for (int i = 0; i < whites; i++) {
        const bool on = pressed_white(i);
        const Rect k{(float)i * kw + 1, 8, kw - 2, h - 10};
        c.fill_rect(k, on ? hex(0xFFE0D1) : hex(0xF2F3F4), 4);
        c.fill_rect({k.x, k.y + k.h - 10, k.w, 10}, on ? kOrange : hex(0xD9DCDF), 4);
    }
    static const bool has_black[7] = {true, true, false, true, true, true, false};
    for (int i = 0; i < whites - 1; i++) {
        if (!has_black[i % 7]) continue;
        const Rect k{(float)(i + 1) * kw - kw * 0.3f, 8, kw * 0.6f, (h - 10) * 0.6f};
        c.fill_rect(k, kKnob, 3);
        c.fill_rect({k.x + 2, k.y + k.h - 8, k.w - 4, 5}, hex(0x34383E), 2);
    }
    c.fill_rect({0, 0, c.rect.w, 8}, hex(0x0B0C0E));
    c.line({0, 8}, {c.rect.w, 8}, kOrange, 2);  // felt strip
    canvas_end(ui);
}

void header(Context& ui) {
    static int preset = 3;
    static const char* const presets[] = {"Init",          "Tape choir",   "Night bus",
                                          "Glass harbour", "Low tide pad", "Brass at dusk"};
    const float cols[] = {40, 132, -484, 34, 230, 34, 16, 160};  // fill, then 34+230+34+16+160 and spacing
    layout_row(ui, 8, cols, 8, 40);
    Canvas logo = canvas_begin(ui, {40, 40});
    logo.circle({20, 20}, 18, kOrange);
    Vec2 wave[24];
    for (int i = 0; i < 24; i++) wave[i] = {9 + (float)i, 20 - 7 * std::sin((float)i / 23 * 6.2831853f)};
    for (int i = 1; i < 24; i++) logo.line(wave[i - 1], wave[i], hex(0xFFFFFF), 2.5f);
    canvas_end(ui);
    text(ui, "Halcyon", ty.brand, kInk);
    text(ui, "Six-voice analog-modelled synth", ty.body, kMuted);
    if (icon_button(ui, "Previous preset", Icon::ChevronLeft).clicked) preset = (preset + 5) % 6;
    combo(ui, "##preset", preset, presets, 6);
    if (icon_button(ui, "Next preset", Icon::ChevronRight).clicked) preset = (preset + 1) % 6;
    label(ui, "");
    text(ui, "CPU 3.1%   48 kHz", ty.mono, kMuted);
}

void frame(Context& ui, const Input&, int w, int h) {
    Rect r{14, 14, (float)w - 28, (float)h - 28};
    if (!begin_window(ui, "##halcyon", r, WindowBorder)) return;
    header(ui);
    spacing(ui, 2);
    const float third = (r.w - 2 * ui.theme().padding - 2 * ui.theme().spacing) / 3;
    const float mods[] = {third, third, third};
    layout_row(ui, 3, mods, 3, 0);
    void (*fns[3])(Context&) = {oscillators, filter, envelope};
    for (auto fn : fns) {
        layout_begin_column(ui);
        if (begin_card(ui, fn == oscillators ? "##osc" : (fn == filter ? "##flt" : "##env"))) {
            fn(ui);
            end_card(ui);
        }
        layout_end_column(ui);
    }
    const float bottom[] = {2 * third + ui.theme().spacing, third};
    layout_row(ui, 2, bottom, 2, 0);
    layout_begin_column(ui);
    if (begin_card(ui, "##scope")) {
        scope(ui);
        end_card(ui);
    }
    layout_end_column(ui);
    layout_begin_column(ui);
    if (begin_card(ui, "##fx")) {
        effects(ui);
        end_card(ui);
    }
    layout_end_column(ui);
    keyboard(ui);
    end_window(ui);
}
}  // namespace

const App halcyon = {setup, frame, "Halcyon"};

}  // namespace showcase
