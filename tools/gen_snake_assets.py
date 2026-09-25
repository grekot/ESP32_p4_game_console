"""Grafika gry Snake: z surowych obrazow Gemini (assets_src/snake/) do PNG z alfa w assets/snake/.

Surowe obrazy wygenerowal Gemini (konto uzytkownika, 25.09.2026): arkusze 4x3 obiektow na tle magenta
(#FF00FF), tla swiatow i ekran tytulowy. Skrypt:
  - wycina obiekty z arkuszy (siatka 4x3, ramka obiektu = piksele nie-magenta w komorce),
  - usuwa tlo magenta z miekka krawedzia (alfa z "magentowosci" piksela) i zdejmuje rozowa poswiate z brzegow,
  - skaluje do rozmiarow gry (LANCZOS) i zapisuje RGBA,
  - tla przycina/skaluje do pola gry 800x448 i przyciemnia lekko, zeby waz byl czytelny,
  - ekran tytulowy skaluje do 800x480.
Deterministyczny: te same wejscia -> te same pliki. Wymaga Pillow.

    python tools/gen_snake_assets.py
"""
import os
import sys

from PIL import Image, ImageEnhance, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "assets_src", "snake")
DST = os.path.join(ROOT, "assets", "snake")

FIELD_W, FIELD_H = 800, 448


def magentaness(r, g, b):
    """Jak bardzo piksel przypomina tlo (255, 0, 255): R i B wysoko ponad G, R ~ B."""
    if abs(r - b) > 90:
        return 0
    return min(r, b) - g


def key_magenta(im, holes=False):
    """RGB -> RGBA: tlo magenta przezroczyste, krawedzie z czesciowa alfa i bez rozowej poswiaty.

    Tlem jest tylko obszar magenty POLACZONY z brzegiem komorki (flood fill) - fioletowy grzyb czy portal
    w srodku obiektu zostaja nietkniete. Miekka alfa i zdejmowanie poswiaty tylko w pasie 3 px przy tle.
    holes=True: dodatkowo kazdy wyraznie magentowy piksel (szczeliny miedzy galeziami drzew) jest tlem - dla obiektow
    bez fioletu (drzewa Karta); rozowe kwiaty maja za mala "magentowosc", zeby zniknac.
    """
    im = im.convert("RGB")
    w, h = im.size
    src = im.load()
    bg = bytearray(w * h)
    stack = [(x, 0) for x in range(w)] + [(x, h - 1) for x in range(w)] + \
            [(0, y) for y in range(h)] + [(w - 1, y) for y in range(h)]
    if holes:
        for y in range(h):
            for x in range(w):
                if magentaness(*src[x, y]) >= 110:
                    stack.append((x, y))
    while stack:
        x, y = stack.pop()
        i = y * w + x
        if bg[i]:
            continue
        if magentaness(*src[x, y]) < 70:
            continue
        bg[i] = 1
        if x > 0: stack.append((x - 1, y))
        if x < w - 1: stack.append((x + 1, y))
        if y > 0: stack.append((x, y - 1))
        if y < h - 1: stack.append((x, y + 1))
    # odleglosc od tla (0 = tlo, 1..3 = pas krawedzi, 4 = wnetrze)
    dist = [0 if v else 4 for v in bg]
    for step in (1, 2, 3):
        prev = dist[:]
        for y in range(h):
            for x in range(w):
                i = y * w + x
                if prev[i] != 4:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < w and 0 <= ny < h and prev[ny * w + nx] == step - 1:
                        dist[i] = step
                        break
    out = Image.new("RGBA", (w, h))
    dst = out.load()
    for y in range(h):
        for x in range(w):
            r, g, b = src[x, y]
            d = dist[y * w + x]
            if d == 0:
                dst[x, y] = (0, 0, 0, 0)
                continue
            a = 255
            if d < 4:
                m = magentaness(r, g, b)
                if m > 30:
                    a = max(0, min(255, int(255 * (130 - m) / 100)))
                    k = m * 0.9
                    r = max(0, int(r - k))
                    b = max(0, int(b - k))
                    g = min(255, int(g + k * 0.1))
            dst[x, y] = (r, g, b, a)
    return out


def erode_alpha(im, px=1):
    """Zjada px pikseli z krawedzi alfa (resztki rozowej obwodki po JPEG)."""
    a = im.getchannel("A").filter(ImageFilter.MinFilter(2 * px + 1))
    im = im.copy()
    im.putalpha(a)
    return im


def cells(sheet, cols=4, rows=3):
    w, h = sheet.size
    cw, ch = w / cols, h / rows
    for r in range(rows):
        for c in range(cols):
            # margines 3% - odcina linie siatki miedzy komorkami
            mx, my = cw * 0.03, ch * 0.03
            box = (int(c * cw + mx), int(r * ch + my), int((c + 1) * cw - mx), int((r + 1) * ch - my))
            yield r, c, sheet.crop(box)


