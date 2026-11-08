# Status (v0.1.0 candidate, branch `v0.1-dev`)

Environment: Linux (Artix, kernel 7.2), GCC 16.2.1, Clang, CMake 4.4.5, Ninja, system SDL 3.4.18, a live
desktop session. No emsdk, no Android NDK/SDK, no Xcode, no MSVC, no xvfb.

| Target | Built | Ran | Tested | Notes |
|---|---|---|---|---|
| Windows | No | No | No | Not verified: no Windows or MSVC here. CI job `windows` is written, never run. |
| Linux | Yes | Yes | Yes | GCC debug and Clang release presets; 57 test cases and the allocation test pass. Demo ran on the live desktop and wrote `docs/screenshots/m3.png`. |
| Web | No | No | Partly | Not verified: no emsdk. Project and CI job are written, never built. The core touch path is covered by unit tests only. |
| Android | No | No | Partly | Not verified: no NDK/SDK. Gradle project and CI job are written, never built. |
| iOS | No | No | Partly | Not verified: not macOS. CMake/Xcode project and CI job are written, never built. |

"Tested" for Web/Android/iOS means the shared core tests pass on Linux; nothing ran on those targets.

## Commands run and results

All from the repo root.

| Command | Result |
|---|---|
| `cmake --preset linux-gcc-debug && cmake --build --preset linux-gcc-debug && ctest --preset linux-gcc-debug` | Built with `-Wall -Wextra -Werror`; 2/2 tests pass (57 doctest cases, 216 assertions; allocation test) |
| `cmake --preset linux-clang-release && cmake --build --preset linux-clang-release && ctest --preset linux-clang-release` | Built; 2/2 tests pass. (Clang first failed on two unused demo fields; fixed.) |
| `scripts/check_core_includes.sh` | `check_core_includes: ok` |
| `./build/linux-gcc-debug/tests/test_alloc` (and the clang-release build) | `heap allocations over 200 frames after warm-up: 0`, pass. glibc `malloc` interposition was active. |
| `find include src/core backends examples tests \( -name '*.cpp' -o -name '*.hpp' \) \| xargs clang-format --dry-run --Werror` | No output, exit 0 |
| ASan + UBSan build of the tests (`-fsanitize=address,undefined`, core only) | All 57 cases and the allocation test pass, no reports |
| `RD_FONT=/usr/share/fonts/noto/NotoSans-Regular.ttf ./build/linux-gcc-debug/examples/desktop_demo/radon_desktop_demo --screenshot docs/screenshots/m3.png --frames 30` | Wrote a 960x600 PNG (checked by eye: windows, text, widgets, tooltip, dark theme) |
| Demo with no font found | Prints the "no font found" error and still runs; build succeeds without `font.ttf` |

## Not verified

- **Windows, Web, Android, iOS**: see the table. The CI jobs for them have never run.
- **Interactive demo use**: button clicks and slider drags are covered by unit tests with synthetic input;
  nobody clicked the real demo window. The screenshot was scripted, with the pointer hovering a button.
- **Screenshot method**: taken on the live desktop, not under xvfb (not installed). Only `m3.png` exists;
  there is no gallery beyond it.
- **Real touch and soft keyboard**: no device was used. `SDL_StartTextInput` / safe-area code is untested.
- **Light theme** is implemented and unit-tested for its values only; it was not looked at.
- **Intermediate milestone commits (M0-M4)** do not build on their own; see ADR 0006.
- **Font-dependent tests** ran against the system Noto Sans, not `examples/assets/font.ttf`, which does not
  exist yet. Without any font they print a skip message.
- Golden draw-list tests and the 60 s fuzz run listed in the plan's testing section are not written.
- `RD_DEBUG_IDS` (duplicate-ID report in debug builds) is not implemented.

## Release decision

`v0.1.0` is **not tagged**. M8 requires all five targets to build in CI, and only Linux has been built.
Missing: a green CI run on Windows, Web, Android and iOS; `examples/assets/font.ttf`; a screenshot gallery.
