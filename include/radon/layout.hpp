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
/// Starts a wrapping row: widgets that know their size (buttons, chips, badges, links, icon buttons,
/// labels) are placed side by side and wrap to a new line when the row is full. Other widgets get
/// `item_width` px (0 = 4 cell heights). `height` 0 = theme cell height. layout_row also ends it.
void flow_begin(Context& ctx, float item_width = 0, float height = 0);
void flow_end(Context& ctx);
/// A responsive grid: as many equal columns of at least `min_cell_width` px as fit (up to 16).
/// Returns the column count. `height` as in layout_row.
int layout_grid(Context& ctx, float min_cell_width, float height = 0);
void separator(Context& ctx);

}  // namespace rd
