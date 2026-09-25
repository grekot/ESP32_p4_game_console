"""Okladki gier do menu konsoli: surowe obrazy z Gemini (assets_src/covers/*.jpg) -> assets/covers/<id>.png 440x248.

440x248 to natywny rozmiar zaznaczonej karty w karuzeli (ui/menu.cpp: COVER_W x COVER_H) - menu wyswietla PNG bez
skalowania, a boczne karty i rozmyte tlo liczy z tego samego obrazu. Skrypt przycina srodek do proporcji karty,
skaluje (LANCZOS) i lekko wyostrza. Snake bierze ilustracje tytulowa (assets_src/snake/title.jpg).
Deterministyczny. Wymaga Pillow.

    python tools/gen_covers.py
"""
import os

from PIL import Image, ImageEnhance, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "assets_src")
DST = os.path.join(ROOT, "assets", "covers")
W, H = 440, 248

# id gry (engine::GAMES) -> zrodlo (wzgledem assets_src/), przesuniecie kadru w pionie (-1 gora .. 1 dol)
COVERS = {
    "mario": ("covers/mario.jpg", 0.0),
    "labirynt3d": ("covers/labirynt3d.jpg", 0.0),
    "kosmos": ("covers/kosmos.jpg", 0.0),
    "kart": ("covers/kart.jpg", 0.0),
    "snake": ("snake/title.jpg", 0.2),
}


def cover(src, shift):
    im = Image.open(src).convert("RGB")
    w, h = im.size
    target = W / H
    if w / h > target:
        nw = int(h * target)
        x0 = (w - nw) // 2
        im = im.crop((x0, 0, x0 + nw, h))
    else:
        nh = int(w / target)
        y0 = int((h - nh) / 2 * (1 + shift))
        im = im.crop((0, y0, w, y0 + nh))
    im = im.resize((W, H), Image.LANCZOS)
    im = im.filter(ImageFilter.UnsharpMask(radius=1.2, percent=60, threshold=2))
    return ImageEnhance.Color(im).enhance(1.05)


def main():
    os.makedirs(DST, exist_ok=True)
    for gid, (rel, shift) in COVERS.items():
        path = os.path.join(SRC, rel)
        if not os.path.exists(path):
            print("  BRAK  %s (%s) - menu narysuje okladke zastepcza" % (gid, rel))
            continue
        cover(path, shift).save(os.path.join(DST, gid + ".png"), optimize=True)
        print("  %-12s %dx%d  <- %s" % (gid + ".png", W, H, rel))


if __name__ == "__main__":
    main()
