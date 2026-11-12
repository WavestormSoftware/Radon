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
    int max_textboxes = 16;  ///< edit state, keyed by ID
    int max_children = 32;   ///< scrolling child regions, keyed by ID
    int max_fonts = 4;
    int max_glyphs = 4096;  ///< glyph cache entries
    int max_anims = 256;    ///< hover/press blend slots, keyed by ID
    int atlas_size = 2048;  ///< glyph atlas, square
    int undo_bytes = 8192;  ///< undo + redo history of the focused text box
    int max_dock_nodes = 32;
    int max_dock_panels = 32;
    int max_a11y_nodes = 1024;
    float dpi_scale = 1.0f;
    bool animations = true;  ///< transitions; widget results never depend on them
    bool a11y = false;       ///< record the accessibility tree each frame (Context::a11y_nodes)
    Theme theme = Theme::dark();
};

}  // namespace rd
