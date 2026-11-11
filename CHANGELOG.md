# Changelog

Radon follows [semantic versioning](https://semver.org); before 1.0, minor versions may break the API.

## 0.3.0 (unreleased)

### Added

- **Mobile**: multi-touch (`Input::touches`) and `Context::gesture()` (pinch, rotate, two-finger pan, swipe,
  velocity); `pull_to_refresh`, `edge_swipe`, `swipe_item`; drawers and bottom sheets follow the finger and
  close on a swipe, bottom sheets have a grab handle; `Input::keyboard_inset` scrolls the focused field
  above the soft keyboard; `TextInputType` and a `textbox` overload pick the soft keyboard; haptic feedback
  (`Context::haptic()`, played by the SDL backend on Android and iOS); `ViewportResponse` and `Canvas`
  report `pinch` and `pan2`.
- **Text editing**: undo/redo (Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z), word-wise movement and deletion, double-click
  word and triple-click line selection, PageUp/PageDown, and on touch a long-press word selection with a
  Cut / Copy / Paste / Select all bar and selection handles. `selectable_text`. Keys Z, Y, PageUp, PageDown.
- **Pickers**: `wheel_picker`, `time_picker`, `combo_search`, `multi_combo`, `autocomplete`, `number_pad`.
- **Charts and data**: `chart` (line, area, bar, stacked bar, scatter) with axes, legend and a value
  readout; `pie_chart` (pie and donut); `sparkline`; `table_begin` / `table_end` with a fixed header,
  resizable and sortable columns, and `table_tree_row`. `Theme::chart` palette.
- **Layout**: `flow_begin` / `flow_end` (wrapping rows of sized widgets), `layout_grid`, `carousel_begin`,
  `accordion_begin`.
- **Docking**: `dock_space`, `begin_dock_panel` / `end_dock_panel` with splits, tabs, floating panels, drop
  zones and close buttons.
- **Rich text**: `rich_text` (bold, italic, code, links, colors, inline icons) and `markdown`;
  `Theme::font_bold`, `font_italic`, `font_mono`.
- **Drag and drop**: `drag_source`, `drop_target`, `dragging`, `reorder_list`.
- **Accessibility**: `Config::a11y` records an accessibility tree (`Context::a11y_nodes`), and
  `Input::a11y_action` lets a screen reader activate, focus and adjust widgets. See `docs/accessibility.md`.
- **Fonts**: `Context::set_font_fallback` chains fonts for missing glyphs (emoji, CJK, symbols).
- 14 icons (Filter, Grid, List, Image, Mail, Phone, Link, Upload, Send, Archive, Bookmark, Pin, Backspace,
  Code) and `Icon::Count`.
- C API v2: touches and keyboard inset in `rd_input`, the new keys, `rd_text_input_type`, `rd_haptic`,
  `rd_pinch`, `rd_swipe`.
- Gallery pages "Charts", "Pickers", "Rich text", "Gestures" and "Docking"; the phone layout opens its menu
  with an edge swipe and refreshes with a pull. `11_mobile_app` uses the new mobile widgets.

### Changed (breaking)

- `Input` has new fields (`touches`, `touch_count`, `keyboard_inset`, `a11y_action`, `a11y_target`) before
  `screen_size`: code that brace-initializes `Input` by position must name the fields instead.
- `rd_input` (C API) grew; zero-initialize it. `RD_C_API_VERSION` is 2.
- Buttons, labels, badges, chips, links and icon buttons place themselves through the new sized-cell path
  (same result outside flow layouts).
- Bottom sheet content starts 12 px lower, under the new grab handle.
- On iOS the SDL backend compiles an Objective-C file (haptics) and links UIKit.

### Fixed

- Several tall cells in one row (two wheel pickers, images or wrapped labels side by side) grew the row by
  the sum of their heights instead of the tallest.
- A tooltip or toast given a default-constructed (null) `std::string_view` passed a null pointer to
  `memcpy` (undefined behaviour; UBSan found the same pattern in new code).

## 0.2.0 (unreleased)

### Added

- **Widgets**: `password_box`, `search_box`, `text_area` (multi-line), `range_slider`, `segmented`, `chip`,
  `rating`, `link`, `splitter`, `calendar`, `date_picker`, `pagination`, `nav_bar`, `breadcrumbs`, `heading`,
  `avatar`, `alert`, `progress_ring`, `skeleton`, `toast`, `table_header` / `table_row`,
  `list_clipper_begin` / `list_clipper_end`, `begin_card` / `end_card`, `drawer_begin` / `drawer_end`
  (side drawers and bottom sheets), `context_menu_begin`, `secondary_clicked`. 20 new icons.
- **Transitions**: popups, dialogs, drawers, toasts and tooltips fade or slide in; checkbox, radio, switch,
  tab underline and segmented highlight animate. `transition_begin` / `transition_end` for page changes,
  `animate()` for your own values, `Context::set_animations` to turn motion off at runtime.
- **Touch and mobile**: `Theme::touch()` and `Theme::scaled()`, flick scrolling with momentum, long-press
  as a secondary click, `Input::safe_rect()`, finger-sized window resize grip, popups that stay on screen.
- **Native integration** (SDL3 backend): OS display scale for sizing, system light/dark theme, mouse
  cursors, right-click, IME / soft keyboard placement, file dialogs, `open_url`, Android back button,
  pausing in the background, sleeping while idle (`PlatformConfig::wait_events`), mouse capture.
- `begin_disabled` / `end_disabled`, `DrawList::set_alpha`, `Context::cursor()`, `animating()`,
  `can_dismiss()`, `text_input_rect()`, `Theme::font_large`.
- macOS preset and CI job; `cmake --install` and `find_package(Radon)` for the core.
- Gallery pages "Data" and "Motion and native", a phone layout with bottom navigation, and the
  `11_mobile_app` example. Demos fall back to a system font and follow the system theme.
- CONTRIBUTING, CODE_OF_CONDUCT, SECURITY, issue and pull request templates.

### Changed

- `RD_BACKEND_SDL3_GL` now defaults to ON only for a top-level build. Adding Radon with `add_subdirectory`
  or FetchContent builds the dependency-free core alone; set the option to ON to get the backend (and SDL3).
- CI fails, instead of skipping, when no test font is found (`RD_REQUIRE_TEST_FONT=1`) and runs with
  read-only repository permissions.
- `rd::sdl3::dpi_scale()` now returns the OS display scale instead of the pixel density. On Android and
  on scaled Windows/Linux desktops the UI was drawn at 1:1 device pixels before and came out tiny;
  pointer positions and the safe area are converted to logical pixels to match.
- `separator_text` always takes a full-width row.
- Tests and demos are no longer built when Radon is added with `add_subdirectory`.
- Popups may be up to 340 px tall (was 260).

### Fixed

- `pagination`: the page buttons for pages 1001 and 1002 shared an ID with Prev and Next, so they shared
  focus and hover (Enter on one also fired the other). Prev and Next now use IDs no page can reach.
- Tooltips, toasts and pasted text cut at a byte limit could end in half a UTF-8 code point. Long IME
  commits in the SDL3 backend are now pushed in whole code points.
- The SDL3 renderer no longer leaks shader and program objects when startup fails.
- The backend tested `defined(TARGET_OS_IPHONE)`, which is also true on macOS, so a Mac build would
  have asked for a fullscreen GLES window.
- A child region and its window both scrolled when dragged with a finger.

## 0.1.0

First release: immediate-mode core (windows, layout, widgets, text input, draw list), SDL3 + OpenGL ES 3
backend, desktop/web/mobile demo projects. See `docs/STATUS.md` for what was verified on each target.
