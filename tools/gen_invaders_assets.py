"""Grafika gry Space Invaders: z surowych obrazow Gemini (assets_src/invaders/) do PNG w assets/invaders/.

Surowe obrazy wygenerowal Gemini (konto uzytkownika, 25.09.2026), wszystkie 2752x1536:
  aliens_sheet.jpg 4x3: kalmar, krab, osmiornica, meduza-robot (klatka A); te same (klatka B);
                        UFO, boss, boss uszkodzony, kapsula zycia
  items_sheet.jpg  4x3: statek gracza, bunkier (krysztalowy luk), pocisk obcych, laser gracza;
                        bonusy: potrojny strzal, laser, oslona, spowolnienie; 4 klatki wybuchu
  bg_planet.jpg, bg_rings.jpg, bg_asteroids.jpg, bg_blackhole.jpg - tla swiatow 16:9
  title.jpg        ilustracja tytulowa 16:9 (bez napisow, logo rysuje gra)
Magenta wycinana tak jak w gen_pacman_assets.py (te same funkcje). Klatki A/B jednego obcego maja wspolna skale
(liczona z wiekszej ramki), zeby animacja nie "pompowala". Wybuchy - wspolna skala z najwiekszej klatki (kula ognia),
wiec iskra na poczatku jest mala. Tla przyciemnione (sprite'y musza sie odcinac), srodek dodatkowo winietowany
od gory (tam stoi formacja). Deterministyczny. Wymaga Pillow.

    python tools/gen_invaders_assets.py [aliens|items|bg|title]
"""
import os
import sys

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_pacman_assets import bbox_of, cells, erode_alpha, fit_center, key_magenta, premultiply_clean  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "assets_src", "invaders")
DST = os.path.join(ROOT, "assets", "invaders")

ALIEN = 44          # obcy w formacji (kratka 60x48 w grze)
UFO_W, UFO_H = 80, 44
BOSS = 160
LIFE = 32
SHIP = 60
BUNKER_W, BUNKER_H = 112, 56
BOMB_W, BOMB_H = 22, 34
BOLT_W, BOLT_H = 12, 40
POWER = 36
BOOM = 72
ICON = 24
W, H = 800, 480

ALIENS = ["squid", "crab", "octo", "jelly"]
POWERS = ["triple", "laser", "shield", "slow"]
# tlo -> (jasnosc, sila winiety srodka)
BGS = {"planet": (0.85, 0.35), "rings": (0.7, 0.45), "asteroids": (0.6, 0.55), "blackhole": (0.9, 0.3)}


