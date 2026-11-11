# Changelog

Radon follows [semantic versioning](https://semver.org); before 1.0, minor versions may break the API.

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
