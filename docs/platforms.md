# Platforms: sizing, touch and native features

Radon's core knows nothing about the platform. Everything here is done by the SDL3 backend
(`backends/sdl3_gl`) through a few small hooks on `rd::Context`, so your own backend can do the same.

## Sizing: logical pixels

All sizes in Radon are **logical pixels**. The backend reports the OS display scale as
`rd::sdl3::dpi_scale()` (2 on a Retina Mac, 1.5 on a Windows or Linux desktop set to 150%, about 2.6 on a
typical Android phone) and converts pointer positions, the safe area and the window size to logical
pixels. A 44 px row is therefore about the same physical size everywhere, and text is rasterized at the
real pixel size so it stays sharp.

```cpp
int fw, fh, w, h;
rd::sdl3::framebuffer_size(plat, fw, fh);     // physical pixels, for the renderer
rd::sdl3::logical_size(plat, w, h);           // logical pixels, for your layout
ui.set_dpi_scale(rd::sdl3::dpi_scale(plat));  // every frame: the window can move between monitors
input.screen_size = {(float)w, (float)h};
```

## Touch

```cpp
ui.set_theme(rd::Theme::touch(rd::Theme::dark()));   // 44 px rows, wider padding, slim scrollbars
```

Use `Theme::touch` on phones and tablets (the demo runner does on Android and iOS, and the gallery does
whenever the window is narrower than 760 px). With `Input::touch` set by the backend:

| Behaviour | |
|---|---|
| Tap | A click, with 8 px of slop. No hover state. |
| Drag | Scrolls the window or child under the finger, with momentum after a flick. |
| Long-press (0.5 s) | A secondary click: opens `context_menu_begin` menus, like a right-click. |
| Sliders, text selection | Capture the drag, so the page does not scroll under them. |

`Input::safe_area` holds the notch and system-bar insets; `input.safe_rect()` is the usable rectangle.
Drawers add the insets themselves. `set_toast_offset(ui, h)` keeps toasts above a bottom bar.

Phone-style building blocks: `nav_bar` (bottom navigation), `drawer_begin` (navigation drawer and bottom
sheet), `toast`, `segmented`, `chip`, `list_clipper_begin`. `examples/snippets/11_mobile_app.cpp` puts
them together.

## Native features

| Feature | How | Windows | Linux | macOS | Web | Android | iOS |
|---|---|---|---|---|---|---|---|
| Display scale (HiDPI) | `dpi_scale`, `logical_size` | yes | yes | yes | yes | yes | yes |
| System light/dark theme | `system_dark`, `system_theme_changed` | yes | yes (portal) | yes | yes | yes | yes |
| Mouse cursors (text beam, resize, hand) | `Context::cursor()` | yes | yes | yes | yes | - | - |
| Right-click context menus | `Input::secondary_pressed` | yes | yes | yes | yes | long-press | long-press |
| Clipboard | `set_clipboard_callbacks` | yes | yes | yes | yes | yes | yes |
| Cmd instead of Ctrl for shortcuts | backend key mapping | - | - | yes | - | - | yes |
| IME candidate window / soft keyboard | `wants_text_input`, `text_input_rect` | yes | yes | yes | browser | yes | yes |
| File dialogs | `show_file_dialog`, `poll_file_dialog` | yes | yes (portal or zenity) | yes | no | partly | no |
| Open a URL in the browser | `open_url` | yes | yes | yes | yes | yes | yes |
| Back button closes popups, then leaves | `Context::can_dismiss()` | - | - | - | - | yes | - |
| Safe area (notch, home indicator) | `Input::safe_area` | - | - | - | yes | yes | yes |
| Pause drawing in the background | handled in `poll` | - | - | - | browser | yes | yes |
| Sleep while idle | `PlatformConfig::wait_events` | yes | yes | yes | browser | yes | yes |
| Mouse capture during drags | handled in `poll` | yes | yes | yes | yes | - | - |

"yes" means the code path exists for that platform; see [STATUS.md](STATUS.md) for which of them have
actually been run.

### Following the system theme

```cpp
ui.set_theme(rd::sdl3::system_dark(plat) ? rd::Theme::dark() : rd::Theme::light());
// each frame, after poll():
if (rd::sdl3::system_theme_changed(plat)) { /* set the theme again */ }
```

### File dialogs and URLs

```cpp
if (rd::button(ui, "Open...").clicked) rd::sdl3::show_file_dialog(plat, rd::sdl3::FileDialog::Open);
std::string path;
if (rd::sdl3::poll_file_dialog(plat, path) && !path.empty()) load(path);   // on a later frame
if (rd::link(ui, "Website").clicked) rd::sdl3::open_url("https://example.com");
```

The dialog does not block the frame loop; the result arrives through `poll_file_dialog`.

### Saving power

```cpp
rd::sdl3::PlatformConfig pc;
pc.wait_events = true;   // poll() sleeps until input arrives while ui.animating() is false
```

`Context::animating()` is true while a transition runs, a finger is down, a toast is showing or a
spinner is visible. With `wait_events` the loop wakes at least every 250 ms so carets keep blinking.
If you draw your own animated content, keep rendering by not enabling it, or make your content call
`rd::animate`.

### Reduced motion

`ui.set_animations(false)` (or `Config::animations = false`) turns off every transition and flick
momentum. Widget results never depend on animations. Two things do move while animating: a drawer slides
(its hit area moves with it for about 0.2 s) and `transition_begin` content is offset by up to 14 px.

## Writing these hooks in your own backend

| Hook | Call it |
|---|---|
| `ctx.set_dpi_scale(scale)` | when the display scale changes |
| `Input::secondary_pressed` | on right mouse button down |
| `Input::touch`, `Input::safe_area` | every frame |
| `ctx.cursor()` | after `end_frame`: set the system cursor |
| `ctx.wants_text_input()`, `ctx.text_input_rect()` | after `end_frame`: show the keyboard / place the IME |
| `ctx.can_dismiss()` | on Android back: send `KeyEscape` if true, otherwise leave |
| `ctx.animating()` | after `end_frame`: decide whether to sleep |
