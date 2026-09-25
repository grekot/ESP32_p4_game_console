"""Sprawdzenie ukladow torow Karta (src/games/kart/kart_tracks.h) bez uruchamiania gry.

Odtwarza linie srodkowa tak jak KartGame::build_track (Catmull-Rom, 16 punktow, 256 probek) i sprawdza:
  - margines swiata (probki >= 90 od brzegu 1024x1024),
  - odleglosc miedzy niesasiednimi fragmentami drogi (>= MIN_SEP: dwie jezdnie z kraweznikami i pas trawy),
  - najostrzejszy zakret (zmiana kierunku na 8 probkach) - nie ostrzej niz pierwotny tor (1,81),
  - miejsce na trybune: probka 9, 70-105 jednostek na lewo od toru, z dala od innych odcinkow drogi,
  - dlugosc okrazenia.
Opcjonalnie rysuje podglad PNG (--png plik): tor, trybuna, pola przyspieszenia, skrzynki.

    python tools/kart_tracks_check.py [--png podglad.png]
"""
import math
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HDR = os.path.join(ROOT, "src", "games", "kart", "kart_tracks.h")
N_PATH, N_CTRL, WORLD = 256, 16, 1024
MARGIN, MIN_SEP, MAX_TURN = 90, 120, 1.9    # MAX_TURN: rad na 8 probek (pierwotny tor: 1,81)


def parse():
    src = open(HDR, encoding="utf-8").read()
    tracks = []
    for m in re.finditer(r'\{\s*"(\w+)",\s*"([^"]*)",\s*(\d+),\s*\{(.*?)\}\s*\},\s*\{\s*(\d+),\s*(\d+)\s*\},\s*\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\}', src, re.S):
        pts = [tuple(float(v) for v in p) for p in re.findall(r"\{\s*(-?[\d.]+)f?,\s*(-?[\d.]+)f?\s*\}", m.group(4) + "}")]
        tracks.append({"id": m.group(1), "name": m.group(2), "theme": int(m.group(3)), "ctrl": pts,
                       "boost": (int(m.group(5)), int(m.group(6))), "box": (int(m.group(7)), int(m.group(8)), int(m.group(9)))})
    return tracks


def path(ctrl):
    per = N_PATH // N_CTRL
    out = []
    for s in range(N_CTRL):
        p0, p1, p2, p3 = ctrl[(s - 1) % N_CTRL], ctrl[s], ctrl[(s + 1) % N_CTRL], ctrl[(s + 2) % N_CTRL]
        for j in range(per):
            t = j / per
            t2, t3 = t * t, t * t * t
            out.append(tuple(0.5 * (2 * p1[k] + (-p0[k] + p2[k]) * t + (2 * p0[k] - 5 * p1[k] + 4 * p2[k] - p3[k]) * t2 +
                                    (-p0[k] + 3 * p1[k] - 3 * p2[k] + p3[k]) * t3) for k in (0, 1)))
    return out


def check(tr):
    p = path(tr["ctrl"])
    errs = []
    if len(tr["ctrl"]) != N_CTRL:
        errs.append("punktow %d zamiast %d" % (len(tr["ctrl"]), N_CTRL))
    lo = min(min(x, y) for x, y in p)
    hi = max(max(x, y) for x, y in p)
    if lo < MARGIN or hi > WORLD - MARGIN:
        errs.append("margines: min %.0f max %.0f (wymagane %d..%d)" % (lo, hi, MARGIN, WORLD - MARGIN))
    sep = 1e9
    for i in range(N_PATH):
        for j in range(N_PATH):
            d = min(abs(i - j), N_PATH - abs(i - j))
            if d < 24:
                continue
            sep = min(sep, math.dist(p[i], p[j]))
    if sep < MIN_SEP:
        errs.append("odcinki drogi za blisko: %.0f < %d" % (sep, MIN_SEP))
    ang = [math.atan2(p[(i + 1) % N_PATH][1] - p[i][1], p[(i + 1) % N_PATH][0] - p[i][0]) for i in range(N_PATH)]
    turn = 0
    for i in range(N_PATH):
        a = ang[(i + 8) % N_PATH] - ang[i]
        a = (a + math.pi) % (2 * math.pi) - math.pi
        turn = max(turn, abs(a))
    if turn > MAX_TURN:
        errs.append("za ostry zakret: %.2f rad / 8 probek (max %.2f)" % (turn, MAX_TURN))
    stand = 1e9
    for i in (9,):   # trybuna stoi przy probce 9 (kart_render.cpp, build_props)
        tx, ty = p[(i + 1) % N_PATH][0] - p[i][0], p[(i + 1) % N_PATH][1] - p[i][1]
        l = math.hypot(tx, ty) or 1
        rx, ry = -ty / l, tx / l   # prawo (os y w dol)
        for off in (70, 92, 105):
            sx, sy = p[i][0] - rx * off, p[i][1] - ry * off
            for j in range(N_PATH):
                if min(abs(j - i), N_PATH - abs(j - i)) < 30:
                    continue
                stand = min(stand, math.dist((sx, sy), p[j]))
            if not (10 < sx < WORLD - 10 and 10 < sy < WORLD - 10):
                errs.append("trybuna poza swiatem")
    if stand < 50:
        errs.append("trybuna za blisko innego odcinka drogi: %.0f" % stand)
    length = sum(math.dist(p[i], p[(i + 1) % N_PATH]) for i in range(N_PATH))
    return p, errs, sep, turn, length


def main():
    tracks = parse()
    if not tracks:
        sys.exit("nie znaleziono torow w " + HDR)
    png = sys.argv[sys.argv.index("--png") + 1] if "--png" in sys.argv else None
    bad = 0
    imgs = []
    for tr in tracks:
        p, errs, sep, turn, length = check(tr)
        print("%-12s motyw %d  dlugosc %5.0f  min. odstep %4.0f  zakret %.2f  %s" %
              (tr["id"], tr["theme"], length, sep, turn, "OK" if not errs else "BLAD: " + "; ".join(errs)))
        bad += bool(errs)
        imgs.append((tr, p))
    if png:
        from PIL import Image, ImageDraw
        S = 256
        out = Image.new("RGB", (S * len(imgs), S), (30, 90, 40))
        d = ImageDraw.Draw(out)
        for n, (tr, p) in enumerate(imgs):
            k = S / WORLD
            pts = [(n * S + x * k, y * k) for x, y in p] + [(n * S + p[0][0] * k, p[0][1] * k)]
            d.line(pts, fill=(80, 80, 90), width=int(76 * k) + 1)
            d.line(pts, fill=(230, 230, 230), width=1)
            d.ellipse([pts[0][0] - 4, pts[0][1] - 4, pts[0][0] + 4, pts[0][1] + 4], fill=(255, 255, 255))
            for b in tr["boost"]:
                x, y = pts[b]
                d.rectangle([x - 3, y - 3, x + 3, y + 3], fill=(60, 160, 255))
            for b in tr["box"]:
                x, y = pts[b]
                d.rectangle([x - 3, y - 3, x + 3, y + 3], fill=(255, 150, 215))
            d.text((n * S + 6, 6), tr["name"], fill=(255, 255, 255))
        out.save(png)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
