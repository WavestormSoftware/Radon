// Copyright (c) 2026 Wavestorm Software
#include <radon/theme.hpp>

namespace rd {

Theme Theme::dark() {
    Theme t;
    t.background = {0x0a, 0x0a, 0x0a, 255};
    t.surface = {0x12, 0x12, 0x12, 255};
    t.surface_hover = {0x1e, 0x1e, 0x1e, 255};
    t.surface_active = {0x2c, 0x2c, 0x2c, 255};
    t.text = {0xee, 0xee, 0xee, 255};
    t.text_muted = {0x8a, 0x8a, 0x8a, 255};
    t.accent = {0x4d, 0xa3, 0xff, 255};  // placeholder until the brand palette is confirmed
    t.accent_text = {0x0a, 0x0a, 0x0a, 255};
    t.border = {0x33, 0x33, 0x33, 255};
    t.shadow = {0, 0, 0, 110};
    t.error = {0xff, 0x5d, 0x5d, 255};
    t.focus_ring = t.accent;
    return t;
}

Theme Theme::light() {
    Theme t;
    t.background = {0xf2, 0xf2, 0xf2, 255};
    t.surface = {0xff, 0xff, 0xff, 255};
    t.surface_hover = {0xee, 0xee, 0xee, 255};
    t.surface_active = {0xdc, 0xdc, 0xdc, 255};
    t.text = {0x11, 0x11, 0x11, 255};
    t.text_muted = {0x66, 0x66, 0x66, 255};
    t.accent = {0x4d, 0xa3, 0xff, 255};
    t.accent_text = {0x0a, 0x0a, 0x0a, 255};
    t.border = {0xcc, 0xcc, 0xcc, 255};
    t.shadow = {0, 0, 0, 60};
    t.error = {0xd9, 0x30, 0x30, 255};
    t.focus_ring = t.accent;
    return t;
}

}  // namespace rd
