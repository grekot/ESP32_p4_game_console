"""Grafika Karta z Gemini (assets_src/kart/) - tekstury, drzewa, panoramy gor dla kazdego motywu toru, chmury, ikony.

Motywy (kolejnosc i kolory mgly jak THEMES w src/games/kart/kart_tracks.h):
    jezioro  laka nad jeziorem    kanion  pustynny kanion    zima  zimowa przelecz    jesien  jesienny las

Surowe obrazy z Gemini (konto uzytkownika, 25.09.2026) w assets_src/kart/:
    asphalt.jpg               asfalt z gory (wspolny)            crowd.jpg      publicznosc na trybunie (wspolna)
    ground_<motyw>.jpg        nawierzchnia z gory: trawa / piasek / snieg / trawa z liscmi
    trees_sheet[_<motyw>].jpg arkusz 4x2 na magencie (drzewa i krzaki, uklad w TREE_SLOTS)
    mountains_a/_b.jpg, mountains_<motyw>.jpg   panoramy, niebo magenta
    clouds.jpg                pas chmur (wspolny)                items_sheet.jpg  ikony (gorny rzad 3x2)

Wynik:
    assets/kart/mountains_<motyw>.png  2048x160 RGBA - petla 360 stopni z sumowaniem sylwetek na szwach, dol w mgle
    assets/kart/clouds.png, banana.png, shell.png, mushroom.png
    kafelki atlasu: atlas_tiles(motyw) - uzywa tools/gen_kart_atlas.py (assets/kart/atlas_<motyw>.png)

Tekstury nawierzchni: srodek obrazu -> bezszwowe (przenikanie z kopia przesunieta o pol kafelka), 128 px; wersja
"daleka" rozmyta i o mniejszym kontrascie (bez mipmap drobny detal migocze w oddali).

Kolejnosc: python tools/gen_kart_assets.py -> python tools/gen_kart_gemini.py -> python tools/gen_kart_atlas.py.
Brak pliku zrodlowego = zostaje wersja rysowana. Deterministyczny. Wymaga Pillow.
"""
import os
import sys

from PIL import Image, ImageChops, ImageEnhance, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "assets_src", "kart")
DST = os.path.join(ROOT, "assets", "kart")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_snake_assets import cells, erode_alpha, key_magenta, premultiply_clean  # noqa: E402

# motyw -> kolor mgly (FOG w kart_tracks.h), nawierzchnia, arkusz drzew, uklad arkusza, panoramy
# tree_slots: (wiersz, kolumna) arkusza 4x2 dla kafelkow tree0..tree3 (atlas: dab/sosna/brzoza/klon na lace) i bush0..1
THEMES = {
    "jezioro": {"fog": (206, 222, 242), "ground": "ground_jezioro.jpg", "trees": "trees_sheet.jpg",
                "slots": {"tree0": (0, 0), "tree1": (0, 1), "tree2": (0, 3), "tree3": (1, 2), "bush0": (1, 0), "bush1": (1, 1)},
                "mountains": ["mountains_a.jpg", "mountains_b.jpg"]},
    "kanion": {"fog": (238, 212, 176), "ground": "ground_kanion.jpg", "trees": "trees_sheet_kanion.jpg", "color": 0.9, "bright": 0.92,
               "slots": {"tree0": (0, 0), "tree1": (0, 1), "tree2": (0, 2), "tree3": (0, 3), "bush0": (1, 0), "bush1": (1, 2)},
               "mountains": ["mountains_kanion.jpg"]},
    "zima": {"fog": (226, 234, 244), "ground": "ground_zima.jpg", "trees": "trees_sheet_zima.jpg", "color": 1.0, "bright": 0.9, "contrast": 1.5,
             "slots": {"tree0": (0, 0), "tree1": (0, 1), "tree2": (0, 2), "tree3": (0, 3), "bush0": (1, 0), "bush1": (1, 1)},
             "mountains": ["mountains_zima.jpg"]},
    "jesien": {"fog": (230, 212, 194), "ground": "ground_jesien.jpg", "trees": "trees_sheet_jesien.jpg", "sky_flood": 18, "color": 1.05, "bright": 1.05, "crop": 0.85, "contrast": 0.8,
               "slots": {"tree0": (0, 0), "tree1": (0, 1), "tree2": (0, 2), "tree3": (0, 3), "bush0": (1, 0), "bush1": (1, 2)},
               "mountains": ["mountains_jesien.jpg"]},
}


def src(name):
    p = os.path.join(SRC, name)
    return p if os.path.exists(p) else None


# ---------------------------------------------------------------- obiekty z arkuszy

