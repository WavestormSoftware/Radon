# Widget guide

Every widget takes the context first, uses the next layout cell, and returns a `rd::Response`
(`hovered`, `focused`, `pressed`, `clicked`, `changed`) or a `bool`. Labels are IDs; `"Text##id"` hides
the part after `##`. Snippets assume `using namespace rd;` and a layout row set up beforehand.

![Widgets](screenshots/gallery-widgets.png)

## Buttons

```cpp
if (button(ui, "Save").clicked) save();
button(ui, "Apply", ButtonStyle::Primary);   // accent fill
button(ui, "More", ButtonStyle::Ghost);      // no fill until hovered
button(ui, "Delete", ButtonStyle::Danger);   // error color
if (icon_button(ui, "trash", Icon::Trash).clicked) remove();
icon_button(ui, "tool", Icon::Pencil, tool == 1);       // highlighted while selected (toolbars)
```
Enter activates a focused button. `tooltip(ui, "text")` right after any widget shows a tooltip on hover.

## Toggles

```cpp
checkbox(ui, "Enabled", enabled);                 // bool&; Space toggles when focused
toggle(ui, "Dark mode", dark);                    // a switch with an animated knob
radio_button(ui, "Small", size, 0);               // int& shared by the group
radio_button(ui, "Large", size, 2);
```

## Numbers

```cpp
slider(ui, "##v", volume, 0.0f, 1.0f, "%.2f");   // printf format; Left/Right step 5% when focused
slider_int(ui, "##n", count, 0, 100);
drag_float(ui, "##d", speed, 0.05f);             // drag horizontally; 3rd arg = value per pixel
drag_int(ui, "##di", level, 0.25f);
stepper(ui, "##s", zoom, 0.25f, 0.25f, 4.0f);    // -  value  +  (drag the middle too)
stepper_int(ui, "##si", copies, 1, 1, 99);
```
All return `changed` when the value moved. Values are clamped to the range.

## Text input

```cpp
char name[64] = "";
TextBuffer buf{name, sizeof name, 0};            // non-owning; capacity includes the NUL
Response r = textbox(ui, "##name", buf, "Your name");   // optional placeholder
if (r.changed) onEdit();
if (r.clicked) onSubmit();                       // Enter
```
Click or Tab to focus. Supported: typing, Backspace/Delete, Left/Right (UTF-8 aware), Home/End, Shift to
select, Ctrl+A, Ctrl+C/X/V through the clipboard callbacks, mouse selection, caret blink, Escape to leave.
Text never overflows the buffer and never splits a UTF-8 sequence. Focusing a text box sets
`ctx.wants_text_input()`, which the SDL backend turns into the soft keyboard on mobile.

## Choosing

```cpp
combo(ui, "##c", selected, items, count);                    // true when changed; label is only the ID
tab_bar(ui, "##tabs", tab, tab_names, 3);                    // underlined tabs, one row
listbox(ui, "##l", sel, items, count, 150);                  // scrolling list in a 150 px region
list_item(ui, "Row", selected);                              // a selectable row (also with an Icon)
```

![Inputs](screenshots/gallery-inputs.png)

## Color

```cpp
Color c{236, 72, 153, 255};
color_picker(ui, "##c", c, /*alpha=*/true);   // SV square + hue bar + alpha bar + hex readout
if (color_button(ui, "swatch", c).clicked) open_popup(ui, "pick");
```
The picker edits `c` while you drag; colors you set from outside are respected. It uses a 140 px row plus a
preview row.

## Display

```cpp
label(ui, "Plain");                          label_colored(ui, "Muted", t.text_muted);
label_wrapped(ui, long_text);                bullet_text(ui, "A point");
separator_text(ui, "Section");               progress_bar(ui, 0.4f);
badge(ui, "new", t.accent);                  spinner(ui);
plot_lines(ui, samples, n, 0, 1, 80);        plot_histogram(ui, samples, n, 0, 1, 60);
image(ui, texture_id, {96, 96});             // texture ids come from the backend
draw_icon(ui, Icon::Check, rect, color);     // any rect, e.g. from layout_next
```
Icons: Plus, Minus, Close, Check, ChevronDown/Up/Left/Right, Search, Eye, EyeOff, Lock, Unlock, Trash,
Folder, Menu, Dots, Settings, Star, Pencil, Undo, Redo.

## Containers

```cpp
if (collapsing_header(ui, "Advanced", open)) { /* ... */ }      // a header bar; returns the open state

if (tree_node(ui, "Artwork", open)) {                            // arrow row; indents children
    layout_row(ui, 1, nullptr, 0, 0);
    bullet_text(ui, "City");
    tree_pop(ui);                                                // only if tree_node returned true
}

if (begin_child(ui, "region", 120)) {                            // scrolling area, takes a full row
    /* widgets */
    end_child(ui);
}
```

![Lists and menus](screenshots/gallery-lists.png)

## Windows

```cpp
static Rect win{40, 40, 360, 300};
if (begin_window(ui, "Title", win, WindowDefault | WindowScrollable | WindowClosable)) {
    /* widgets */
    end_window(ui);
}
open_window(ui, "Title");        // reopen after the close button was used
```
`rect` is read and written: dragging the title bar or the bottom-right grip updates it. Flags:
`WindowMovable`, `WindowResizable`, `WindowClosable`, `WindowScrollable`, `WindowTitleBar`,
`WindowBorder`. A window without those is a plain panel; positioning panels yourself every frame gives you
fixed docked layouts like the gallery and the dashboard.

## Menus, popups and dialogs

```cpp
if (begin_menu_bar(ui)) {                          // a bar row at the top of a window
    if (begin_menu(ui, "File")) {
        if (menu_item(ui, "Open...", "Ctrl+O")) open();
        if (menu_item(ui, "Grid", nullptr, show_grid)) show_grid = !show_grid;   // checkmark
        end_menu(ui);
    }
    end_menu_bar(ui);
}

if (button(ui, "Options").clicked) open_popup(ui, "options");      // anchored under the last widget
if (popup_begin(ui, "options")) { /* widgets */ popup_end(ui); }   // closes on outside click / Escape

if (button(ui, "Quit").clicked) open_modal(ui, "quit");
if (modal_begin(ui, "quit", "Quit?", 320)) {                       // dims and blocks the UI behind it
    if (button(ui, "OK").clicked) close_modal(ui);
    modal_end(ui);
}
```
One popup and one modal can be open at a time. `modal_begin` centers on `Input::screen_size`, so set that
field (the SDL backend runners do). `open_popup` / `open_modal` must run in the same ID scope as the matching
`_begin`.

## Keyboard and touch

- **Tab / Shift+Tab** move focus through the focused window's widgets; a focus ring appears. **Enter**
  activates buttons, tabs, list rows and menu items; **Space** toggles checkboxes, switches and radios;
  **Left/Right** adjust sliders and steppers.
- **Touch**: a finger is the pointer. There is no hover; a tap within 8 px of the press is a click; dragging
  scrolls windows and child regions (but not sliders, which capture drags).

## Custom widgets

Build on `layout_next(ui)` to get a rect, then draw. The context exposes the draw list for the
backend only, so custom drawing goes through the widgets above (`draw_icon`, `image`, `plot_*`) or a
renderer-level hook in your own backend. If you need more primitives, add them to `DrawList`
(`rect`, `shadow`, `glyph`, `image`, `line`, `triangle`, `gradient_rect`) and call them from a widget in
`src/core/widgets/`; each is a few lines.
