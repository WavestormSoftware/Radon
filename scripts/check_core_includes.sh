#!/usr/bin/env bash
# Fails if the core (include/ and src/core/) includes any SDL, OpenGL or platform header.
set -u
cd "$(dirname "$0")/.."
pattern='^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"](SDL|GL/|GLES|OpenGL/|EGL/|emscripten|android/|windows\.h|Windows\.h|X11/|wayland|UIKit|Cocoa|jni\.h|unistd\.h|sys/)'
if grep -rnE "$pattern" include src/core; then
    echo "check_core_includes: FAILED (platform header in the core)" >&2
    exit 1
fi
echo "check_core_includes: ok"
