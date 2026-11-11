# Third-party notices

Radon itself is MIT licensed (see [LICENSE](LICENSE)). It includes or links the following.

## Vendored in `src/third_party/`

| Component | Use | License | Text |
|---|---|---|---|
| [stb_truetype.h](https://github.com/nothings/stb) | Font rasterizing | MIT or public domain | [LICENSE.stb](src/third_party/LICENSE.stb) |
| [stb_rect_pack.h](https://github.com/nothings/stb) | Glyph atlas packing | MIT or public domain | [LICENSE.stb](src/third_party/LICENSE.stb) |
| [doctest](https://github.com/doctest/doctest) | Unit tests only; not linked into the library | MIT | [LICENSE.doctest](src/third_party/LICENSE.doctest) |

## Fonts in `examples/showcase/fonts/` (examples only, not part of the library)

| Component | Use | License | Text |
|---|---|---|---|
| [IBM Plex Sans and Mono](https://github.com/IBM/plex) | Showcase typefaces | SIL Open Font License 1.1 | [OFL-IBMPlex.txt](examples/showcase/fonts/OFL-IBMPlex.txt) |
| [Bricolage Grotesque](https://github.com/ateliertriay/bricolage) | Showcase typeface | SIL Open Font License 1.1 | [OFL-BricolageGrotesque.txt](examples/showcase/fonts/OFL-BricolageGrotesque.txt) |

## Linked, not vendored

- [SDL3](https://libsdl.org) (zlib license) is used by `backends/sdl3_gl` and the demos. The core library
  does not depend on it. SDL3 is found on the system or fetched at configure time.
- The demos load a TrueType font at run time. No font ships with this repository; if you redistribute a
  build that bundles one, you must follow that font's license.
