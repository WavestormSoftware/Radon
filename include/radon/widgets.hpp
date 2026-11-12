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
    WindowTransparent = 1 << 6,  ///< no background fill
    WindowNoPadding = 1 << 7,    ///< content starts at the window edge
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
/// Ctrl+Z / Ctrl+Y undo and redo; Ctrl+arrows and Ctrl+Backspace work by word; double-click selects a
/// word. On touch screens a long press selects a word and shows Cut / Copy / Paste.
Response textbox(Context& ctx, std::string_view label, TextBuffer& buffer, const char* hint = nullptr);
/// A text box that asks the platform for a specific soft keyboard (number pad, email, URL).
Response textbox(Context& ctx, std::string_view label, TextBuffer& buffer, const char* hint,
                 TextInputType type);
Response label(Context& ctx, std::string_view text);
Response label_wrapped(Context& ctx, std::string_view text);
Response progress_bar(Context& ctx, float fraction);  ///< fraction < 0: indeterminate (sliding bar)
Response image(Context& ctx, TextureId tex, Vec2 size, Rect uv = {0, 0, 1, 1});
/// Result of `viewport`: where the view is and the input that belongs to it.
struct ViewportResponse : Response {
    Rect rect;   ///< logical px, where the texture was drawn
    Vec2 local;  ///< pointer relative to rect's top-left, logical px (valid also while dragging outside)
    bool down = false;                ///< pointer held, and the press started inside the viewport
    bool released = false;            ///< pointer went up this frame after a press that started inside
    float wheel = 0;                  ///< scroll, only while hovered
    const KeyEvents* keys = nullptr;  ///< key events this frame, only while focused (else null)
    float pinch = 1;                  ///< two-finger zoom factor this frame, while the fingers are over it
    Vec2 pan2;                        ///< two-finger pan this frame
};
/// Shows an engine/game texture in a cell and forwards input to it. Click focuses it; a drag that
/// started inside keeps reporting `local` and `down` after the pointer leaves. `flip_y` for GL render
/// targets.
ViewportResponse viewport(Context& ctx, TextureId tex, Vec2 size, bool flip_y = false);
/// Custom window titlebar for borderless windows (PlatformConfig::custom_titlebar). Draws the bar along the
/// top of the window with an optional icon, the title and minimize / maximize / close buttons, and reports
/// the drag region and the button clicks to the backend. Widgets you add between begin and end (a menu
/// bar, tabs, a search box) land between the title and the buttons and stay clickable. Start the rest of
/// the UI at `titlebar_height(ctx)`. Call end only when begin returned true.
enum TitlebarFlags : uint32_t {
    TitlebarCenterTitle = 1 << 0,  ///< title centered in the bar; custom widgets then start after the icon
    TitlebarNoMinimize = 1 << 1,
    TitlebarNoMaximize = 1 << 2,
};
bool titlebar_begin(Context& ctx, std::string_view title, uint32_t flags = 0, TextureId icon = 0);
void titlebar_end(Context& ctx);
float titlebar_height(const Context& ctx);
/// Fills the whole window with a rounded, bordered backdrop (square corners when maximized). Use it with
/// PlatformConfig::transparent for a window with rounded corners; call it first each frame.
void window_background(Context& ctx, const Paint& fill = {});

/// Draws a modern box at `r` in one call: drop shadow, solid or gradient fill, border, per-corner radii and opacity.
///   rd::Style st; st.fill = rd::Paint::linear(a, b, 90); st.radius = 14; st.shadow = {black, 24, {0, 8}};
///   rd::box(ui, r, st);
void box(Context& ctx, const Rect& r, const Style& st);

/// Immediate-mode 2D drawing area. Coordinates are relative to the canvas top-left and everything is
/// clipped to it. Pair `canvas_begin` with `canvas_end`. Shapes are drawn in call order.
class Canvas {
public:
    Rect rect;          ///< where the canvas is, logical px
    Response response;  ///< hover/press/focus of the canvas as a whole
    Vec2 local;         ///< pointer relative to the canvas top-left
    float pinch = 1;    ///< two-finger zoom factor this frame, while the fingers are over the canvas
    Vec2 pan2;          ///< two-finger pan this frame
    /// A shadowed, bordered, gradient-filled box; see `rd::box`. Coordinates are canvas-relative.
    void box(Rect r, const Style& st);
    void fill_rect(Rect r, Color c, float radius = 0);
    void stroke_rect(Rect r, Color c, float thickness = 1, float radius = 0);
    void circle(Vec2 center, float radius, Color c);
    void line(Vec2 a, Vec2 b, Color c, float thickness = 1);
    void triangle(Vec2 a, Vec2 b, Vec2 c, Color col);
    /// A sprite: `uv` selects the part of the texture (sprite sheets), `tint` multiplies the pixels.
    void sprite(TextureId tex, Rect dst, Rect uv = {0, 0, 1, 1}, Color tint = {255, 255, 255, 255});

private:
    friend Canvas canvas_begin(Context&, Vec2, Color);
    Context* ctx_ = nullptr;
};
/// Allocates a layout cell of `size` (width capped to the cell), fills it with `background` and starts
/// drawing. A size of 0 or less takes the cell's width or height: after layout_row(ctx, 1, nullptr, 0,
/// -1) a canvas of {0, 0} fills the rest of the window.
Canvas canvas_begin(Context& ctx, Vec2 size, Color background = {0, 0, 0, 0});
void canvas_end(Context& ctx);
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
    Share,
    Filter,
    Grid,
    List,
    Image,
    Mail,
    Phone,
    Link,
    Upload,
    Send,
    Archive,
    Bookmark,
    Pin,
    Backspace,
    Code,
    Minimize,
    Maximize,
    Restore,
    Count  ///< number of icons, not an icon
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
/// A rotary control two rows tall (audio, game tools), centered in its cell, with the visible part of
/// `label` as a small caption under it ("Cutoff", or "##id" for none). Drag up/down, scroll, or use
/// the arrow keys; Home/End jump to the ends. The value shows in a tooltip while hovered.
Response knob(Context& ctx, std::string_view label, float& value, float min, float max,
              const char* format = "%.2f");
