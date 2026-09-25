#!/usr/bin/env python3
"""Atlas tekstur gry Kart (assets/kart/atlas.png) - Pillow, 512x512, PNG 8-bit z paleta (indeks 0 = przezroczysty).

    pip install pillow            (raz)
    python tools/gen_kart_atlas.py

Uklad kafelkow (x, y, w, h) - te same liczby sa w src/games/kart/kart_render.cpp (tabela ATLAS_*):
    asfalt        0,0,128,128     trawa 128,0,128,128     trawa sucha 256,0,128,128
    publicznosc 384,0,128,32      banery 384,32 / 384,64 / 384,96 (128x32)
    drzewo okragle 0,128,128,128  choinka 128,128,128,128
    krzak 256,128,64,64           krzak jasny 320,128,64,64      bieznik 256,192,64,64      brama 0,384,448,40
    malowania bolidow 0/64/128/192, 256, 64x128 (czerwony, niebieski, zielony, zolty; gora = przod)
Deterministyczne (stale ziarno) - ten sam plik przy kazdym uruchomieniu. Wlasny PNG o tym samym ukladzie
(512x512, tryb P, indeks 0 = przezroczysty) zastepuje ten plik bez zmian w kodzie.
"""
import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "assets" / "kart" / "atlas.png"
W = H = 512
KART_COLORS = [(228, 40, 44), (44, 104, 228), (56, 186, 78), (250, 200, 44)]


def font(size):
    for name in ("arialbd.ttf", "DejaVuSans-Bold.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def noise_tile(w, h, base, amp, rng, blur=0.0, seed_pts=0):
    img = Image.new("RGB", (w, h), base)
    px = img.load()
    for y in range(h):
        for x in range(w):
            k = rng.uniform(-amp, amp)
            px[x, y] = tuple(max(0, min(255, int(c + k))) for c in base)
    if blur > 0:
        img = img.filter(ImageFilter.GaussianBlur(blur))
    return img


def asphalt(rng):
    img = noise_tile(128, 128, (86, 86, 94), 14, rng, blur=0.6)
    d = ImageDraw.Draw(img)
    # ciemniejsze plamy i drobne pekniecia
    for _ in range(14):
        cx, cy, r = rng.uniform(0, 128), rng.uniform(0, 128), rng.uniform(6, 18)
        for ox in (-128, 0, 128):
            for oy in (-128, 0, 128):
                d.ellipse([cx + ox - r, cy + oy - r, cx + ox + r, cy + oy + r], fill=(78, 78, 86))
    img = img.filter(ImageFilter.GaussianBlur(1.2))
    d = ImageDraw.Draw(img)
    for _ in range(6):
        x, y = rng.uniform(0, 128), rng.uniform(0, 128)
        for _ in range(8):
            nx, ny = x + rng.uniform(-8, 8), y + rng.uniform(-8, 8)
            d.line([(x, y), (nx, ny)], fill=(62, 62, 70), width=1)
            x, y = nx, ny
    # ziarno na wierzchu
    px = img.load()
    for y in range(128):
        for x in range(128):
            k = rng.randint(-7, 7)
            px[x, y] = tuple(max(0, min(255, c + k)) for c in px[x, y])
    return img


def grass(rng, base, blade, flowers, blades=900, blur_end=0.0):
    img = noise_tile(128, 128, base, 16, rng, blur=0.8)
    d = ImageDraw.Draw(img)
    for _ in range(blades):
        x, y = rng.uniform(0, 128), rng.uniform(0, 128)
        l = rng.uniform(2, 5)
        a = rng.uniform(-0.5, 0.5)
        col = tuple(max(0, min(255, c + rng.randint(-10, 18))) for c in blade)
        for ox in (-128, 0, 128):
            for oy in (-128, 0, 128):
                d.line([(x + ox, y + oy), (x + ox + math.sin(a) * l, y + oy - l)], fill=col, width=1)
    for _ in range(flowers):
        x, y = rng.uniform(0, 128), rng.uniform(0, 128)
        col = rng.choice([(250, 240, 120), (250, 250, 250), (240, 120, 160)])
        d.ellipse([x - 1.5, y - 1.5, x + 1.5, y + 1.5], fill=col)
    if blur_end > 0:
        img = img.filter(ImageFilter.GaussianBlur(blur_end))   # gladsza wersja na dalekie plany (bez mipmap mniej migocze)
    return img


def crowd(rng):
    img = Image.new("RGB", (128, 32), (40, 44, 60))
    d = ImageDraw.Draw(img)
    cols = [(240, 70, 60), (60, 120, 240), (250, 210, 60), (240, 240, 240), (60, 200, 110), (250, 140, 40), (200, 80, 220)]
    for row in range(2):
        for i in range(21):
            x = 3 + i * 6 + rng.uniform(-1, 1)
            y = 6 + row * 14 + rng.uniform(-1, 1)
            c = rng.choice(cols)
            d.ellipse([x - 2, y - 2, x + 2, y + 2], fill=(226, 190, 160))         # glowa
            d.rectangle([x - 3, y + 2, x + 3, y + 10], fill=c)                       # tulow
    return img


def banner(text, bg, fg):
    img = Image.new("RGB", (128, 32), bg)
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, 127, 31], outline=(255, 255, 255), width=2)
    f = font(20)
    bbox = d.textbbox((0, 0), text, font=f)
    d.text(((128 - (bbox[2] - bbox[0])) / 2 - bbox[0], (32 - (bbox[3] - bbox[1])) / 2 - bbox[1]), text, font=f, fill=fg)
    return img


