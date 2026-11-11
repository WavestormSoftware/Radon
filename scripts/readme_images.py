#!/usr/bin/env python3
"""Builds the README images in docs/images/ from the showcase screenshots in docs/screenshots/.

Run scripts/screenshots.sh first (it calls this script at the end). Needs Pillow.
Output: hero.png (Tilekit with two Stride phones) and showcase-*.png, one per app, each on a flat backdrop
in that app's own signature color.
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


def rgb(h):
    return tuple(int(h[i : i + 2], 16) for i in (1, 3, 5))


def backdrop(w, h, color, dots=None):
    """A flat color, optionally with a faint dot grid (the layout grid Radon places widgets on)."""
    bg = Image.new("RGBA", (w, h), rgb(color) + (255,))
    if dots:
        d = ImageDraw.Draw(bg)
        for y in range(16, h, 28):
            for x in range(16, w, 28):
                d.ellipse((x - 1, y - 1, x + 1, y + 1), fill=dots)
    return bg


def round_mask(size, radius):
    w, h = size
    m = Image.new("L", (w * SS, h * SS), 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w * SS - 1, h * SS - 1), radius * SS, fill=255)
    return m.resize(size, Image.LANCZOS)


def place(canvas, img, xy, radius, shadow=36, strength=120, ring=(255, 255, 255, 30)):
    """Pastes `img` with rounded corners, a hairline edge and a soft drop shadow."""
    x, y = xy
    mask = round_mask(img.size, radius)
    sm = Image.new("L", canvas.size, 0)
    sm.paste(mask, (x, y + shadow // 2))
    sh = Image.new("RGBA", canvas.size, (0, 0, 0, 255))
    sh.putalpha(sm.filter(ImageFilter.GaussianBlur(shadow)).point(lambda a: a * strength // 255))
    canvas.alpha_composite(sh)
    canvas.paste(img, (x, y), mask)
    if ring:
        w, h = img.size
        edge = Image.new("L", (w * SS, h * SS), 0)
        ImageDraw.Draw(edge).rounded_rectangle((0, 0, w * SS - 1, h * SS - 1), radius * SS, outline=255, width=2 * SS)
        edge = edge.resize(img.size, Image.LANCZOS)
        layer = Image.new("RGBA", img.size, ring[:3] + (0,))
        layer.putalpha(ImageChops.multiply(edge, Image.new("L", img.size, ring[3])))
        canvas.alpha_composite(layer, (x, y))


def phone(canvas, img, xy, width, shadow=40):
    """A phone screen in a thin dark bezel."""
    screen = img.resize((width, round(img.height * width / img.width)), Image.LANCZOS)
    bezel = max(6, width // 40)
    body = Image.new("RGBA", (screen.width + 2 * bezel, screen.height + 2 * bezel), (8, 10, 14, 255))
    body.paste(screen, (bezel, bezel), round_mask(screen.size, width // 9))
    place(canvas, body, xy, width // 9 + bezel, shadow=shadow, ring=(255, 255, 255, 40))


def scaled(img, w):
    return img.resize((w, round(img.height * w / img.width)), Image.LANCZOS)


def hero():
    W, H = 1800, 1080
    c = backdrop(W, H, "#0F1217", dots=(255, 255, 255, 7))
    place(c, scaled(shot("showcase-tilekit"), 1360), (70, 120), 16, ring=(255, 255, 255, 36))
    phone(c, shot("showcase-stride-plan"), (1250, 250), 300)
    phone(c, shot("showcase-stride-today"), (1440, 190), 300)
    return c


def framed(img, color, width=1420, radius=16):
    W, H = 1600, 1060
    c = backdrop(W, H, color)
    img = scaled(img, width)
    place(c, img, ((W - img.width) // 2, (H - img.height) // 2), radius, strength=150, ring=None)
    return c


def liveops():
    """The dashboard in both themes, split on a diagonal."""
    dark, light = shot("showcase-liveops-dark"), shot("showcase-liveops-light")
    w, h = dark.size
    m = Image.new("L", (w, h), 0)
    ImageDraw.Draw(m).polygon([(w * 0.6, 0), (w, 0), (w, h), (w * 0.4, h)], fill=255)
    img = dark.copy()
    img.paste(light, (0, 0), m)
    return framed(img, "#6D4AFF")


def stride():
    c = backdrop(1600, 1060, "#FFD84D")
    phone(c, shot("showcase-stride-today"), (360, 87), 400, shadow=46)
    phone(c, shot("showcase-stride-plan"), (820, 87), 400, shadow=46)
    return c


def tilekit():
    """The scene and inspector at a size where the pixel art and the controls are legible."""
    src = shot("showcase-tilekit")
    s = src.width / 1440  # the shot is rendered at --scale 1.5
    return framed(src.crop((int(190 * s), int(52 * s), int(1440 * s), int(650 * s))), "#EC4899")


def main():
    os.makedirs(OUT, exist_ok=True)
    out = {
        "hero.png": hero(),
        "showcase-halcyon.png": framed(shot("showcase-halcyon"), "#FF5A1F", radius=22),
        "showcase-liveops.png": liveops(),
        "showcase-stride.png": stride(),
        "showcase-tilekit.png": tilekit(),
    }
    for name, img in out.items():
        path = os.path.join(OUT, name)
        img.convert("RGB").save(path, optimize=True)
        print("wrote", os.path.relpath(path, ROOT))


if __name__ == "__main__":
    sys.exit(main())