/// Numbered steps joined by lines (setup wizards, checkout). Steps before `current` show a check and can
/// be clicked to go back: returns the clicked index or -1.
int steps(Context& ctx, std::string_view label, const char* const* items, int count, int current);

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

// ---------------------------------------------------------------------------------------------
// Gestures and mobile patterns
// ---------------------------------------------------------------------------------------------

/// Call right after begin_window / begin_child (or inside a scrolling region): pulling down past the
/// top shows a spinner and, past the threshold, sets `refreshing`. Clear it when the work is done.
/// Returns true on the frame the refresh starts.
bool pull_to_refresh(Context& ctx, std::string_view id, bool& refreshing);
/// True on the frame a finger that went down near the `side` edge of the screen (Left or Right) is
/// dragged inward: open a navigation drawer.
bool edge_swipe(Context& ctx, Side side);
/// An action revealed behind a swipe_item row.
struct SwipeAction {
    const char* label;
    Icon icon;
    Color color;
};
/// A list row that slides left to reveal `actions` (delete, archive...). `triggered` receives the index
/// of the tapped action, else -1. A tap on the row reports `clicked`.
Response swipe_item(Context& ctx, std::string_view label, const SwipeAction* actions, int count,
                    int& triggered, bool selected = false);

// ---------------------------------------------------------------------------------------------
// Pickers
// ---------------------------------------------------------------------------------------------

/// An iOS-style scrolling wheel of `count` items, `visible` rows tall (odd numbers look best).
/// Flick it; it snaps to an item. Returns true on change.
bool wheel_picker(Context& ctx, std::string_view label, int& selected, const char* const* items, int count,
                  int visible = 5);
/// A field showing HH:MM that opens hour and minute wheels. `hour` is 0-23 either way; `h24` false shows
/// 12-hour with AM/PM. Returns true on change.
bool time_picker(Context& ctx, std::string_view label, int& hour, int& minute, bool h24 = true);
/// A combo with a filter box at the top of the list. Returns true on change.
bool combo_search(Context& ctx, std::string_view label, int& selected, const char* const* items, int count);
/// A combo whose list has a checkbox per item; `selected` has `count` entries. Returns true on change.
bool multi_combo(Context& ctx, std::string_view label, bool* selected, const char* const* items, int count);
/// A text box that offers matching `suggestions` (case-insensitive substring) in a popup while typing.
/// Choosing one replaces the text and reports `changed` and `clicked`.
Response autocomplete(Context& ctx, std::string_view label, TextBuffer& buffer,
                      const char* const* suggestions, int count, const char* hint = nullptr);
/// A 4x3 keypad (digits, decimal point, backspace) that edits `buffer`. Takes four rows.
Response number_pad(Context& ctx, std::string_view label, TextBuffer& buffer, bool decimal = true);

// ---------------------------------------------------------------------------------------------
// Charts
// ---------------------------------------------------------------------------------------------

enum class ChartType { Line, Area, Bar, StackedBar, Scatter };
struct ChartSeries {
    const char* name = nullptr;
    const float* values = nullptr;
    int count = 0;
    Color color{0, 0, 0, 0};  ///< alpha 0 = take the next Theme::chart color
};
/// A chart with axes, grid lines, a legend (for more than one series) and a hover/touch readout of the
/// values at the pointer. `x_labels` may be null. Returns the response of the plot area.
Response chart(Context& ctx, std::string_view label, ChartType type, const ChartSeries* series, int count,
               const char* const* x_labels, float height);
/// A pie (or donut) of `count` values with a legend. Hovering a slice shows its share.
Response pie_chart(Context& ctx, std::string_view label, const float* values, const char* const* labels,
                   int count, float height, bool donut = false);
