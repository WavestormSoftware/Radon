# Building and running

Requirements: CMake 3.20+, a C++17 compiler, Ninja (recommended). SDL3 is found on the system or fetched
(release 3.2.8) by CMake. The demos need a TrueType font (`examples/assets/font.ttf`, or `RD_FONT`, or
`--font`).

## Linux

```sh
cmake --preset linux-gcc-debug          # or linux-clang-release
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug          # 75+ tests and the allocation test
scripts/check_core_includes.sh          # the core must include no platform headers
```
SDL needs the usual development packages when it is built from source (X11/Wayland, GL, ALSA headers).

Executables: `build/<preset>/examples/desktop_demo/radon_desktop_demo` (the gallery) and
`build/<preset>/examples/radon_ex_*` (ten examples).

## Windows

```bat
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-release
```
Executables land in `build\windows-msvc-release\examples\...\Release`. (The CI job exists; this repo's
author has not run it on Windows.)

## macOS

Needs Xcode command line tools and CMake. SDL3 is fetched and built if it is not installed
(`brew install sdl3` is faster).

```sh
cmake --preset macos-release
cmake --build --preset macos-release
ctest --preset macos-release
```
(The CI job exists; this has not been built on a Mac yet.)

## Web (WebGL2)

You need [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) (tested target: 3.1.74+).

```sh
# 1. install and activate emsdk once
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
~/emsdk/emsdk install latest && ~/emsdk/emsdk activate latest
source ~/emsdk/emsdk_env.sh               # sets EMSDK for this shell

# 2. put a font where the build can embed it (the web build has no file system)
cp /path/to/YourFont.ttf examples/assets/font.ttf

# 3. configure and build
cd Radon
cmake --preset web-emscripten-release
cmake --build --preset web-emscripten-release
```

Output (`build/web-emscripten-release/examples/web_demo/`): `radon_web.html`, `radon_web.js`,
`radon_web.wasm`. There is no server component, but browsers refuse to load `.wasm` from `file://`, so
serve the folder:

```sh
cd build/web-emscripten-release/examples/web_demo
python3 -m http.server 8080          # then open http://localhost:8080/radon_web.html
# or: emrun --port 8080 radon_web.html
```

Notes: WebGL2 is required (any current browser). Touch works on phones/tablets. The font is baked into the
`.wasm` via `--embed-file`; without `examples/assets/font.ttf` the page runs with no text. To host it, upload
the three files; the HTML shell is `examples/web_demo/shell.html`.

## Android

Needs: JDK 17+, Android SDK (platform 34, build-tools 34), NDK 27, Gradle 8.x, CMake 3.22 from the SDK.

```sh
git clone --depth 1 --branch release-3.2.8 https://github.com/libsdl-org/SDL.git /tmp/SDL   # Java sources
cd examples/mobile_demo/android
gradle assembleRelease -Psdl3Dir=/tmp/SDL
# -> app/build/outputs/apk/release/app-release-unsigned.apk
```
Install it for testing: sign with a debug key and `adb install` (the build produces an **unsigned** APK):
```sh
apksigner sign --ks ~/.android/debug.keystore --ks-pass pass:android app-release-unsigned.apk
adb install -r app-release-unsigned.apk
```
The font is copied into the APK assets from `examples/assets/font.ttf` when it exists, or from
`-PfontFile=/path/to/font.ttf`.

## iOS

macOS with Xcode only. A simulator build needs no signing:
```sh
cmake --preset ios-simulator-release
cmake --build --preset ios-simulator-release -- -sdk iphonesimulator CODE_SIGNING_ALLOWED=NO
```
Device builds need your Apple developer team in the generated Xcode project.

## CMake options

| Option | Default | |
|---|---|---|
| `RD_BUILD_EXAMPLES` | ON on desktop | the gallery and examples |
| `RD_BUILD_TESTS` | ON on desktop | doctest suite + allocation test |
| `RD_BACKEND_SDL3_GL` | ON when top-level, OFF as a subproject | the SDL3/GL backend (fetches SDL3) |
| `RD_WARNINGS_AS_ERRORS` | OFF (ON in presets) | `-Werror` / `/WX` |
| `RD_INSTALL` | ON when top-level on desktop | install rules for the core and `find_package(Radon)` |

## Using Radon from your project

```cmake
set(RD_BACKEND_SDL3_GL ON)                                 # only if you want the SDL3 backend (fetches SDL3)
add_subdirectory(Radon)                                    # or FetchContent_Declare(...)
target_link_libraries(app PRIVATE radon::radon radon::sdl3_gl)
```
As a subproject Radon builds only the libraries (no demos, tests or install rules).

The dependency-free core can also be installed:

```sh
cmake --install build/linux-clang-release --prefix /usr/local
```
```cmake
find_package(Radon 0.2 REQUIRED)
target_link_libraries(app PRIVATE radon::radon)            # bring your own backend
```
The SDL3 backend is not installed; use `add_subdirectory` / FetchContent when you want it.

## Troubleshooting

- **"no font found"**: the demos look for `examples/assets/font.ttf`, then common system fonts. Provide
  a TTF with `--font` or `RD_FONT`. The UI runs without text.
- **The UI is tiny or huge on a phone**: lay out in logical pixels (`rd::sdl3::logical_size`), keep
  `ctx.set_dpi_scale(rd::sdl3::dpi_scale(plat))` updated, and use `rd::Theme::touch()`.
- **Nothing is clickable on the first frame**: expected; hover uses the previous frame's windows.
- **`ctx.error()` is true**: a pool is full. Raise the matching `rd::Config` limit.
- **Text looks blurry on high-DPI screens**: call `ctx.set_dpi_scale(rd::sdl3::dpi_scale(plat))` (the
  runner and `attach()` do).
