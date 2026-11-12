// Copyright (c) 2026 Wavestorm Software
#include <radon/theme.hpp>

namespace rd {

static Color mix(Color a, Color b, float f) {
    auto m = [f](uint8_t x, uint8_t y) { return (uint8_t)(x + (y - x) * f + 0.5f); };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), 255};
}

Theme Theme::with_accent(Theme t, Color accent) {
    t.accent = accent;
    t.focus_ring = accent;
    t.selection = mix(t.surface, accent, 0.28f);
    return t;
}

Theme Theme::scaled(Theme t, float k) {
    for (float* m : {&t.padding, &t.spacing, &t.corner_radius, &t.scrollbar_width, &t.title_height,
                     &t.cell_height, &t.window_radius, &t.shadow_blur, &t.indent})
        *m *= k;
    return t;
}

Theme Theme::touch(Theme t) {
    t.cell_height = 44;  // the common minimum touch target (Apple HIG 44 pt, Material 48 dp)
    t.title_height = 48;
    t.padding = 12;
    t.spacing = 8;
    t.indent = 20;
    t.scrollbar_width = 6;
    t.corner_radius += 2;
    return t;
}

Theme Theme::glass(Theme t) {
    const bool darkish = (int)t.background.r + t.background.g + t.background.b < 384;
    auto a = [](Color c, int alpha) {
        c.a = (uint8_t)alpha;
        return c;
    };
    t.surface = a(t.surface, 150);
    t.surface_hover = darkish ? Color{255, 255, 255, 26} : Color{255, 255, 255, 150};
    t.surface_active = darkish ? Color{255, 255, 255, 46} : Color{255, 255, 255, 210};
    t.border = darkish ? Color{255, 255, 255, 34} : Color{255, 255, 255, 170};
    t.shadow = {0, 0, 0, darkish ? (uint8_t)150 : (uint8_t)60};
    t.corner_radius = 10;
    t.window_radius = 16;
    t.shadow_blur = 36;
    Color hi = mix(t.accent, {255, 255, 255, 255}, 0.22f), lo = mix(t.accent, {90, 60, 255, 255}, 0.35f);
    t.accent_paint = Paint::linear(hi, lo, 90);
    t.window_paint = Paint::linear(a(t.surface, 170), a(t.surface, 120), 90);
    t.card_paint = t.surface_hover;
    return t;
}

Theme Theme::dark() {
    Theme t;
    t.background = {0x0b, 0x0e, 0x12, 255};
    t.surface = {0x0f, 0x14, 0x18, 255};
    t.surface_hover = {0x15, 0x1a, 0x20, 255};
    t.surface_active = {0x20, 0x27, 0x30, 255};
    t.text = {0xdf, 0xe4, 0xea, 255};
    t.text_muted = {0x8b, 0x95, 0xa1, 255};
    t.accent_text = {0xff, 0xff, 0xff, 255};
    t.border = {0x25, 0x2d, 0x36, 255};
    t.shadow = {0, 0, 0, 130};
    t.error = {0xff, 0x5d, 0x6c, 255};
    t.success = {0x3d, 0xd6, 0x8c, 255};
    t.warning = {0xf5, 0xb5, 0x4a, 255};
    const Color chart[8] = {{0xec, 0x48, 0x99, 255}, {0x4d, 0xa3, 0xff, 255}, {0x3d, 0xd6, 0x8c, 255},
                            {0xf5, 0xb5, 0x4a, 255}, {0xa7, 0x8b, 0xfa, 255}, {0x2d, 0xd4, 0xd4, 255},
                            {0xff, 0x7a, 0x45, 255}, {0x94, 0xa3, 0xb8, 255}};
    for (int i = 0; i < 8; i++) t.chart[i] = chart[i];
    return with_accent(t, {0xec, 0x48, 0x99, 255});
}

Theme Theme::midnight() {
    Theme t = dark();
    t.background = {0x0a, 0x0a, 0x0a, 255};
    t.surface = {0x12, 0x12, 0x12, 255};
    t.surface_hover = {0x1e, 0x1e, 0x1e, 255};
    t.surface_active = {0x2c, 0x2c, 0x2c, 255};
    t.border = {0x33, 0x33, 0x33, 255};
    t.accent_text = {0x0a, 0x0a, 0x0a, 255};
    return with_accent(t, {0x4d, 0xa3, 0xff, 255});
}

Theme Theme::light() {
    Theme t;
    t.background = {0xf2, 0xf4, 0xf7, 255};
    t.surface = {0xff, 0xff, 0xff, 255};
    t.surface_hover = {0xee, 0xf0, 0xf4, 255};
    t.surface_active = {0xdd, 0xe1, 0xe8, 255};
    t.text = {0x14, 0x18, 0x1e, 255};
    t.text_muted = {0x66, 0x70, 0x7c, 255};
    t.accent_text = {0xff, 0xff, 0xff, 255};
    t.border = {0xd3, 0xd8, 0xe0, 255};
    t.shadow = {0, 0, 0, 55};
    t.error = {0xd9, 0x30, 0x40, 255};
    t.success = {0x1f, 0xa3, 0x68, 255};
    t.warning = {0xc9, 0x80, 0x10, 255};
    const Color chart[8] = {{0xd9, 0x2f, 0x84, 255}, {0x25, 0x63, 0xeb, 255}, {0x1f, 0xa3, 0x68, 255},
                            {0xc9, 0x80, 0x10, 255}, {0x7c, 0x3a, 0xed, 255}, {0x0e, 0x94, 0x9a, 255},
                            {0xe0, 0x55, 0x1b, 255}, {0x64, 0x74, 0x8b, 255}};
    for (int i = 0; i < 8; i++) t.chart[i] = chart[i];
    return with_accent(t, {0xd9, 0x2f, 0x84, 255});
}

}  // namespace rd
