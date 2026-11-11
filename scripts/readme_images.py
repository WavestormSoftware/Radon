#!/usr/bin/env python3
"""Builds the README showcase images in docs/images/ from docs/screenshots/ (run scripts/screenshots.sh first).

Needs Pillow. Output: hero.png (desktop window with two phones on a glow), theme-split.png (dark/light
diagonal) and tile-*.png (framed feature shots).
"""
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SHOTS = os.path.join(ROOT, "docs", "screenshots")
OUT = os.path.join(ROOT, "docs", "images")
SS = 4  # supersampling for smooth rounded corners


def shot(name):
    return Image.open(os.path.join(SHOTS, name + ".png")).convert("RGBA")


def backdrop(w, h):
    """Wavestorm navy with a pink and a blue glow."""
    bg = Image.new("RGBA", (w, h), (11, 14, 18, 255))
    glow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(glow)
    d.ellipse((int(w * 0.45), int(-h * 0.35), int(w * 1.25), int(h * 0.75)), fill=(236, 72, 153, 120))
    d.ellipse((int(-w * 0.25), int(h * 0.35), int(w * 0.45), int(h * 1.35)), fill=(77, 163, 255, 80))
    glow = glow.filter(ImageFilter.GaussianBlur(min(w, h) * 0.18))
    return Image.alpha_composite(bg, glow)


def round_mask(size, radius):
    w, h = size
    m = Image.new("L", (w * SS, h * SS), 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w * SS - 1, h * SS - 1), radius * SS, fill=255)
    return m.resize(size, Image.LANCZOS)


def place(canvas, img, xy, radius, border=(255, 255, 255, 38), shadow=40):
    """Pastes `img` with rounded corners, a hairline border and a soft drop shadow."""
    x, y = xy
    w, h = img.size
    mask = round_mask(img.size, radius)
    sh = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    sm = Image.new("L", canvas.size, 0)
    sm.paste(mask, (x, y + shadow // 3))
    sh.putalpha(sm.point(lambda a: a * 170 // 255).filter(ImageFilter.GaussianBlur(shadow)))
    canvas.alpha_composite(sh)
    canvas.paste(img, (x, y), mask)
    ring = Image.new("L", (w * SS, h * SS), 0)
    ImageDraw.Draw(ring).rounded_rectangle((0, 0, w * SS - 1, h * SS - 1), radius * SS, outline=255, width=2 * SS)
    ring = ring.resize(img.size, Image.LANCZOS)
    layer = Image.new("RGBA", img.size, border)
    layer.putalpha(ImageChops.multiply(ring, Image.new("L", img.size, border[3])))
    canvas.alpha_composite(layer, (x, y))


def scaled(img, w):
    return img.resize((w, round(img.height * w / img.width)), Image.LANCZOS)


def hero():
    W, H = 1600, 900
    c = backdrop(W, H)
    place(c, scaled(shot("gallery-dashboard"), 1180), (70, 90), 14)
    place(c, scaled(shot("gallery-phone"), 300), (1110, 160), 30)
    place(c, scaled(shot("gallery-phone-charts"), 300), (1250, 250), 30)
    return c.convert("RGB")


def theme_split():
    dark, light = shot("gallery-widgets"), shot("gallery-light")
    w, h = dark.size
    m = Image.new("L", (w * SS, h * SS), 0)
    ImageDraw.Draw(m).polygon([(w * SS * 0.58, 0), (w * SS, 0), (w * SS, h * SS), (w * SS * 0.42, h * SS)], fill=255)
    m = m.resize((w, h), Image.LANCZOS)
    img = dark.copy()
    img.paste(light, (0, 0), m)
    W, H = 1600, 1000
    c = backdrop(W, H)
    place(c, scaled(img, 1440), (80, 80), 14)
    return c.convert("RGB")


def tile(name, crop=None):
    img = shot(name)
    if crop:
        img = img.crop(crop)
    img = scaled(img, 760)
    W, H = 840, img.height + 80
    c = backdrop(W, H)
    place(c, img, (40, 40), 12, shadow=24)
    return c.convert("RGB")


def main():
    os.makedirs(OUT, exist_ok=True)
    out = {
        "hero.png": hero(),
        "theme-split.png": theme_split(),
        "tile-charts.png": tile("gallery-charts", (217, 0, 1280, 760)),
        "tile-docking.png": tile("gallery-docking"),
        "tile-richtext.png": tile("gallery-richtext", (217, 0, 1280, 760)),
        "tile-data.png": tile("gallery-data", (217, 0, 1280, 760)),
    }
    for name, img in out.items():
        path = os.path.join(OUT, name)
        img.save(path, optimize=True)
        print("wrote", os.path.relpath(path, ROOT))


if __name__ == "__main__":
    sys.exit(main())
