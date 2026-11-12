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
    float backdrop_blur = 0;     ///< > 0: windows and popups frost what is behind them (blur radius, px)
    float focus_ring_width = 2;  ///< keyboard focus ring
    float indent = 16;           ///< tree node / indent() step
    bool shadows = true;         ///< draw window and popup shadows
    // Typography. A font handle already carries its pixel size. -1 falls back to the default font.
    FontHandle font_body = -1, font_heading = -1, font_small = -1;
    FontHandle font_large = -1;   ///< heading() text; falls back to font_heading
    FontHandle font_bold = -1;    ///< rich text **bold**; without it bold is drawn twice, offset
    FontHandle font_italic = -1;  ///< rich text *italic*; falls back to the body font
    FontHandle font_mono = -1;    ///< rich text `code` and code blocks; falls back to the body font
    Color chart[8];               ///< series colors for charts, in order
    // Optional fills. The default (empty) Paint falls back to the plain color: accent, surface or surface_hover.
    Paint accent_paint;  ///< primary buttons and progress bars: gradients welcome
    Paint window_paint;  ///< windows, popups and dialogs
    Paint card_paint;    ///< cards

    /// Wavestorm dark: near-black navy surfaces with a hot pink accent.
    static Theme dark();
    static Theme light();
    /// The previous blue-accent dark theme.
    static Theme midnight();
    /// Returns `base` recolored with `accent` (accent, focus ring and selection tint).
    static Theme with_accent(Theme base, Color accent);
    /// Returns `base` with every metric multiplied by `k`.
    static Theme scaled(Theme base, float k);
    /// Returns `base` with finger-sized metrics: 44 px rows, wider padding, slim scrollbars.
    /// Use it on phones and tablets (or whenever Input::touch is set).
    static Theme touch(Theme base);
    /// Returns `base` restyled as frosted glass: translucent surfaces, hairline light borders, large radii,
    /// soft deep shadows and a gradient accent. Draw a colorful background (a gradient `box` over the whole
    /// window) first, or the translucency has nothing to show.
    static Theme glass(Theme base);
};

}  // namespace rd
