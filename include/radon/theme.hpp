// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <radon/types.hpp>

namespace rd {

struct Theme {
    // Colors
    Color background, surface, surface_hover, surface_active, text, text_muted, accent, accent_text, border,
        shadow, error, focus_ring;
    // Metrics (logical pixels)
    float padding = 8, spacing = 6, corner_radius = 6, border_width = 1, scrollbar_width = 10,
          title_height = 30, cell_height = 28;
    // Typography. A font handle already carries its pixel size. -1 falls back to the default font.
    FontHandle font_body = -1, font_heading = -1, font_small = -1;

    static Theme dark();
    static Theme light();
};

}  // namespace rd
