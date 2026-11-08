# 0002: SDL3 via FetchContent, preferring an installed copy

SDL3 is declared with `FetchContent` pinned to tag `release-3.2.8`, with `FIND_PACKAGE_ARGS 3.2 CONFIG`.
If a suitable SDL3 is installed, CMake uses it; otherwise it fetches and builds the pinned tag. This keeps
the mandated FetchContent path for CI, web and mobile (which have no system SDL3) and avoids a long SDL
build on developer machines. This machine used the installed SDL 3.4.18.
