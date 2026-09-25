"""Grafika gry Pacman: z surowych obrazow Gemini (assets_src/pacman/) do PNG z alfa w assets/pacman/.

Surowe obrazy wygenerowal Gemini (konto uzytkownika, 25.09.2026), wszystkie 2816x1536 (tytul 2752x1536):
  hero_sheet.jpg   4x2: gorny wiersz 4 klatki paszczy (zamknieta -> szeroko otwarta), dolny 4 klatki smierci
  ghosts_sheet.jpg 4x3: czerwony, rozowy, blekitny, pomaranczowy (klatka 1); ten sam rzad klatka 2 (falbanka);
                        przestraszony granatowy, przestraszony bialy, same oczy, pusto
  fruits_sheet.jpg 4x2: wisnie, truskawka, pomarancza, jablko; arbuz, dzwonek, klucz, gwiazda
  bg_sheet.jpg     2x2 tla swiatow (magentowe linie podzialu): neon, cukierki, dzungla, lawa
  title.jpg        ilustracja tytulowa 16:9
Skrypt: wycina komorki, usuwa magente z miekka krawedzia (jak gen_snake_assets.py), skaluje i zapisuje RGBA.
Klatki paszczy bohatera sa wyrownywane do LEWEJ krawedzi i srodka w pionie ze wspolna skala (paszcza otwiera sie
z prawej - centrowanie po ramce alfa przesuwaloby kule miedzy klatkami). Duchy: wspolna skala z pierwszego ducha,
zeby oczy i przestraszone duchy nie byly powiekszane. Deterministyczny. Wymaga Pillow.

    python tools/gen_pacman_assets.py [hero|ghosts|fruits|bg|title]
"""
import os
import sys

from PIL import Image, ImageEnhance, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "assets_src", "pacman")
DST = os.path.join(ROOT, "assets", "pacman")

SPR = 40       # bohater i duchy (kratka 24 px - postacie sa wieksze od korytarza, jak w oryginale)
FRUIT = 32
ICON = 24
MAZE_W, MAZE_H = 648, 456


def magentaness(r, g, b):
    if abs(r - b) > 90:
        return 0
    return min(r, b) - g


