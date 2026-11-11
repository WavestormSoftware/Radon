# Status

Environment: Linux, GCC 16.2.1, Clang, CMake 4.4.5, Ninja, system SDL 3.4.18, a live Wayland
desktop session, Android SDK 34 / NDK 27 / Gradle. No emsdk, no Xcode, no MSVC, no Android device.

| Target | Built | Ran | Tested | Notes |
|---|---|---|---|---|
| Linux | Yes | Yes | Yes | GCC debug + Clang release, `-Wall -Wextra -Werror`; 129 test cases + allocation test pass; ASan/UBSan clean. Gallery and 11 examples render on the live desktop. |
| Android | Yes (unsigned APK, arm64-v8a) | No | Core only | Built with NDK 27 against SDL 3.2.8, including the 0.3 haptics (JNI) and keyboard-type code. **Never installed on a device or emulator.** |
| Windows | No | No | No | No Windows/MSVC here. CI job written, never run. |
| macOS | No | No | No | Preset and CI job written, never run. |
| Web | No | No | Core only | No emsdk here. Project and CI job written, never built. |
| iOS | No | No | Core only | Not macOS. CMake/Xcode project and CI job written, never built. |

"Core only" means the shared core tests pass on Linux; nothing ran on that target.

## Commands run and results

| Command | Result |
|---|---|
| `cmake --preset linux-gcc-debug && cmake --build ... && ctest ...` | 2/2 pass (129 cases / 513 assertions, allocation test) |
| `cmake --preset linux-clang-release && cmake --build ... && ctest ...` | 2/2 pass |
| `scripts/check_core_includes.sh` | `check_core_includes: ok` |
| `tests/test_alloc` | 0 heap allocations over 200 frames after warm-up, with every widget including the 0.3 set (pickers, charts, tables, carousel, accordion, rich text, Markdown, drag and drop, swipe rows, pull to refresh, docking), two-finger input, undo and the accessibility tree on |
| ASan + UBSan build of the core tests (`-fsanitize=address,undefined`) | all pass, no reports |
| `clang-format -i` over `include src/core backends examples tests` | applied |
| `cmake --install` + `tests/install` consumer with `find_package(Radon)` | builds and runs |
| `scripts/screenshots.sh` | regenerated `docs/screenshots/*.png`; the new pages, light theme, phone layout and bottom sheet were checked by eye |
| `gradle assembleRelease -Psdl3Dir=... -PfontFile=...` | BUILD SUCCESSFUL, `app-release-unsigned.apk` |
| Live gallery left idle for 4 s with `wait_events` | about 1% CPU |

## 0.3 features (gestures, pickers, charts, docking, rich text, drag and drop, accessibility)

- Linux: unit tests drive each feature with synthetic input: pinch and swipe recognition, swipe-to-dismiss
  sheets, pull to refresh, swipe rows, edge swipes, undo/redo and word editing, keyboard type, keyboard
  avoidance, drag and drop, reordering, wheel snapping, autocomplete, number pad, chart axis steps, column
  resizing and sorting, flow and grid layout, carousel snapping, accordion, docking (side placement, tab
  undock, redock by drop zone, close), rich text link clicks, read-only text and the accessibility tree
  with screen-reader activation. The gallery pages were rendered and checked by eye at desktop and phone
  size.
- Multi-touch, haptics, the soft-keyboard type and the keyboard inset estimate ran only in unit tests or
  were only compiled: there is no touch screen or phone here. iOS haptics (`haptics_ios.m`) was never
  compiled.
- Accessibility stops at the tree: no platform screen reader is connected yet.
- Color emoji (CBDT/COLR) are not supported; fallback fonts render as one color.

## Game integration (viewport, render targets, canvas, C API, raylib, gamepad, audio)

- Linux: unit tests cover the viewport and canvas widgets and the C API; the viewport, canvas and raylib demos were rendered and checked by eye. Audio was checked with SDL's dummy driver (WAV load, 8-voice cap).
- CI builds and runs the headless tests on every platform. Nothing game-related was run on a GPU outside Linux.
- Never run: gamepads (no controller here), the render-target code on GLES3/WebGL2, audio on a real device.

## Not verified

- **Windows, macOS, Web, iOS**: never built. Every CI job except Linux has never run.
- **Anything on a phone.** The APK builds, but the display-scale sizing, flick scrolling, long-press,
  soft keyboard placement, back button, safe area and background pause have only been exercised through
  unit tests with synthetic input and by resizing the desktop window to phone size. Expect to tune on a
  real device.
- **Native features on the desktop**: system theme changes, cursors, the file dialog, `open_url` and IME
  placement compile against SDL 3.2.8 and 3.4, but nobody clicked through them in a live window. Drawers,
  toasts and the date popup were checked with scripted clicks in screenshots only.
- **Fractional display scales** (125%, 150%): the conversion is written, not run on such a display.
- **Font-dependent tests** use a system Noto Sans; without any font they print a skip message.
- Golden draw-list tests and a fuzz run are not written.
- There is no screen-reader accessibility support and no multi-touch.

## Release

`v0.3.0` (and `v0.2.0`) are not tagged: it needs green CI on Windows, macOS, Web, Android and iOS, and one pass on a
real Android or iOS device.
