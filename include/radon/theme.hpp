// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <radon/types.hpp>

namespace rd {

struct Theme {
    // Colors
    Color background, surface, surface_hover, surface_active, text, text_muted, accent, accent_text, border,
        shadow, error, focus_ring;
    Color selection;  ///< tint for selected rows and text selection
    Color success, warning;
    // Metrics (logical pixels)
    float padding = 8, spacing = 6, corner_radius = 6, border_width = 1, scrollbar_width = 10,
          title_height = 30, cell_height = 28;
    float window_radius = 8;     ///< corner radius of windows and popups
    float shadow_blur = 18;      ///< window shadow softness
    float focus_ring_width = 2;  ///< keyboard focus ring
    float indent = 16;           ///< tree node / indent() step
    bool shadows = true;         ///< draw window and popup shadows
    // Typography. A font handle already carries its pixel size. -1 falls back to the default font.
    FontHandle font_body = -1, font_heading = -1, font_small = -1;

    /// Wavestorm dark: near-black navy surfaces with a hot pink accent.
    static Theme dark();
    static Theme light();
    /// The previous blue-accent dark theme.
    static Theme midnight();
    /// Returns `base` recolored with `accent` (accent, focus ring and selection tint).
    static Theme with_accent(Theme base, Color accent);
};

}  // namespace rd
