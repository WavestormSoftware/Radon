#!/usr/bin/env bash
# Regenerates docs/screenshots/*.png from a built tree (default: build/linux-clang-release).
# Needs a window system (X11/Wayland) and a font: set RD_FONT or put one at examples/assets/font.ttf.
set -eu
cd "$(dirname "$0")/.."
BUILD="${1:-build/linux-clang-release}"
OUT=docs/screenshots
mkdir -p "$OUT"
shot() { # name binary size [extra args...]
  local name=$1 bin=$2 size=$3; shift 3
  "$BUILD/$bin" --size "$size" --dark --screenshot "$OUT/$name.png" --frames 40 "$@" >/dev/null
  echo "wrote $OUT/$name.png"
}
# Gallery pages the docs show (the page number is the gallery's sidebar order)
for page in 0:widgets 1:inputs 2:layout 3:lists 6:data 7:motion 8:charts 12:docking; do
  shot "gallery-${page#*:}" examples/desktop_demo/radon_desktop_demo 1280x760 --page "${page%%:*}" --pointer 640,700
done
shot gallery-light examples/desktop_demo/radon_desktop_demo 1280x760 --page 0 --light --pointer 640,700
shot gallery-phone examples/desktop_demo/radon_desktop_demo 420x860 --page 0 --pointer 10,10
shot gallery-phone-gestures examples/desktop_demo/radon_desktop_demo 420x860 --page 11 --pointer 10,10
shot ex-11-mobile-app examples/radon_ex_11_mobile_app 400x800 --pointer 10,10
shot ex-01-hello examples/radon_ex_01_hello 480x280 --pointer 120,110
shot ex-02-settings-form examples/radon_ex_02_settings_form 520x540 --pointer 10,10
shot ex-03-todo-list examples/radon_ex_03_todo_list 480x520 --pointer 10,10
shot ex-04-theme-editor examples/radon_ex_04_theme_editor 800x700 --pointer 10,10
shot ex-05-layout-cookbook examples/radon_ex_05_layout_cookbook 580x700 --pointer 10,10
shot ex-06-menus-and-dialogs examples/radon_ex_06_menus_and_dialogs 600x400 --pointer 10,10
shot ex-07-live-plots examples/radon_ex_07_live_plots 620x480 --pointer 10,10
shot ex-08-palette examples/radon_ex_08_palette 440x580 --pointer 10,10
shot ex-10-dashboard examples/radon_ex_10_dashboard 1280x760 --pointer 10,10
shot ex-09-calculator examples/radon_ex_09_calculator 360x470 --pointer 10,10

# Showcase apps, rendered 1.5x sharper for the README images
show() { # name page size [extra args...]
  local name=$1 page=$2 size=$3; shift 3
  "$BUILD/examples/showcase/radon_showcase" --page "$page" --size "$size" --scale 1.5 --pointer 1,1 \
    --screenshot "$OUT/$name.png" --frames 40 "$@" >/dev/null
  echo "wrote $OUT/$name.png"
}
show showcase-tilekit 0 1440x900
show showcase-halcyon 1 1240x780
show showcase-stride-today 2 390x844
show showcase-stride-plan 3 390x844
show showcase-liveops-light 4 1440x900 --light
show showcase-liveops-dark 4 1440x900 --dark
# README images (hero and one image per showcase app); needs Pillow
if python3 -c "import PIL" 2>/dev/null; then python3 scripts/readme_images.py; else echo "skipped README images: pip install pillow"; fi
