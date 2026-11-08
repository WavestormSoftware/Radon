// Copyright (c) 2026 Wavestorm Software
#pragma once
#include <radon/context.hpp>

namespace rd {

enum WindowFlags : uint32_t {
    WindowNone = 0,
    WindowMovable = 1 << 0,
    WindowResizable = 1 << 1,
    WindowClosable = 1 << 2,
    WindowScrollable = 1 << 3,
    WindowTitleBar = 1 << 4,
    WindowBorder = 1 << 5,
    WindowDefault = WindowMovable | WindowResizable | WindowTitleBar | WindowBorder,
};

/// Begins a window. Returns false if it is closed or the pool is full; call end_window only when true.
bool begin_window(Context& ctx, std::string_view title, Rect& rect, uint32_t flags = WindowDefault);
void end_window(Context& ctx);
/// Reopens a window closed with its close button.
void open_window(Context& ctx, std::string_view title);

Response button(Context& ctx, std::string_view label);
Response checkbox(Context& ctx, std::string_view label, bool& value);
Response slider(Context& ctx, std::string_view label, float& value, float min, float max,
                const char* format = "%.2f");
Response drag_float(Context& ctx, std::string_view label, float& value, float speed = 1.0f);
/// Enter reports `clicked` (submit). Escape drops focus. `hint` is placeholder text shown while empty.
Response textbox(Context& ctx, std::string_view label, TextBuffer& buffer, const char* hint = nullptr);
Response label(Context& ctx, std::string_view text);
Response label_wrapped(Context& ctx, std::string_view text);
Response progress_bar(Context& ctx, float fraction);
Response image(Context& ctx, TextureId tex, Vec2 size, Rect uv = {0, 0, 1, 1});
bool collapsing_header(Context& ctx, std::string_view label, bool& open);
bool combo(Context& ctx, std::string_view label, int& selected, const char* const* items, int count);
void tooltip(Context& ctx, std::string_view text);  ///< shown while the last widget is hovered
/// One popup can be open at a time. It anchors below the last widget.
bool popup_begin(Context& ctx, std::string_view id);
void popup_end(Context& ctx);
void open_popup(Context& ctx, std::string_view id);
void close_popup(Context& ctx);
void push_id(Context& ctx, std::string_view id);
void pop_id(Context& ctx);

// ---------------------------------------------------------------------------------------------
// More input widgets
// ---------------------------------------------------------------------------------------------

enum class Icon {
    Plus,
    Minus,
    Close,
    Check,
    ChevronDown,
    ChevronUp,
    ChevronLeft,
    ChevronRight,
    Search,
    Eye,
    EyeOff,
    Lock,
    Unlock,
    Trash,
    Folder,
    Menu,
    Dots,
    Settings,
    Star,
    Pencil,
    Undo,
    Redo
};

enum class ButtonStyle { Default, Primary, Ghost, Danger };
/// A button with a visual style. Primary uses the accent color, Ghost has no fill until hovered.
Response button(Context& ctx, std::string_view label, ButtonStyle style);
/// A radio button: selects `option` into `value`. Group radios by sharing the same `value`.
Response radio_button(Context& ctx, std::string_view label, int& value, int option);
/// A switch. Behaves like checkbox.
Response toggle(Context& ctx, std::string_view label, bool& value);
Response slider_int(Context& ctx, std::string_view label, int& value, int min, int max);
Response drag_int(Context& ctx, std::string_view label, int& value, float speed = 0.25f);
/// A number field with - and + buttons; dragging the middle also changes it.
Response stepper(Context& ctx, std::string_view label, float& value, float step, float min, float max,
                 const char* format = "%.2f");
Response stepper_int(Context& ctx, std::string_view label, int& value, int step, int min, int max);
/// Underlined tabs on one row. Returns true when the selection changed.
bool tab_bar(Context& ctx, std::string_view label, int& selected, const char* const* tabs, int count);
/// A full-width selectable row, for lists. `clicked` selects.
Response list_item(Context& ctx, std::string_view label, bool selected);
/// A list row with a leading icon.
Response list_item(Context& ctx, std::string_view label, bool selected, Icon icon);
/// A scrolling list of items in a child region of `height` pixels. Returns true on change.
bool listbox(Context& ctx, std::string_view label, int& selected, const char* const* items, int count,
             float height);
/// Edits an RGBA color with a saturation/value square, hue bar and optional alpha bar.
/// Takes a 140 px row plus a preview row. Returns true when the color changed.
bool color_picker(Context& ctx, std::string_view label, Color& color, bool alpha = true);
/// A color swatch button. `clicked` is typically used to open a popup containing a color_picker.
Response color_button(Context& ctx, std::string_view label, Color color);

// ---------------------------------------------------------------------------------------------
// Display widgets
// ---------------------------------------------------------------------------------------------

Response label_colored(Context& ctx, std::string_view text, Color color);
/// A small pill with text, for tags and counts.
void badge(Context& ctx, std::string_view text, Color background);
void bullet_text(Context& ctx, std::string_view text);
/// A muted caption followed by a horizontal rule, for grouping.
void separator_text(Context& ctx, std::string_view text);
/// An animated loading indicator.
void spinner(Context& ctx);
/// A line chart of `count` values. If min == max the range is fitted to the data.
void plot_lines(Context& ctx, const float* values, int count, float min, float max, float height = 48);
void plot_histogram(Context& ctx, const float* values, int count, float min, float max, float height = 48);

/// Draws a vector icon centered in `rect`.
void draw_icon(Context& ctx, Icon icon, const Rect& rect, Color color);
/// A square-ish button showing an icon. `label` is the ID (and tooltip-friendly name).
Response icon_button(Context& ctx, std::string_view label, Icon icon);
/// An icon button that is drawn highlighted while `selected` (toolbars).
Response icon_button(Context& ctx, std::string_view label, Icon icon, bool selected);

// ---------------------------------------------------------------------------------------------
// Containers
// ---------------------------------------------------------------------------------------------

/// A scrolling region of `height` px (negative = fill remaining) that takes a full row.
/// Call end_child only when this returned true.
bool begin_child(Context& ctx, std::string_view label, float height, bool border = true);
void end_child(Context& ctx);
/// A tree row with an arrow. When it returns true the layout is indented; call tree_pop afterwards.
bool tree_node(Context& ctx, std::string_view label, bool& open);
void tree_pop(Context& ctx);
/// Shifts the layout right by `px` (negative = theme indent) until unindent.
void indent(Context& ctx, float px = -1);
void unindent(Context& ctx, float px = -1);

/// A menu bar row at the top of a window. Call end_menu_bar only when this returned true.
bool begin_menu_bar(Context& ctx);
void end_menu_bar(Context& ctx);
/// A menu title in the bar. Call end_menu only when this returned true.
bool begin_menu(Context& ctx, std::string_view label);
void end_menu(Context& ctx);
/// An item in a menu. Returns true when chosen (the menu closes).
bool menu_item(Context& ctx, std::string_view label, const char* shortcut = nullptr, bool checked = false);

/// A centered dialog that dims and blocks everything behind it. Needs Input::screen_size.
/// Call modal_end only when modal_begin returned true.
void open_modal(Context& ctx, std::string_view id);
void close_modal(Context& ctx);
bool modal_begin(Context& ctx, std::string_view id, std::string_view title, float width);
void modal_end(Context& ctx);

// ---------------------------------------------------------------------------------------------
// Customization
// ---------------------------------------------------------------------------------------------

/// Replaces the theme until the matching pop_theme (up to 8 deep). Use it to style a section.
void push_theme(Context& ctx, const Theme& theme);
void pop_theme(Context& ctx);

}  // namespace rd