def fit_rect(obj, w, h, scale=None, pad=0.04):
    """Ramka alfa wysrodkowana w prostokacie w x h. scale = piksele zrodla na piksel wyniku (None = dopasuj)."""
    bbox = bbox_of(obj)
    if not bbox:
        return Image.new("RGBA", (w, h), (0, 0, 0, 0))
    obj = obj.crop(bbox)
    ow, oh = obj.size
    if scale is None:
        scale = max(ow / (w * (1 - 2 * pad)), oh / (h * (1 - 2 * pad)))
    bw, bh = int(w * scale), int(h * scale)
    box = Image.new("RGBA", (bw, bh), (0, 0, 0, 0))
    box.paste(obj, ((bw - ow) // 2, (bh - oh) // 2))
    return box.resize((w, h), Image.LANCZOS)


def fit_scale(objs, w, h, pad=0.04):
    """Wspolna skala, przy ktorej najwieksza z ramek miesci sie w w x h."""
    s = 0
    for o in objs:
        b = bbox_of(o)
        if b:
            s = max(s, (b[2] - b[0]) / (w * (1 - 2 * pad)), (b[3] - b[1]) / (h * (1 - 2 * pad)))
    return s


def save(im, name):
    premultiply_clean(im).save(os.path.join(DST, name), optimize=True)
    print("  %-22s %dx%d" % (name, im.size[0], im.size[1]))


def load_sheet(fname):
    path = os.path.join(SRC, fname)
    if not os.path.exists(path):
        print("BRAK", path)
        return None
    sheet = Image.open(path).convert("RGB")
    return sheet.resize((sheet.size[0] // 2, sheet.size[1] // 2), Image.LANCZOS)


def sheet_objs(fname, holes=False):
    sheet = load_sheet(fname)
    if sheet is None:
        return None
    return {(r, c): erode_alpha(key_magenta(cell, holes=holes), 1) for r, c, cell in cells(sheet, 4, 3)}


def process_aliens():
    objs = sheet_objs("aliens_sheet.jpg")
    if objs is None:
        return
    for c, name in enumerate(ALIENS):
        pair = [objs[(0, c)], objs[(1, c)]]
        s = fit_scale(pair, ALIEN, ALIEN)
        for f in range(2):
            save(fit_rect(pair[f], ALIEN, ALIEN, s), "%s%d.png" % (name, f))
    save(fit_rect(objs[(2, 0)], UFO_W, UFO_H), "ufo.png")
    pair = [objs[(2, 1)], objs[(2, 2)]]
    s = fit_scale(pair, BOSS, BOSS)
    save(fit_rect(pair[0], BOSS, BOSS, s), "boss0.png")
    save(fit_rect(pair[1], BOSS, BOSS, s), "boss1.png")
    save(fit_rect(objs[(2, 3)], LIFE, LIFE), "life.png")


def process_items():
    objs = sheet_objs("items_sheet.jpg")
    if objs is None:
        return
    save(fit_rect(objs[(0, 0)], SHIP, SHIP), "ship.png")
    save(fit_center(objs[(0, 0)], ICON), "icon_ship.png")
    save(fit_rect(objs[(0, 1)], BUNKER_W, BUNKER_H, pad=0.0), "bunker.png")
    save(fit_rect(objs[(0, 2)], BOMB_W, BOMB_H), "bomb.png")
    save(fit_rect(objs[(0, 3)], BOLT_W, BOLT_H, pad=0.0), "bolt.png")
    for c, name in enumerate(POWERS):
        save(fit_center(objs[(1, c)], POWER), "pw_%s.png" % name)
    booms = [objs[(2, c)] for c in range(4)]
    s = fit_scale(booms, BOOM, BOOM, pad=0.02)
    for c in range(4):
        save(fit_rect(booms[c], BOOM, BOOM, s), "boom%d.png" % c)


def crop_16_10(im):
    w, h = im.size
    target = W / H
    if w / h > target:
        nw = int(h * target)
        return im.crop(((w - nw) // 2, 0, (w - nw) // 2 + nw, h))
    nh = int(w / target)
    return im.crop((0, (h - nh) // 2, w, (h - nh) // 2 + nh))


def process_bg():
    for name, (bright, vig) in BGS.items():
        path = os.path.join(SRC, "bg_%s.jpg" % name)
        if not os.path.exists(path):
            print("BRAK", path)
            continue
        im = crop_16_10(Image.open(path).convert("RGB")).resize((W, H), Image.LANCZOS)
        im = ImageEnhance.Brightness(im).enhance(bright)
        # winieta: ciemna elipsa w gornej i srodkowej czesci (formacja obcych), brzegi i dol bez zmian
        mask = Image.new("L", (W, H), 0)
        ImageDraw.Draw(mask).ellipse((W * 0.12, -H * 0.25, W * 0.88, H * 0.78), fill=int(255 * vig))
        mask = mask.filter(ImageFilter.GaussianBlur(70))
        im = Image.composite(Image.new("RGB", (W, H), (4, 5, 14)), im, mask)
        im.save(os.path.join(DST, "bg_%s.png" % name), optimize=True)
        print("  %-22s %dx%d" % ("bg_%s.png" % name, W, H))


def process_title():
    path = os.path.join(SRC, "title.jpg")
    if not os.path.exists(path):
        print("BRAK", path)
        return
    im = crop_16_10(Image.open(path).convert("RGB")).resize((W, H), Image.LANCZOS)
    im.save(os.path.join(DST, "title.png"), optimize=True)
    print("  %-22s %dx%d" % ("title.png", W, H))


def main():
    os.makedirs(DST, exist_ok=True)
    only = sys.argv[1:]
    if not only or "aliens" in only:
        process_aliens()
    if not only or "items" in only:
        process_items()
    if not only or "bg" in only:
        process_bg()
    if not only or "title" in only:
        process_title()


if __name__ == "__main__":
    main()
