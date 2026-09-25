#!/usr/bin/env python3
"""Grafika 2D gry Kart (assets/kart/) - Pillow, supersampling x4, kanal alfa.

    pip install pillow            (raz)
    python tools/gen_kart_assets.py

Gokarty, drzewa, skrzynki i flaga sa modelami 3D budowanymi w kodzie (src/games/kart/kart_render.cpp, gfx3d),
wiec tu powstaja tylko elementy 2D: ikony przedmiotow do HUD (banan, skorupa, grzyb), pas chmur i pas gor
na niebo, dym, poswiata slonca i plomien turbo. Wlasne PNG o tych samych nazwach i rozmiarach zastepuja te
pliki bez zmian w kodzie.
"""
import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "assets" / "kart"
SS = 4   # supersampling


def save(img, name):
    OUT.mkdir(parents=True, exist_ok=True)
    img.save(OUT / name, optimize=True)
    print(f"assets/kart/{name}  {img.width}x{img.height}")


def down(img, w, h):
    return img.resize((w, h), Image.LANCZOS)


def radial(size, inner, outer, power=1.0):
    """Kolo z gradientem od srodka (inner) do brzegu (outer), RGBA."""
    img = Image.new("RGBA", (size, size), outer)
    px = img.load()
    c = (size - 1) / 2
    for y in range(size):
        for x in range(size):
            d = min(1.0, math.hypot(x - c, y - c) / c)
            t = d ** power
            px[x, y] = tuple(int(inner[i] * (1 - t) + outer[i] * t) for i in range(4))
    return img


def with_shadow(img, S, cx, y, rx, ry, alpha=100):
    sh = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(sh).ellipse([cx - rx, y - ry, cx + rx, y + ry], fill=(0, 0, 0, alpha))
    sh = sh.filter(ImageFilter.GaussianBlur(1.5 * SS))
    return Image.alpha_composite(sh, img)


def banana():
    W = 48; S = W * SS
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx, cy = S / 2, S * 0.5
    box = [cx - 16 * SS, cy - 10 * SS, cx + 16 * SS, cy + 16 * SS]
    d.arc(box, 200, 340, fill=(250, 215, 60, 255), width=int(7 * SS))
    d.arc([box[0] + 1 * SS, box[1] + 2 * SS, box[2] - 1 * SS, box[3]], 215, 325, fill=(255, 240, 150, 255), width=int(2 * SS))
    for ang in (200, 340):
        ex = cx + 16 * SS * math.cos(math.radians(ang))
        ey = cy + 3 * SS + 13 * SS * math.sin(math.radians(ang))
        d.ellipse([ex - 3 * SS, ey - 3 * SS, ex + 3 * SS, ey + 3 * SS], fill=(120, 80, 30, 255))
    img = with_shadow(img, S, cx, S * 0.78, 14 * SS, 4 * SS)
    return down(img, W, W)


def shell():
    W = 48; S = W * SS
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx, cy = S / 2, S * 0.48
    d.ellipse([cx - 17 * SS, cy - 8 * SS, cx + 17 * SS, cy + 16 * SS], fill=(240, 232, 200, 255))
    d.ellipse([cx - 15 * SS, cy - 14 * SS, cx + 15 * SS, cy + 12 * SS], fill=(40, 140, 60, 255))
    d.ellipse([cx - 12 * SS, cy - 11 * SS, cx + 12 * SS, cy + 8 * SS], fill=(80, 200, 100, 255))
    for (ox, oy, r) in ((-6, -4, 3.2), (5, -5, 3.0), (0, 3, 3.4), (-7, 3, 2.4), (7, 2, 2.4)):
        d.ellipse([cx + ox * SS - r * SS, cy + oy * SS - r * SS, cx + ox * SS + r * SS, cy + oy * SS + r * SS], fill=(30, 110, 50, 255))
    d.ellipse([cx - 9 * SS, cy - 12 * SS, cx - 3 * SS, cy - 8 * SS], fill=(190, 240, 200, 200))
    img = with_shadow(img, S, cx, S * 0.84, 15 * SS, 4 * SS)
    return down(img, W, W)


def mushroom():
    W = 48; S = W * SS
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx = S / 2
    d.rounded_rectangle([cx - 8 * SS, S * 0.5, cx + 8 * SS, S * 0.82], radius=4 * SS, fill=(250, 232, 205, 255))
    d.ellipse([cx - 3 * SS, S * 0.58, cx - 1 * SS, S * 0.66], fill=(40, 30, 40, 255))
    d.ellipse([cx + 1 * SS, S * 0.58, cx + 3 * SS, S * 0.66], fill=(40, 30, 40, 255))
    d.chord([cx - 18 * SS, S * 0.12, cx + 18 * SS, S * 0.62], 180, 360, fill=(228, 40, 40, 255))
    d.rectangle([cx - 18 * SS, S * 0.37, cx + 18 * SS, S * 0.50], fill=(228, 40, 40, 255))
    for (ox, oy, r) in ((-9, -3, 4.5), (8, -2, 4), (0, -9, 3.5), (-1, 6, 3.6)):
        d.ellipse([cx + ox * SS - r * SS, S * 0.37 + oy * SS - r * SS, cx + ox * SS + r * SS, S * 0.37 + oy * SS + r * SS], fill=(255, 255, 255, 255))
    d.ellipse([cx - 12 * SS, S * 0.16, cx - 5 * SS, S * 0.24], fill=(255, 150, 150, 160))
    img = with_shadow(img, S, cx, S * 0.86, 12 * SS, 3.5 * SS)
    return down(img, W, W)


