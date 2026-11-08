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

Theme Theme::dark() {
    Theme t;
    t.background = {0x0b, 0x0e, 0x12, 255};
    t.surface = {0x0f, 0x14, 0x18, 255};
    t.surface_hover = {0x15, 0x1a, 0x20, 255};
    t.surface_active = {0x20, 0x27, 0x30, 255};
    t.text = {0xdf, 0xe4, 0xea, 255};
    t.text_muted = {0x8b, 0x95, 0xa1, 255};
    t.accent_text = {0xff, 0xff, 0xff, 255};
    t.border = {0x1d, 0x24, 0x2c, 255};
    t.shadow = {0, 0, 0, 130};
    t.error = {0xff, 0x5d, 0x6c, 255};
    t.success = {0x3d, 0xd6, 0x8c, 255};
    t.warning = {0xf5, 0xb5, 0x4a, 255};
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
    return with_accent(t, {0xd9, 0x2f, 0x84, 255});
}

}  // namespace rd