/// A small line without axes for the inside of a table cell or a card. color alpha 0 = accent.
void sparkline(Context& ctx, const float* values, int count, Color color = {0, 0, 0, 0});

// ---------------------------------------------------------------------------------------------
// Data tables
// ---------------------------------------------------------------------------------------------

struct TableColumn {
    const char* name;
    float width;  ///< px, changed when the user drags the header edge; <= 0 fills the rest
    float min_width = 40;
};
struct TableSort {
    int column = -1;
    bool ascending = true;
    bool changed = false;  ///< set on the frame a header was clicked: re-sort your rows
};
/// A table with a fixed header, resizable columns and a scrolling body of `height` px (negative = fill).
/// With `sort`, clicking a header sorts. Emit rows with table_row / table_tree_row, one widget per
/// column. Call table_end only when this returned true.
bool table_begin(Context& ctx, std::string_view id, TableColumn* columns, int count, float height,
                 TableSort* sort = nullptr);
void table_end(Context& ctx);
/// A table_row whose first cell shows `label` indented by `depth`, with an expand arrow when `open` is
/// not null (clicking it toggles). Emit the remaining columns after it.
Response table_tree_row(Context& ctx, int index, std::string_view label, int depth, bool* open,
                        bool selected = false);

// ---------------------------------------------------------------------------------------------
// Layout containers
// ---------------------------------------------------------------------------------------------

/// A horizontally scrolling strip of `item_width` px cells, `height` px tall, that snaps to items and
/// shows page dots. Swipe or use the wheel. Each layout_next (or layout_begin_column) is one item.
/// Call carousel_end only when this returned true.
bool carousel_begin(Context& ctx, std::string_view id, float height, float item_width);
void carousel_end(Context& ctx);
/// One section of an accordion: only the section whose `index` equals `open` is expanded (clicking a
/// header opens it, clicking it again closes all). Content animates open. Call accordion_end only when
/// this returned true.
bool accordion_begin(Context& ctx, std::string_view label, int& open, int index);
void accordion_end(Context& ctx);

// ---------------------------------------------------------------------------------------------
// Docking
// ---------------------------------------------------------------------------------------------

/// Where begin_dock_panel puts a panel the first time it is seen.
enum class DockPlace { Center, Left, Right, Top, Bottom, Floating };
/// Starts a dock area covering `rect` (usually the screen below a menu bar). Call it outside any window,
/// before the panels. Panels docked in it show as tabs; drag a tab out to float it, drag a floating
/// panel's title onto a drop zone to dock it again, drag the gaps to resize.
void dock_space(Context& ctx, std::string_view id, Rect rect);
/// A dockable panel. Returns true when it is visible (its tab is selected, or it floats); call
/// end_dock_panel only then. With `open`, the panel gets a close button that sets it false; skip the
/// call while it is false. `place` and `ratio` (share of the area) apply on first appearance.
bool begin_dock_panel(Context& ctx, std::string_view title, bool* open = nullptr,
                      DockPlace place = DockPlace::Center, float ratio = 0.25f);
void end_dock_panel(Context& ctx);

// ---------------------------------------------------------------------------------------------
// Rich text
// ---------------------------------------------------------------------------------------------

struct RichTextResponse : Response {
    std::string_view link;  ///< URL of the link clicked this frame (a view into the markup), else empty
};
/// Wrapped text with inline markup: **bold**, *italic*, `code`, [text](url), {#rrggbb}color{/},
/// :icon: (home, star, heart, check, info, warning, user, bell, ...). Backslash escapes a character.
RichTextResponse rich_text(Context& ctx, std::string_view markup);
/// Markdown blocks: # headings, - and 1. lists, > quotes, ``` code blocks, --- rules and blank-line
/// paragraphs, each paragraph with rich_text markup.
RichTextResponse markdown(Context& ctx, std::string_view text);
/// Read-only text that can be selected and copied. Lines break at '\n'.
Response selectable_text(Context& ctx, std::string_view label, std::string_view text);

// ---------------------------------------------------------------------------------------------
// Drag and drop
// ---------------------------------------------------------------------------------------------

/// Call after a widget to make it draggable (mouse: press and move; touch: long press, then move).
/// `data` (up to 256 bytes) is copied when the drag starts; `preview` is drawn at the pointer.
/// Returns true while this source is being dragged.
bool drag_source(Context& ctx, const char* type, const void* data, int size, std::string_view preview);
/// Call after a widget: it highlights while a payload of `type` hovers it and returns the payload on
/// the frame it is dropped (else null). `size` receives the byte count.
const void* drop_target(Context& ctx, const char* type, int* size = nullptr);
/// True while any drag-and-drop is in progress.
bool dragging(Context& ctx);
/// A list whose rows can be dragged into a new order. `order` holds `count` indices into `labels`
/// and is permuted in place. Returns true on change.
bool reorder_list(Context& ctx, std::string_view id, int* order, const char* const* labels, int count);

}  // namespace rd