def clouds():
    W, H = 1024, 160
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    rng = random.Random(7)
    for _ in range(26):
        cx, cy = rng.uniform(0, W), rng.uniform(30, 110)
        for _ in range(rng.randint(4, 8)):
            rx, ry = rng.uniform(22, 60), rng.uniform(12, 26)
            ox, oy = rng.uniform(-40, 40), rng.uniform(-8, 8)
            a = rng.randint(150, 220)
            for xx in (cx + ox, cx + ox - W, cx + ox + W):   # zawijanie na krawedziach
                d.ellipse([xx - rx, cy + oy - ry, xx + rx, cy + oy + ry], fill=(255, 255, 255, a))
    # rozmycie tylko kanalu alfa (kolor zostaje bialy - bez ciemnej obwodki), dol chmur lekko szary
    a = img.split()[3].filter(ImageFilter.GaussianBlur(5))
    px = img.load()
    ap = a.load()
    for y in range(H):
        k = 1.0 - 0.18 * y / H
        for x in range(W):
            px[x, y] = (int(255 * k), int(255 * min(1.0, k + 0.03)), 255, ap[x, y])
    return img


def mountains():
    W, H = 2048, 160
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))

    def ridge(fn, top_col, bot_col, snow=None):
        layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        ld = ImageDraw.Draw(layer)
        pts = [(x, H - fn(x)) for x in range(0, W + 1, 2)]
        ld.polygon([(0, H)] + pts + [(W, H)], fill=(255, 255, 255, 255))
        grad = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        gd = ImageDraw.Draw(grad)
        for y in range(H):
            t = y / (H - 1)
            col = tuple(int(top_col[i] * (1 - t) + bot_col[i] * t) for i in range(3)) + (255,)
            gd.line([(0, y), (W, y)], fill=col)
        layer = Image.composite(grad, layer, layer.split()[3])
        if snow:
            sd = ImageDraw.Draw(layer)
            for x in range(0, W):
                h = fn(x)
                if h > snow:
                    sd.line([(x, H - h), (x, H - snow - (h - snow) * 0.35)], fill=(235, 240, 250, 255))
        img.alpha_composite(layer)

    def far(x):
        a = x / W * 2 * math.pi
        return 70 + 34 * math.sin(a * 3) + 18 * math.sin(a * 7 + 1.3) + 9 * math.sin(a * 17 + 0.4) + 4 * math.sin(a * 41)

    def near(x):
        a = x / W * 2 * math.pi
        return 30 + 16 * math.sin(a * 5 + 2) + 9 * math.sin(a * 11 + 0.7) + 4 * math.sin(a * 29 + 2)

    ridge(far, (150, 165, 205), (185, 205, 235), snow=105)
    ridge(near, (70, 125, 105), (150, 185, 190))
    return img


def smoke():
    return radial(64, (240, 240, 240, 200), (200, 200, 200, 0), power=0.9)


def glow():
    return radial(160, (255, 250, 220, 220), (255, 230, 150, 0), power=0.6)


def flame():
    W, H = 32, 64
    sheet = Image.new("RGBA", (W * 2, H), (0, 0, 0, 0))
    for f in range(2):
        img = Image.new("RGBA", (W * SS, H * SS), (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        cx = W * SS / 2
        L = (54 if f == 0 else 44) * SS
        for (k, col) in ((1.0, (255, 120, 30, 200)), (0.72, (255, 190, 60, 230)), (0.42, (255, 250, 210, 255))):
            w = 12 * SS * k
            d.polygon([(cx - w, 4 * SS), (cx + w, 4 * SS), (cx + w * 0.25, 4 * SS + L * k), (cx - w * 0.25, 4 * SS + L * k)], fill=col)
        img = img.filter(ImageFilter.GaussianBlur(1.2 * SS))
        sheet.alpha_composite(down(img, W, H), (f * W, 0))
    return sheet


def main():
    save(banana(), "banana.png")
    save(shell(), "shell.png")
    save(mushroom(), "mushroom.png")
    save(clouds(), "clouds.png")
    save(mountains(), "mountains.png")
    save(smoke(), "smoke.png")
    save(glow(), "glow.png")
    save(flame(), "flame.png")


if __name__ == "__main__":
    main()
