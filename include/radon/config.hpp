// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <radon/theme.hpp>

namespace rd {

/// Pool sizes. Everything is allocated once in the Context constructor.
struct Config {
    int max_windows = 32;
    int max_id_stack = 32;
    int max_layout_stack = 64;
    int max_draw_commands = 4096;
    int max_vertices = 65536;
    int max_indices = 196608;
    int max_textboxes = 16;   ///< edit state, keyed by ID
    int max_fonts = 4;
    int max_glyphs = 4096;    ///< glyph cache entries
    int max_anims = 256;      ///< hover/press blend slots, keyed by ID
    int atlas_size = 1024;    ///< glyph atlas, square
    float dpi_scale = 1.0f;
    bool animations = true;   ///< cosmetic only; never changes interaction results
    Theme theme = Theme::dark();
};

}  // namespace rd
