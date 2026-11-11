# Contributing to Radon

Thanks for helping. Bug reports, fixes, new widgets, backends and documentation are all welcome.

## Build and test

```sh
cmake --preset linux-gcc-debug        # or linux-clang-release, macos-release, windows-msvc-release
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

Before opening a pull request, run what CI runs:

```sh
scripts/check_core_includes.sh        # the core must not include platform headers
find include src/core backends examples tests -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

Other platforms are described in [docs/building.md](docs/building.md).

## Ground rules

- **The core has no dependencies.** Nothing in `include/` or `src/core/` may include SDL, OpenGL or an
  OS header. Platform work goes in `backends/`.
- **No allocation after the `Context` constructor.** State lives in fixed pools sized by `rd::Config`.
  When a pool is full, skip the work and set the error flag; never crash. `tests/test_alloc.cpp` counts
  allocations, so add new widgets to it.
- **Animations are optional.** A widget must return the same results with `Config::animations = false`.
- **Every widget works with mouse, touch and keyboard.** Focusable controls react to Enter or Space and
  draw the focus ring.
- **New behaviour needs a test.** The tests drive the core with synthetic input (`tests/harness.hpp`);
  no window is needed.
- Follow `.clang-format`. Comments explain why, not what.

## Adding a widget

1. Declare it in `include/radon/widgets.hpp` with a doc comment.
2. Implement it under `src/core/widgets/` using `layout_next`, `update_control` and the draw list.
3. Add a test, add it to `tests/test_alloc.cpp`, show it in `examples/common/gallery.cpp`.
4. Describe it in `docs/widgets.md` and add a line to `CHANGELOG.md`.

## Pull requests

Keep them focused: one change per pull request, with the reason in the description. Say which platforms
you built and ran on; [docs/STATUS.md](docs/STATUS.md) records what has really been verified.

By contributing you agree that your work is released under the [MIT license](LICENSE).
