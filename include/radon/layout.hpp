// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <radon/context.hpp>

namespace rd {

/// Starts a row of `columns` cells with height `height` (0 = theme cell height, negative = remaining
/// height plus that offset). `widths` has `count` entries (cell i uses widths[min(i, count-1)]);
/// positive = pixels, negative = fill remaining space (-1 fills exactly). A null `widths` splits the
/// row into equal cells.
void layout_row(Context& ctx, int columns, const float* widths, int count, float height);
/// Next cell using the current row. Widgets call this; hosts may too for custom drawing.
Rect layout_next(Context& ctx);
void layout_begin_column(Context& ctx);  ///< nested vertical layout in the next cell
void layout_end_column(Context& ctx);
void spacing(Context& ctx, float px);
void separator(Context& ctx);

}  // namespace rd
