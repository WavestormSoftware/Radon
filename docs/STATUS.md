# Status

Environment: Linux (Artix), GCC 16.2.1, Clang, CMake 4.4.5, Ninja, system SDL 3.4.18, a live desktop
session, Android SDK 34 / NDK 27 / Gradle 8.9 (installed for the APK build). No emsdk, no Xcode, no MSVC.

| Target | Built | Ran | Tested | Notes |
|---|---|---|---|---|
| Windows | No | No | No | No Windows/MSVC here. CI job written, never run. |
| Linux | Yes | Yes | Yes | GCC debug + Clang release; 75 test cases + allocation test pass; ASan/UBSan clean. Gallery and 10 examples run on the live desktop. |
| Web | No | No | Core only | No emsdk here (installing it was not permitted in this session). Project and CI job written, never built. Instructions: `docs/building.md`. |
| Android | Yes (unsigned + debug-signed APK) | No | Core only | Built with NDK 27 / Gradle 8.9. The APK was not installed on a device or emulator. |
| iOS | No | No | Core only | Not macOS. CMake/Xcode project and CI job written, never built. |

"Core only" means the shared core tests pass on Linux; nothing ran on that target.

## Commands run and results

| Command | Result |
|---|---|
| `cmake --preset linux-gcc-debug && cmake --build --preset linux-gcc-debug && ctest --preset linux-gcc-debug` | Built with `-Wall -Wextra -Werror`; 2/2 pass (75 cases, allocation test) |
| `cmake --preset linux-clang-release && cmake --build --preset linux-clang-release && ctest --preset linux-clang-release` | Built; 2/2 pass |
| `scripts/check_core_includes.sh` | `check_core_includes: ok` |
| `build/<preset>/tests/test_alloc` | 0 heap allocations over 200 frames after warm-up, with the whole widget set (menus, modal, color picker, plots...) |
| `xargs clang-format --dry-run --Werror` on `include src/core backends examples tests` | clean |
| ASan + UBSan build of the core tests | all pass, no reports |
| `scripts/screenshots.sh` | regenerated `docs/screenshots/*.png` (gallery pages, light theme, phone width, 10 examples); checked by eye |
| `gradle assembleRelease -Psdl3Dir=... -PfontFile=...` in `examples/mobile_demo/android` | BUILD SUCCESSFUL; `app-release-unsigned.apk` (arm64-v8a) |

## Not verified

- **Windows, Web, iOS** builds, and all CI jobs (none have run).
- **The APK on a device**: built and signature-verified only. Touch, soft keyboard and safe-area code are untested on hardware.
- **Mouse interaction in the real window**: clicks and drags are covered by unit tests with synthetic input; the screenshots are scripted with a fixed pointer. Nobody drove the live windows.
- **Light theme** was checked by eye in one screenshot only.
- **Font-dependent tests** run against a system Noto Sans because `examples/assets/font.ttf` is not in the repo; without any font they print a skip message.
- Golden draw-list tests and a timed fuzz run are not written. `RD_DEBUG_IDS` is not implemented.
- Intermediate milestone commits M1-M5 do not build on their own (ADR 0006).

## Release

`v0.1.0` is not tagged: it needs green CI on Windows, Web, Android and iOS, and the owner's `font.ttf`.
