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
    Redo,
    ArrowLeft,
    ArrowRight,
    ArrowUp,
    ArrowDown,
    Home,
    User,
    Bell,
    Heart,
    Info,
    Warning,
    Calendar,
    Clock,
    Sun,
    Moon,
    Play,
    Pause,
    Refresh,
    Copy,
    Download,
    Share
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

// ---------------------------------------------------------------------------------------------
// Text input variants
// ---------------------------------------------------------------------------------------------

/// A text box that shows stars instead of the text. Copy and cut are disabled.
Response password_box(Context& ctx, std::string_view label, TextBuffer& buffer, const char* hint = nullptr);
/// A text box with a search icon and a clear button. `changed` also reports the clear.
Response search_box(Context& ctx, std::string_view label, TextBuffer& buffer, const char* hint = nullptr);
/// A multi-line editor of `height` px. Enter inserts a line break; lines break only at '\n'.
Response text_area(Context& ctx, std::string_view label, TextBuffer& buffer, float height,
                   const char* hint = nullptr);

// ---------------------------------------------------------------------------------------------
// More controls
// ---------------------------------------------------------------------------------------------

/// A slider with two thumbs selecting [low, high]. Arrow keys move `high`, Shift+arrows move `low`.
Response range_slider(Context& ctx, std::string_view label, float& low, float& high, float min, float max);
/// Equal-width options in one rounded track with a sliding highlight. Returns true on change.
bool segmented(Context& ctx, std::string_view label, int& selected, const char* const* items, int count);
/// A pill sized to its text, drawn filled while `selected` (filters, tags). `clicked` toggles.
Response chip(Context& ctx, std::string_view label, bool selected);
/// A row of `max` stars. Clicking star n sets `value` to n; clicking the current value clears it.
Response rating(Context& ctx, std::string_view label, int& value, int max = 5);
/// Accent-colored text that underlines on hover. Open the URL yourself when `clicked`.
Response link(Context& ctx, std::string_view text);
/// A thin drag handle that takes one cell: dragging adds the horizontal movement to `size`
/// (clamped to [min, max]). Use `size` as the width of the cell before it.
Response splitter(Context& ctx, std::string_view label, float& size, float min, float max);
/// A month grid. The arrows change `month`/`year`; clicking a day sets `day`. Returns true on change.
bool calendar(Context& ctx, std::string_view label, int& year, int& month, int& day);
/// A field showing YYYY-MM-DD that opens a calendar popup. Returns true on change.
bool date_picker(Context& ctx, std::string_view label, int& year, int& month, int& day);
/// Previous / page numbers / next in one cell. `page` is zero-based. Returns true on change.
bool pagination(Context& ctx, std::string_view label, int& page, int page_count);
/// A bottom navigation bar (phones): icons over small labels in one cell; give the row ~56 px.
bool nav_bar(Context& ctx, std::string_view label, int& selected, const Icon* icons,
             const char* const* labels, int count);
/// A clickable path ("Home > Docs > File"). Returns the clicked index or -1.
int breadcrumbs(Context& ctx, std::string_view label, const char* const* items, int count);

// ---------------------------------------------------------------------------------------------
// More display widgets
// ---------------------------------------------------------------------------------------------

/// Large text using Theme::font_large (falls back to font_heading).
void heading(Context& ctx, std::string_view text);
/// A colored circle with up to two initials taken from `name`.
void avatar(Context& ctx, std::string_view name, Color color);
/// A tinted message box with an icon; the text wraps and the row grows to fit.
void alert(Context& ctx, Tone tone, std::string_view text);
/// A circular progress indicator for a fraction in [0, 1].
void progress_ring(Context& ctx, float fraction);
/// A shimmering placeholder bar for content that is still loading. `width` in (0, 1] of the cell.
void skeleton(Context& ctx, float width = 1.0f);
/// Shows a short message at the bottom of the screen for `seconds`, with a fade in and out.
/// Call it once when something happens (not every frame). Up to 4 are stacked.
void toast(Context& ctx, std::string_view text, Tone tone = Tone::Info, float seconds = 3.0f);
/// Keeps `bottom_px` free under the toasts, e.g. the height of a bottom navigation bar.
void set_toast_offset(Context& ctx, float bottom_px);

// ---------------------------------------------------------------------------------------------
// Tables and long lists
// ---------------------------------------------------------------------------------------------

/// A header row; also sets the layout to these columns so the following cells line up. `widths` is as
/// in layout_row. With `sort_column`/`ascending`, clicking a column sorts by it (returns true).
bool table_header(Context& ctx, std::string_view label, const char* const* columns, int count,
                  const float* widths, int* sort_column = nullptr, bool* ascending = nullptr);
/// Starts body row `index`: zebra stripe, hover and selection highlight across the full width.
/// Follow it with one widget per column. `clicked` selects.
/// The row takes the click, so put buttons in rows only when you ignore the row's Response.
Response table_row(Context& ctx, int index, bool selected = false);

/// For lists with thousands of uniform rows inside a scrolling window or child: computes the visible
/// range [first, last) and reserves the space of the hidden rows. Emit rows first..last-1 with
/// layout_row(ctx, n, widths, n, row_height), then call list_clipper_end. `row_height` 0 = theme.
void list_clipper_begin(Context& ctx, int count, float row_height, int& first, int& last);
void list_clipper_end(Context& ctx);

// ---------------------------------------------------------------------------------------------
// More containers
// ---------------------------------------------------------------------------------------------

/// A bordered, padded group that grows to fit its content (one frame behind when the content
/// changes). Call end_card only when this returned true.
bool begin_card(Context& ctx, std::string_view label);
void end_card(Context& ctx);

enum class Side { Left, Right, Bottom };
/// A panel that slides in from a screen edge over a dimmed background (navigation drawer, bottom
/// sheet). Open it with open_modal(id), close with close_modal, Escape, the Android back button or
/// a tap outside. `size` is its width (Left/Right) or height (Bottom); safe-area insets are added.
/// Call drawer_end only when drawer_begin returned true.
bool drawer_begin(Context& ctx, std::string_view id, Side side, float size);
void drawer_end(Context& ctx);

/// True if the last widget was right-clicked (mouse) or long-pressed (touch).
bool secondary_clicked(Context& ctx);
/// Opens popup `id` at the pointer when the last widget is right-clicked or long-pressed, and begins
/// it. Fill it with menu_item calls and call popup_end when this returned true.
bool context_menu_begin(Context& ctx, std::string_view id);

// ---------------------------------------------------------------------------------------------
// State and transitions
// ---------------------------------------------------------------------------------------------

/// Widgets until end_disabled are drawn faded and ignore all input. Nests up to 8 deep.
void begin_disabled(Context& ctx, bool disabled = true);
void end_disabled(Context& ctx);

/// Eases a value stored under `id` toward `target` and returns it; call every frame. The first
/// frame (and any frame after a pause) starts at `from`. Use it to animate your own drawing.
float animate(Context& ctx, std::string_view id, float target, float duration = 0.15f, float from = 0.0f);
/// Everything until transition_end fades and slides in whenever `key` changes (page switches, tabs).
void transition_begin(Context& ctx, std::string_view id, int key);
void transition_end(Context& ctx);

}  // namespace rd