def fit(obj, size, pad=0.04):
    """Przycina do ramki alfa, dopelnia do kwadratu (srodek) i skaluje do size x size."""
    bbox = obj.getchannel("A").point(lambda v: 255 if v > 40 else 0).getbbox()
    if bbox:
        obj = obj.crop(bbox)
    w, h = obj.size
    side = int(max(w, h) * (1 + 2 * pad))
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    sq.paste(obj, ((side - w) // 2, (side - h) // 2))
    return sq.resize((size, size), Image.LANCZOS)


def premultiply_clean(im):
    """Piksele o alfa 0 -> czarne (mniejsze PNG, bez smieci w kolorze pod przezroczystoscia)."""
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                px[x, y] = (0, 0, 0, 0)
    return im


def save(im, name):
    path = os.path.join(DST, name)
    premultiply_clean(im).save(path, optimize=True)
    print("  %-28s %dx%d" % (name, im.size[0], im.size[1]))


def process_sheet(fname, names):
    path = os.path.join(SRC, fname)
    if not os.path.exists(path):
        print("BRAK", path)
        return
    sheet = Image.open(path).convert("RGB")
    # pracujemy na polowie rozdzielczosci (arkusz 2816x1536 -> 1408x768): szybciej, obiekty i tak ida do <= 64 px
    sheet = sheet.resize((sheet.size[0] // 2, sheet.size[1] // 2), Image.LANCZOS)
    for r, c, cell in cells(sheet):
        spec = names.get((r, c))
        if not spec:
            continue
        obj = erode_alpha(key_magenta(cell), 1)
        for name, size in spec:
            save(fit(obj, size), name)


def process_background(fname, name, darken=0.88, sat=0.95):
    path = os.path.join(SRC, fname)
    if not os.path.exists(path):
        print("BRAK", path)
        return
    im = Image.open(path).convert("RGB")
    # przyciecie do proporcji pola 800x448 (srodek), potem skala
    w, h = im.size
    target = FIELD_W / FIELD_H
    if w / h > target:
        nw = int(h * target)
        im = im.crop(((w - nw) // 2, 0, (w - nw) // 2 + nw, h))
    else:
        nh = int(w / target)
        im = im.crop((0, (h - nh) // 2, w, (h - nh) // 2 + nh))
    im = im.resize((FIELD_W, FIELD_H), Image.LANCZOS)
    im = ImageEnhance.Brightness(im).enhance(darken)
    im = ImageEnhance.Color(im).enhance(sat)
    im.save(os.path.join(DST, name), optimize=True)
    print("  %-28s %dx%d" % (name, FIELD_W, FIELD_H))


def process_title(fname, name):
    path = os.path.join(SRC, fname)
    if not os.path.exists(path):
        print("BRAK", path)
        return
    im = Image.open(path).convert("RGB")
    w, h = im.size
    target = 800 / 480
    if w / h > target:
        nw = int(h * target)
        im = im.crop(((w - nw) // 2, 0, (w - nw) // 2 + nw, h))
    else:
        nh = int(w / target)
        im = im.crop((0, (h - nh) // 2, w, (h - nh) // 2 + nh))
    im = im.resize((800, 480), Image.LANCZOS)
    im.save(os.path.join(DST, name), optimize=True)
    print("  %-28s 800x480" % name)


# (wiersz, kolumna) -> lista (plik, rozmiar). Przedmioty 40 px (rysowane na kafelku 32 z lekkim wystawaniem),
# przeszkody 36 px, ikony HUD 24 px.
ITEMS = {
    (0, 0): [("apple.png", 40), ("icon_apple.png", 24)],
    (0, 1): [("golden.png", 40), ("icon_golden.png", 24)],
    (0, 2): [("mushroom.png", 40), ("icon_mushroom.png", 24)],
    (0, 3): [("hourglass.png", 40), ("icon_hourglass.png", 24)],
    (1, 0): [("star.png", 40), ("icon_star.png", 24)],
    (1, 1): [("heart.png", 40), ("icon_heart.png", 24)],
    (1, 2): [("magnet.png", 40), ("icon_magnet.png", 24)],
    (1, 3): [("gem.png", 40), ("icon_gem.png", 24)],
    (2, 0): [("bomb.png", 40), ("icon_bomb.png", 24)],
    (2, 1): [("rock.png", 36)],
    (2, 2): [("bush.png", 36)],
    (2, 3): [("stump.png", 36)],
}

ITEMS2 = {
    (0, 0): [("cactus.png", 36)],
    (0, 1): [("sandstone.png", 36)],
    (0, 2): [("snowrock.png", 36)],
    (0, 3): [("ice.png", 36)],
    (1, 0): [("lavarock.png", 36)],
    (1, 1): [("crate.png", 36)],
    (1, 2): [("portal.png", 48)],
    (1, 3): [("orb.png", 40), ("icon_orb.png", 24)],
    (2, 0): [("head_green.png", 40)],
    (2, 1): [("head_blue.png", 40)],
    (2, 2): [("head_orange.png", 40)],
}

# (zrodlo, wynik, jasnosc, nasycenie) - korekta per swiat: waz i przedmioty musza odcinac sie od tla
WORLDS = [
    ("bg_meadow.jpg", "bg_meadow.png", 0.90, 0.95),
    ("bg_desert.jpg", "bg_desert.png", 0.92, 1.00),
    ("bg_snow.jpg", "bg_snow.png", 0.90, 1.00),
    ("bg_jungle.jpg", "bg_jungle.png", 1.35, 0.90),
    ("bg_volcano.jpg", "bg_volcano.png", 1.00, 1.00),
]


def main():
    os.makedirs(DST, exist_ok=True)
    only = sys.argv[1:]
    if not only or "items" in only:
        print("items_sheet.jpg")
        process_sheet("items_sheet.jpg", ITEMS)
    if not only or "items2" in only:
        print("items2_sheet.jpg")
        process_sheet("items2_sheet.jpg", ITEMS2)
    if not only or "bg" in only:
        print("tla")
        for src, dst, bright, sat in WORLDS:
            process_background(src, dst, bright, sat)
    if not only or "title" in only:
        print("tytul")
        process_title("title.jpg", "title.png")


if __name__ == "__main__":
    main()