def key_magenta(im, thr=70, edge=30, holes=False):
    """RGB -> RGBA: tlo magenta (polaczone z brzegiem) przezroczyste, krawedz z czesciowa alfa bez rozowej poswiaty.

    thr: prog "magentowosci" tla (rozowy duch wymaga wyzszego, 150 - jego cienie maja ok. 90 i znikaly),
    edge: od jakiej magentowosci piksel krawedzi jest wygaszany/odbarwiany,
    holes: takze zamknieta magenta (dziura miedzy ogonkami wisni, ucho klucza) jest tlem.
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
                if magentaness(*src[x, y]) >= max(thr, 110):
                    stack.append((x, y))
    while stack:
        x, y = stack.pop()
        i = y * w + x
        if bg[i]:
            continue
        if magentaness(*src[x, y]) < thr:
            continue
        bg[i] = 1
        if x > 0: stack.append((x - 1, y))
        if x < w - 1: stack.append((x + 1, y))
        if y > 0: stack.append((x, y - 1))
        if y < h - 1: stack.append((x, y + 1))
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
                if m > edge:
                    a = max(0, min(255, int(255 * (edge + 100 - m) / 100)))
                    k = m * 0.9
                    r = max(0, int(r - k))
                    b = max(0, int(b - k))
                    g = min(255, int(g + k * 0.1))
            dst[x, y] = (r, g, b, a)
    return out


def erode_alpha(im, px=1):
    a = im.getchannel("A").filter(ImageFilter.MinFilter(2 * px + 1))
    im = im.copy()
    im.putalpha(a)
    return im


def cells(sheet, cols, rows, margin=0.03):
    w, h = sheet.size
    cw, ch = w / cols, h / rows
    for r in range(rows):
        for c in range(cols):
            mx, my = cw * margin, ch * margin
            box = (int(c * cw + mx), int(r * ch + my), int((c + 1) * cw - mx), int((r + 1) * ch - my))
            yield r, c, sheet.crop(box)


def bbox_of(obj):
    return obj.getchannel("A").point(lambda v: 255 if v > 40 else 0).getbbox()


def fit_center(obj, size, scale=None, pad=0.04):
    """Ramka alfa wysrodkowana w kwadracie. scale = piksele zrodla na piksel wyniku (None = dopasuj do size)."""
    bbox = bbox_of(obj)
    if not bbox:
        return Image.new("RGBA", (size, size), (0, 0, 0, 0))
    obj = obj.crop(bbox)
    w, h = obj.size
    if scale is None:
        side = int(max(w, h) * (1 + 2 * pad))
    else:
        side = int(size * scale)
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    sq.paste(obj, ((side - w) // 2, (side - h) // 2))
    return sq.resize((size, size), Image.LANCZOS)


def fit_left(obj, size, scale, pad=0.04):
    """Klatki paszczy: wyrownanie do lewej krawedzi i srodka w pionie (kula ma stala srednice = wysokosc ramki)."""
    bbox = bbox_of(obj)
    obj = obj.crop(bbox)
    w, h = obj.size
    side = int(size * scale)
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    left = int(side * pad)
    sq.paste(obj, (left, (side - h) // 2))
    return sq.resize((size, size), Image.LANCZOS)


def premultiply_clean(im):
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            if px[x, y][3] == 0:
                px[x, y] = (0, 0, 0, 0)
    return im


def save(im, name):
    path = os.path.join(DST, name)
    premultiply_clean(im).save(path, optimize=True)
    print("  %-22s %dx%d" % (name, im.size[0], im.size[1]))


def load_sheet(fname):
    path = os.path.join(SRC, fname)
    if not os.path.exists(path):
        print("BRAK", path)
        return None
    sheet = Image.open(path).convert("RGB")
    return sheet.resize((sheet.size[0] // 2, sheet.size[1] // 2), Image.LANCZOS)


def process_hero():
    sheet = load_sheet("hero_sheet.jpg")
    if sheet is None:
        return
    objs = {}
    for r, c, cell in cells(sheet, 4, 2):
        objs[(r, c)] = erode_alpha(key_magenta(cell), 1)
    # wspolna skala: wysokosc ramki zamknietej paszczy (kula) z zapasem 8 %
    b0 = bbox_of(objs[(0, 0)])
    diameter = b0[3] - b0[1]
    scale = diameter * 1.08 / SPR
    for c in range(4):
        save(fit_left(objs[(0, c)], SPR, scale), "hero%d.png" % c)
    save(fit_left(objs[(0, 1)], ICON, diameter * 1.08 / ICON), "icon_hero.png")   # skala liczona dla 24 px, nie 40
    for c in range(4):
        save(fit_center(objs[(1, c)], SPR, scale), "die%d.png" % c)


def process_ghosts():
    sheet = load_sheet("ghosts_sheet.jpg")
    if sheet is None:
        return
    objs = {}
    for r, c, cell in cells(sheet, 4, 3):
        pink = r < 2 and c == 1
        objs[(r, c)] = erode_alpha(key_magenta(cell, thr=150 if pink else 70, edge=120 if pink else 30), 1)
    b0 = bbox_of(objs[(0, 0)])
    scale = max(b0[2] - b0[0], b0[3] - b0[1]) * 1.06 / SPR
    names = ["red", "pink", "cyan", "orange"]
    for c in range(4):
        save(fit_center(objs[(0, c)], SPR, scale), "ghost_%s0.png" % names[c])
        save(fit_center(objs[(1, c)], SPR, scale), "ghost_%s1.png" % names[c])
    save(fit_center(objs[(2, 0)], SPR, scale), "scared0.png")
    save(fit_center(objs[(2, 1)], SPR, scale), "scared1.png")
    save(fit_center(objs[(2, 2)], SPR, scale), "eyes.png")


FRUITS = ["cherry", "strawberry", "orange", "apple", "melon", "bell", "key", "star"]


def process_fruits():
    sheet = load_sheet("fruits_sheet.jpg")
    if sheet is None:
        return
    for r, c, cell in cells(sheet, 4, 2):
        obj = erode_alpha(key_magenta(cell, holes=True), 1)
        name = FRUITS[r * 4 + c]
        save(fit_center(obj, FRUIT), "%s.png" % name)
        save(fit_center(obj, ICON), "icon_%s.png" % name)


WORLDS = ["neon", "candy", "jungle", "lava"]


def process_bg():
    path = os.path.join(SRC, "bg_sheet.jpg")
    if not os.path.exists(path):
        print("BRAK", path)
        return
    sheet = Image.open(path).convert("RGB")
    w, h = sheet.size
    for i, name in enumerate(WORLDS):
        c, r = i % 2, i // 2
        # margines 2 % odcina magentowe linie podzialu
        box = (int(c * w / 2 + w * 0.02), int(r * h / 2 + h * 0.02), int((c + 1) * w / 2 - w * 0.02), int((r + 1) * h / 2 - h * 0.02))
        im = sheet.crop(box)
        cw, ch = im.size
        target = MAZE_W / MAZE_H
        if cw / ch > target:
            nw = int(ch * target)
            im = im.crop(((cw - nw) // 2, 0, (cw - nw) // 2 + nw, ch))
        else:
            nh = int(cw / target)
            im = im.crop((0, (ch - nh) // 2, cw, (ch - nh) // 2 + nh))
        im = im.resize((MAZE_W, MAZE_H), Image.LANCZOS)
        im = ImageEnhance.Brightness(im).enhance(0.55)   # tlo pod swiecacym labiryntem musi byc ciemne
        im = ImageEnhance.Color(im).enhance(0.9)
        im.save(os.path.join(DST, "bg_%s.png" % name), optimize=True)
        print("  %-22s %dx%d" % ("bg_%s.png" % name, MAZE_W, MAZE_H))


def process_title():
    path = os.path.join(SRC, "title.jpg")
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
    im.save(os.path.join(DST, "title.png"), optimize=True)
    print("  %-22s 800x480" % "title.png")


def main():
    os.makedirs(DST, exist_ok=True)
    only = sys.argv[1:]
    if not only or "hero" in only:
        process_hero()
    if not only or "ghosts" in only:
        process_ghosts()
    if not only or "fruits" in only:
        process_fruits()
    if not only or "bg" in only:
        process_bg()
    if not only or "title" in only:
        process_title()


if __name__ == "__main__":
    main()