def fit_bottom(obj, w, h, margin=1):
    """Obiekt z alfa przyciety do ramki, przeskalowany z zachowaniem proporcji, stoi na dolnej krawedzi kafelka."""
    bbox = obj.getchannel("A").point(lambda v: 255 if v > 40 else 0).getbbox()
    if bbox:
        obj = obj.crop(bbox)
    ow, oh = obj.size
    k = min((w - 2 * margin) / ow, (h - margin) / oh)
    nw, nh = max(1, int(ow * k)), max(1, int(oh * k))
    obj = obj.resize((nw, nh), Image.LANCZOS)
    tile = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    tile.paste(obj, ((w - nw) // 2, h - nh), obj)
    return tile


def sheet_objects(path, cols, rows, names, holes=False):
    sheet = Image.open(path).convert("RGB")
    sheet = sheet.resize((sheet.size[0] // 2, sheet.size[1] // 2), Image.LANCZOS)
    out = {}
    for r, c, cell in cells(sheet, cols, rows):
        n = names.get((r, c))
        if n:
            out[n] = erode_alpha(key_magenta(cell, holes), 1)
    return out


# ---------------------------------------------------------------- tekstury nawierzchni

def make_tileable(im, size=512, crop=1.0):
    """Kwadrat ze srodka (crop = czesc krotszego boku: mniej = wieksze elementy na kafelku) -> bezszwowy: przy
    krawedziach kopia przesunieta o pol kafelka (jej brzegi to srodek oryginalu, wiec stykaja sie ciagle przy
    zawinieciu), w srodku oryginal; granica miekka."""
    w, h = im.size
    s = int(min(w, h) * crop)
    im = im.crop(((w - s) // 2, (h - s) // 2, (w - s) // 2 + s, (h - s) // 2 + s)).resize((size, size), Image.LANCZOS)
    rolled = ImageChops.offset(im, size // 2, size // 2)
    mask = Image.new("L", (size, size))
    mp = mask.load()
    for y in range(size):
        for x in range(size):
            d = min(x, size - 1 - x, y, size - 1 - y) / (size * 0.5)   # 0 brzeg .. 1 srodek
            t = min(1.0, max(0.0, (d - 0.12) / 0.38))
            mp[x, y] = int(255 * t * t * (3 - 2 * t))
    return Image.composite(im, rolled, mask)


def ground_tiles(path, crop=0.5, far_blur=1.6, near_blur=0.0, contrast=1.0, color=1.0, bright=1.0):
    """(bliska, daleka) 128x128 RGB z obrazu nawierzchni. crop < 1: wieksze elementy (czytelne z bliska, mniej
    migotania niz drobny szum calego obrazu upchniety w 128 tekseli); color/bright: dopasowanie do tonu sceny."""
    im = make_tileable(Image.open(path).convert("RGB"), crop=crop)
    im = ImageEnhance.Brightness(ImageEnhance.Color(im).enhance(color)).enhance(bright)
    base = im.resize((128, 128), Image.LANCZOS)
    near = base.filter(ImageFilter.GaussianBlur(near_blur)) if near_blur else base
    near = ImageEnhance.Contrast(near).enhance(contrast)
    # daleka: rozmycie z zawinieciem (kopie 3x3, srodek) - GaussianBlur nie wie o kafelkowaniu
    big = Image.new("RGB", (384, 384))
    for oy in range(3):
        for ox in range(3):
            big.paste(base, (ox * 128, oy * 128))
    far = big.filter(ImageFilter.GaussianBlur(far_blur)).crop((128, 128, 256, 256))
    far = ImageEnhance.Contrast(far).enhance(contrast * 0.7)
    return near, far


def crowd_tile(path):
    """128x32 publicznosc: pas ze srodka obrazu, bezszwowy w poziomie (przenikanie koncow)."""
    im = Image.open(path).convert("RGB")
    w, h = im.size
    # waski wycinek (proporcje kafelka 4:1) z kilkoma rzedami - postacie wieksze niz przy calej szerokosci trybuny
    bh = int(h * 0.22)
    bw = min(w, bh * 4)
    y0 = int(h * 0.42)
    band = im.crop(((w - bw) // 2, y0, (w - bw) // 2 + bw, y0 + bh))
    band = band.resize((256, 64), Image.LANCZOS)
    rolled = ImageChops.offset(band, 128, 0)
    mask = Image.new("L", band.size)
    mp = mask.load()
    for x in range(256):
        d = min(x, 255 - x) / 128.0
        t = min(1.0, max(0.0, (d - 0.1) / 0.4))
        for y in range(64):
            mp[x, y] = int(255 * t)
    return Image.composite(band, rolled, mask).resize((128, 32), Image.LANCZOS)


def atlas_tiles(theme="jezioro"):
    """Kafelki atlasu motywu dla gen_kart_atlas.py: {nazwa: obraz} - asphalt, grass, grass_far, crowd (RGB),
    tree0..3, bush0..1 (RGBA, twarda alfa pod alpha-test). Brak zrodla = brak klucza (zostaje rysowany)."""
    cfg = THEMES[theme]
    tiles = {}
    p = src("asphalt.jpg")
    if p:
        tiles["asphalt"], _ = ground_tiles(p, crop=0.45, near_blur=0.3, contrast=1.25, bright=1.12)
    p = src(cfg["ground"])
    if p:
        tiles["grass"], tiles["grass_far"] = ground_tiles(p, crop=cfg.get("crop", 0.5), contrast=cfg.get("contrast", 1.0),
                                                   color=cfg.get("color", 1.25), bright=cfg.get("bright", 1.25))
    p = src("crowd.jpg")
    if p:
        tiles["crowd"] = crowd_tile(p)
    p = src(cfg["trees"])
    if p:
        names = {v: k for k, v in cfg["slots"].items()}
        for name, obj in sheet_objects(p, 4, 2, names, holes=True).items():
            size = (64, 64) if name.startswith("bush") else (128, 128)
            t = fit_bottom(obj, *size)
            t.putalpha(t.getchannel("A").point(lambda v: 255 if v >= 128 else 0))
            tiles[name] = t
    return tiles


# ---------------------------------------------------------------- niebo

def key_sky(path, sky_flood=0):
    """Panorama z niebem magenta -> RGBA przycieta od szczytow do dolu obrazu.
    sky_flood > 0: dodatkowo tlem jest wszystko polaczone z GORNA krawedzia, co ma "magentowosc" >= sky_flood
    (Gemini dorysowal na niebie jesieni rozowe oblaki, nie czysta magente). Nie dla kanionu - tam fioletowe gory
    w tle stykaja sie z niebem."""
    im = Image.open(path).convert("RGB")
    im = im.resize((im.size[0] // 2, im.size[1] // 2), Image.LANCZOS)
    rgba = erode_alpha(key_magenta(im, holes=True), 1)
    if sky_flood:
        from gen_snake_assets import magentaness
        w, h = im.size
        src_px, cp = im.load(), rgba.load()
        seen = bytearray(w * h)
        stack = [(x, 0) for x in range(w)]
        while stack:
            x, y = stack.pop()
            i = y * w + x
            if seen[i]:
                continue
            seen[i] = 1
            r, g, b = src_px[x, y]
            pinkish = magentaness(r, g, b) >= sky_flood or (r - g > 40 and b >= g - 5)   # roz / losos oblakow
            if cp[x, y][3] and not pinkish:
                continue
            cp[x, y] = (0, 0, 0, 0)
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if 0 <= nx < w and 0 <= ny < h and not seen[ny * w + nx]:
                    stack.append((nx, ny))
    bbox = rgba.getchannel("A").point(lambda v: 255 if v > 40 else 0).getbbox()
    if bbox:
        rgba = rgba.crop((0, bbox[1], rgba.size[0], rgba.size[1]))
    return rgba


def to_fog_bottom(img, fog, frac=0.35):
    """Dolna czesc pasa przenika w kolor mgly (horyzont) - gory nie koncza sie ostra krawedzia na drodze."""
    w, h = img.size
    px = img.load()
    start = int(h * (1 - frac))
    for y in range(start, h):
        t = (y - start) / max(1, h - 1 - start)
        t = t * t
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            px[x, y] = (int(r + (fog[0] - r) * t), int(g + (fog[1] - g) * t), int(b + (fog[2] - b) * t), a)
    return img


def crossfade_loop(parts, total_w, h, overlap):
    """Sklada paski w petle o szerokosci total_w (koniec laczy sie z poczatkiem). Segment i zajmuje
    [i*seg, i*seg + seg + overlap) modulo total_w; na szwie (overlap px) wagi dwoch segmentow sumuja sie do 1.
    Kolory przechodza liniowo, a sylwetki SUMUJA sie: alfa = max z obu stron, kazda strona zanika dopiero w swojej
    dalszej polowie szwu - bez "duchow" polprzezroczystych szczytow, ktore dawalo zwykle przenikanie."""
    n = len(parts)
    seg = total_w // n
    scaled = [p.resize((seg + overlap, h), Image.LANCZOS).load() for p in parts]
    contrib = [[[] for _ in range(total_w)] for _ in range(h)]
    for i in range(n):
        sp = scaled[i]
        for lx in range(seg + overlap):
            if lx < overlap:
                w = (lx + 0.5) / overlap
            elif lx >= seg:
                w = 1.0 - (lx - seg + 0.5) / overlap
            else:
                w = 1.0
            x = (i * seg + lx) % total_w
            for y in range(h):
                contrib[y][x].append((w, sp[lx, y]))
    data = []
    for y in range(h):
        for x in range(total_w):
            c = contrib[y][x]
            if len(c) == 1:
                data.append(c[0][1])
                continue
            a = max(px[3] * min(1.0, 2 * w) for w, px in c)
            ks = sum(w * px[3] for w, px in c)
            if a <= 0 or ks <= 0:
                data.append((0, 0, 0, 0))
                continue
            data.append(tuple(int(sum(w * px[3] * px[k] for w, px in c) / ks) for k in range(3)) + (int(a),))
    img = Image.new("RGBA", (total_w, h))
    img.putdata(data)
    return img


def mountains(theme):
    cfg = THEMES[theme]
    paths = [src(n) for n in cfg["mountains"]]
    paths = [p for p in paths if p]
    if not paths:
        print("  BRAK panoramy %s - zostaje mountains.png" % theme)
        return
    parts = [key_sky(p, cfg.get("sky_flood", 0)) for p in paths]
    seq = [parts[i % len(parts)] for i in range(4)]   # A B A B (jedna panorama: A A A A)
    strip = to_fog_bottom(crossfade_loop(seq, 2048, 160, 96), cfg["fog"])
    name = "mountains_%s.png" % theme
    premultiply_clean(strip).save(os.path.join(DST, name), optimize=True)
    print("  %s 2048x160" % name)


def clouds():
    p = src("clouds.jpg")
    if not p:
        print("  BRAK clouds.jpg - zostaje rysowany pas chmur")
        return
    im = Image.open(p).convert("RGB")
    im = im.resize((im.size[0] // 2, im.size[1] // 2), Image.LANCZOS)
    # Gemini dorysowal pod chmurami wzgorza - ucinamy od pierwszego wiersza, w ktorym >25% pikseli jest zielonych
    w, h = im.size
    px = im.load()
    for y in range(h // 3, h):
        green = sum(1 for x in range(0, w, 4) if px[x, y][1] > px[x, y][0] + 15 and px[x, y][1] > px[x, y][2] - 10)
        if green > 0.25 * (w // 4):
            im = im.crop((0, 0, w, max(1, y - 24)))
            break
    rgba = erode_alpha(key_magenta(im, holes=True), 1)
    # chmury sa szaro-biale: piksel wyraznie kolorowy (resztki wzgorz, rozowa poswiata) = tlo
    cp = rgba.load()
    for y in range(rgba.size[1]):
        for x in range(rgba.size[0]):
            r, g, b, a = cp[x, y]
            if a and max(r, g, b) - min(r, g, b) > 45:
                cp[x, y] = (0, 0, 0, 0)
    bbox = rgba.getchannel("A").point(lambda v: 255 if v > 40 else 0).getbbox()
    if bbox:
        rgba = rgba.crop((0, bbox[1], rgba.size[0], bbox[3]))
    strip = crossfade_loop([rgba], 1024, 160, 120)
    premultiply_clean(strip).save(os.path.join(DST, "clouds.png"), optimize=True)
    print("  clouds.png 1024x160")


ITEM_CELLS = {(0, 0): "banana", (0, 1): "shell", (0, 2): "mushroom"}


def items():
    p = src("items_sheet.jpg")
    if not p:
        print("  BRAK items_sheet.jpg - zostaja rysowane ikony")
        return
    from gen_snake_assets import fit
    for name, obj in sheet_objects(p, 3, 2, ITEM_CELLS).items():   # Gemini dal 3x2 - gorny rzad
        ic = fit(obj, 48)
        premultiply_clean(ic).save(os.path.join(DST, name + ".png"), optimize=True)
        print("  %s.png 48x48" % name)


def main():
    os.makedirs(DST, exist_ok=True)
    only = sys.argv[1:]
    for theme in THEMES:
        if not only or theme in only or "mountains" in only:
            mountains(theme)
    if not only or "clouds" in only:
        clouds()
    if not only or "items" in only:
        items()
    for theme in THEMES:
        t = atlas_tiles(theme)
        print("  atlas %-8s z Gemini: %s" % (theme, ", ".join(sorted(t)) if t else "brak"))


if __name__ == "__main__":
    main()
