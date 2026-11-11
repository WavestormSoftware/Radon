# Status

Environment: Linux, GCC 16.2.1, Clang, CMake 4.4.5, Ninja, system SDL 3.4.18, a live Wayland
desktop session, Android SDK 34 / NDK 27 / Gradle. No emsdk, no Xcode, no MSVC, no Android device.

| Target | Built | Ran | Tested | Notes |
|---|---|---|---|---|
| Linux | Yes | Yes | Yes | GCC debug + Clang release, `-Wall -Wextra -Werror`; 95 test cases + allocation test pass; ASan/UBSan clean. Gallery and 11 examples render on the live desktop. |
| Android | Yes (unsigned APK, arm64-v8a) | No | Core only | Built with NDK 27 against SDL 3.2.8. **Never installed on a device or emulator.** |
| Windows | No | No | No | No Windows/MSVC here. CI job written, never run. |
| macOS | No | No | No | Preset and CI job written, never run. |
| Web | No | No | Core only | No emsdk here. Project and CI job written, never built. |
| iOS | No | No | Core only | Not macOS. CMake/Xcode project and CI job written, never built. |

"Core only" means the shared core tests pass on Linux; nothing ran on that target.

## Commands run and results

| Command | Result |
|---|---|
| `cmake --preset linux-gcc-debug && cmake --build ... && ctest ...` | 2/2 pass (95 cases / 394 assertions, allocation test) |
| `cmake --preset linux-clang-release && cmake --build ... && ctest ...` | 2/2 pass |
| `scripts/check_core_includes.sh` | `check_core_includes: ok` |
| `tests/test_alloc` | 0 heap allocations over 200 frames after warm-up, with every widget including the 0.2 set, toasts and context menus |
| ASan + UBSan build of the core tests (`-fsanitize=address,undefined`) | all pass, no reports |
| `clang-format -i` over `include src/core backends examples tests` | applied |
| `cmake --install` + `tests/install` consumer with `find_package(Radon)` | builds and runs |
| `scripts/screenshots.sh` | regenerated `docs/screenshots/*.png`; the new pages, light theme, phone layout and bottom sheet were checked by eye |
| `gradle assembleRelease -Psdl3Dir=... -PfontFile=...` | BUILD SUCCESSFUL, `app-release-unsigned.apk` |
| Live gallery left idle for 4 s with `wait_events` | about 1% CPU |

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

`v0.2.0` is not tagged: it needs green CI on Windows, macOS, Web, Android and iOS, and one pass on a
real Android or iOS device.