def tree_round(rng):
    S = 4
    img = Image.new("RGBA", (128 * S, 128 * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rectangle([58 * S, 80 * S, 70 * S, 126 * S], fill=(105, 68, 32, 255))
    d.polygon([(52 * S, 126 * S), (76 * S, 126 * S), (70 * S, 100 * S), (58 * S, 100 * S)], fill=(95, 60, 28, 255))
    blobs = [(64, 50, 40, (52, 140, 60)), (40, 66, 28, (44, 124, 52)), (88, 66, 28, (48, 132, 56)), (64, 30, 26, (70, 165, 72)),
             (48, 40, 22, (62, 152, 66)), (82, 42, 22, (66, 158, 70))]
    for (cx, cy, r, col) in blobs:
        d.ellipse([(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S], fill=col + (255,))
    for _ in range(60):
        cx, cy = rng.uniform(30, 98), rng.uniform(16, 84)
        if (cx - 64) ** 2 / 40 ** 2 + (cy - 50) ** 2 / 36 ** 2 > 1:
            continue
        r = rng.uniform(3, 7)
        col = rng.choice([(84, 178, 80), (60, 150, 64), (96, 190, 90)])
        d.ellipse([(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S], fill=col + (255,))
    return img.resize((128, 128), Image.LANCZOS)


def tree_pine(rng):
    S = 4
    img = Image.new("RGBA", (128 * S, 128 * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rectangle([60 * S, 96 * S, 68 * S, 126 * S], fill=(92, 58, 28, 255))
    layers = [(126, 100, 44, (30, 108, 50)), (100, 72, 36, (36, 122, 56)), (76, 46, 28, (44, 138, 62)), (52, 22, 18, (54, 150, 70))]
    for (yb, yt, hw, col) in layers:
        d.polygon([(64 * S - hw * S, yb * S), (64 * S + hw * S, yb * S), (64 * S, yt * S)], fill=col + (255,))
        d.polygon([(64 * S - hw * S, yb * S), (64 * S, yb * S), (64 * S, yt * S)], fill=tuple(int(c * 0.85) for c in col) + (255,))
    return img.resize((128, 128), Image.LANCZOS)


def bush(rng, light):
    S = 4
    img = Image.new("RGBA", (64 * S, 64 * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    base = (96, 172, 66) if light else (46, 130, 54)
    for (cx, cy, r) in [(32, 40, 22), (18, 46, 14), (46, 46, 14), (32, 28, 14), (22, 34, 12), (42, 34, 12)]:
        col = tuple(max(0, min(255, c + rng.randint(-12, 12))) for c in base)
        d.ellipse([(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S], fill=col + (255,))
    return img.resize((64, 64), Image.LANCZOS)


def tread(rng):
    img = Image.new("RGB", (64, 64), (26, 26, 30))
    d = ImageDraw.Draw(img)
    for y in range(0, 64, 16):
        d.rectangle([0, y + 2, 63, y + 5], fill=(14, 14, 18))
        d.rectangle([0, y + 10, 63, y + 12], fill=(38, 38, 44))
    for x in (14, 32, 50):
        d.rectangle([x, 0, x + 2, 63], fill=(16, 16, 20))
    return img


def gate():
    """448x40: szachownica z czerwonym polem KART na srodku (baner bramy 92 x 8 jednostek)."""
    img = Image.new("RGB", (448, 40), (24, 24, 30))
    d = ImageDraw.Draw(img)
    for y in range(0, 40, 10):
        for x in range(0, 448, 10):
            if ((x // 10) + (y // 10)) % 2 == 0:
                d.rectangle([x, y, x + 9, y + 9], fill=(244, 244, 244))
    d.rectangle([164, 4, 283, 35], fill=(236, 52, 52))
    f = font(28)
    bbox = d.textbbox((0, 0), "KART", font=f)
    d.text((224 - (bbox[2] - bbox[0]) / 2 - bbox[0], 20 - (bbox[3] - bbox[1]) / 2 - bbox[1]), "KART", font=f, fill=(255, 255, 255))
    return img


def livery(color, number):
    """64 (w poprzek bolidu) x 128 (wzdluz: gora = przod). Pasy wzdluz, numer w kolku z przodu, sponsorzy z tylu."""
    img = Image.new("RGB", (64, 128), color)
    d = ImageDraw.Draw(img)
    dark = tuple(int(c * 0.55) for c in color)
    d.rectangle([26, 0, 30, 127], fill=(245, 245, 245))
    d.rectangle([33, 0, 37, 127], fill=(245, 245, 245))
    d.rectangle([0, 0, 2, 127], fill=dark)
    d.rectangle([61, 0, 63, 127], fill=dark)
    d.ellipse([14, 22, 50, 58], fill=(245, 245, 245))
    f = font(28)
    bbox = d.textbbox((0, 0), str(number), font=f)
    d.text((32 - (bbox[2] - bbox[0]) / 2 - bbox[0], 40 - (bbox[3] - bbox[1]) / 2 - bbox[1]), str(number), font=f, fill=(20, 20, 26))
    for i, c in enumerate([(250, 210, 60), (60, 200, 220), (240, 240, 240)]):
        d.rectangle([8, 84 + i * 13, 16, 92 + i * 13], fill=c)
        d.rectangle([48, 84 + i * 13, 56, 92 + i * 13], fill=c)
    return img


def main():
    rng = random.Random(4242)
    atlas = Image.new("RGBA", (W, H), (0, 0, 0, 0))

    def put(img, x, y):
        if img.mode != "RGBA":
            img = img.convert("RGBA")
        atlas.alpha_composite(img, (x, y))

    put(asphalt(rng), 0, 0)
    put(grass(rng, (74, 150, 60), (104, 184, 78), 18, blades=700), 128, 0)
    put(grass(rng, (104, 158, 64), (134, 182, 84), 6, blades=400, blur_end=1.1), 256, 0)
    put(crowd(rng), 384, 0)
    put(banner("KART", (240, 110, 30), (255, 255, 255)), 384, 32)
    put(banner("TURBO", (40, 150, 220), (255, 255, 255)), 384, 64)
    put(banner("LAKE", (150, 60, 200), (255, 240, 120)), 384, 96)
    put(tree_round(rng), 0, 128)
    put(tree_pine(rng), 128, 128)
    put(bush(rng, False), 256, 128)
    put(bush(rng, True), 320, 128)
    put(tread(rng), 256, 192)
    put(gate(), 0, 384)
    for i, col in enumerate(KART_COLORS):
        put(livery(col, i + 1), i * 64, 256)

    # paleta: 255 kolorow + indeks 0 = przezroczysty (magenta)
    alpha = atlas.split()[3]
    rgb = atlas.convert("RGB")
    q = rgb.quantize(255, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = q.getpalette()[: 255 * 3]
    out = Image.new("P", (W, H))
    out.putpalette([255, 0, 255] + pal)
    src = q.load()
    dst = out.load()
    al = alpha.load()
    for y in range(H):
        for x in range(W):
            dst[x, y] = 0 if al[x, y] < 128 else src[x, y] + 1
    OUT.parent.mkdir(parents=True, exist_ok=True)
    out.save(OUT, optimize=True)
    print(f"{OUT.relative_to(ROOT)}  {W}x{H}  paleta 256 (0 = przezroczysty)")


if __name__ == "__main__":
    main()
