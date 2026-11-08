# Layout

Radon uses microui-style row layout. A **row** is a list of cells with widths and a height; every widget
call takes the next cell, wrapping to a new row when the current one is full.

```cpp
const float cols[] = {90, -1};
rd::layout_row(ui, 2, cols, 2, 0);     // 2 cells: 90 px, then fill the rest
rd::label(ui, "Volume");               // cell 1
rd::slider(ui, "##v", v, 0, 1);        // cell 2
rd::label(ui, "Pan");                  // wraps: cell 1 of a new row with the same widths
rd::slider(ui, "##p", pan, -1, 1);
```

## `layout_row(ctx, columns, widths, count, height)`

| Argument | Meaning |
|---|---|
| `columns` | Cells per row (max 16). |
| `widths` / `count` | Cell `i` uses `widths[min(i, count-1)]`. `nullptr` splits the row into equal cells. |
| width > 0 | Pixels. |
| width < 0 | Fill the remaining space. **-1 fills exactly**; -2 leaves 1 px; `{-100, 100}` is "fill, then 100 px". |
| `height` | `0` = theme `cell_height`; positive = pixels; negative = fill the remaining height (-1 exactly). |

Cells are separated by `theme.spacing`; windows pad their content by `theme.padding`.

![Layout page](screenshots/gallery-layout.png)

## Columns

`layout_begin_column` / `layout_end_column` nest a vertical layout inside the next cell:

```cpp
const float halves[] = {-1, -1};
rd::layout_row(ui, 2, halves, 2, 0);
rd::layout_begin_column(ui);           // left cell becomes its own layout
  rd::layout_row(ui, 1, nullptr, 0, 0);
  rd::button(ui, "One"); rd::button(ui, "Two"); rd::button(ui, "Three");
rd::layout_end_column(ui);
rd::layout_begin_column(ui);
  rd::layout_row(ui, 1, nullptr, 0, 0);
  rd::button(ui, "Right");
rd::layout_end_column(ui);
```

The row below continues after the taller column.

## Other tools

| Call | Use |
|---|---|
| `spacing(ctx, px)` | Vertical gap; the next widget starts a fresh row. |
| `separator(ctx)` | A thin rule. `separator_text(ctx, "Group")` adds a caption. |
| `indent(ctx, px)` / `unindent(ctx, px)` | Shift the layout (default: `theme.indent`). `tree_node` does it for you. |
| `layout_next(ctx)` | The next cell as a `Rect`, for custom drawing or measuring. |
| `begin_child(ctx, id, height)` | A scrolling region that takes a full row; `-1` height fills the rest. |

## Wrapped text and images

`label_wrapped` and `image` grow their row to fit (up to 32 wrapped lines). `plot_lines` takes a height.

## Responsive layouts

Nothing is retained, so layouts are just code. The gallery switches from a sidebar to a combo box below 760
logical pixels wide, and splits pages into 1 to 3 window columns from the available width (see
`examples/common/gallery.cpp`).
