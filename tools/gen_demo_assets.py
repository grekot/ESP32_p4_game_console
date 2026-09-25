#!/usr/bin/env python3
"""Proceduralne PNG dla gier pokazowych (assets/labirynt3d, assets/kosmos) - czysty Python (zlib), bez PIL.

    python tools/gen_demo_assets.py          # nadpisuje pliki w assets/labirynt3d/ i assets/kosmos/

Grafika jest deterministyczna (staly seed). Wlasne PNG o tych samych nazwach po prostu ja zastepuja -
gry czytaja pliki przez platform::read_file, nic w kodzie nie trzeba zmieniac.
"""
import math
import os
import random
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
rng = random.Random(4242)


class Img:
    def __init__(self, w, h, fill=(0, 0, 0, 0)):
        self.w, self.h = w, h
        self.px = [list(fill) for _ in range(w * h)]

    def set(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y * self.w + x] = [c[0], c[1], c[2], 255 if len(c) == 3 else c[3]]

    def get(self, x, y):
        return self.px[y * self.w + x]

    def rect(self, x, y, w, h, c):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.set(xx, yy, c)

    def circle(self, cx, cy, r, c):
        for yy in range(int(cy - r) - 1, int(cy + r) + 2):
            for xx in range(int(cx - r) - 1, int(cx + r) + 2):
                if (xx - cx) ** 2 + (yy - cy) ** 2 <= r * r:
                    self.set(xx, yy, c)

    def save(self, path):
        raw = bytearray()
        for y in range(self.h):
            raw.append(0)
            for x in range(self.w):
                raw += bytes(self.px[y * self.w + x])

        def chunk(tag, data):
            return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

        png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", self.w, self.h, 8, 6, 0, 0, 0))
        png += chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(png)
        print(f"{os.path.relpath(path, ROOT)}  {self.w}x{self.h}  {len(png)} B")


def shade(c, k):
    return (max(0, min(255, int(c[0] * k))), max(0, min(255, int(c[1] * k))), max(0, min(255, int(c[2] * k))))


def noise(c, amount):
    k = 1.0 + rng.uniform(-amount, amount)
    return shade(c, k)


# ------------------------------------------------------------------ labirynt 3D: tekstury 64x64

def tex_brick():
    img = Img(64, 64, (90, 70, 62, 255))            # zaprawa
    brick = (176, 66, 48)
    for row in range(8):
        y = row * 8
        off = 8 if row % 2 else 0
        for col in range(-1, 5):
            x = col * 16 + off
            c = noise(brick, 0.12)
            img.rect(x + 1, y + 1, 14, 6, c)
            img.rect(x + 1, y + 1, 14, 1, shade(c, 1.15))     # blik na gorze
            img.rect(x + 1, y + 6, 14, 1, shade(c, 0.8))      # cien na dole
            for _ in range(3):                                  # ubytki
                img.set(x + rng.randint(2, 13), y + rng.randint(2, 5), shade(c, 0.85))
    return img


def tex_stone():
    img = Img(64, 64, (60, 62, 68, 255))
    base = (128, 132, 140)
    blocks = [(0, 0, 32, 20), (32, 0, 32, 20), (0, 20, 20, 24), (20, 20, 44, 24), (0, 44, 40, 20), (40, 44, 24, 20)]
    for (x, y, w, h) in blocks:
        c = noise(base, 0.15)
        img.rect(x + 1, y + 1, w - 2, h - 2, c)
        img.rect(x + 1, y + 1, w - 2, 1, shade(c, 1.2))
        img.rect(x + 1, y + 1, 1, h - 2, shade(c, 1.1))
        img.rect(x + 1, y + h - 2, w - 2, 1, shade(c, 0.75))
        for _ in range(w * h // 12):
            img.set(x + rng.randint(2, w - 3), y + rng.randint(2, h - 3), noise(c, 0.1))
    return img


def tex_door():
    img = Img(64, 64, (52, 34, 18, 255))
    plank = (150, 100, 52)
    for i in range(6):
        x = 2 + i * 10
        c = noise(plank, 0.1)
        img.rect(x, 2, 9, 60, c)
        for y in range(2, 62):                                   # slojе
            if (y * 7 + i * 13) % 11 == 0:
                img.rect(x + 1, y, 7, 1, shade(c, 0.85))
        img.rect(x, 2, 1, 60, shade(c, 1.15))
    for y in (8, 32, 54):                                        # okucia
        img.rect(2, y, 60, 3, (70, 72, 80))
        for x in (6, 30, 54):
            img.set(x, y + 1, (200, 200, 210))
    img.circle(50, 34, 3, (220, 190, 60))                        # klamka
    img.rect(0, 0, 64, 2, (40, 26, 14))
    img.rect(0, 62, 64, 2, (40, 26, 14))
    return img


def tex_exit():
    img = Img(64, 64, (20, 30, 40, 255))
    for r in range(30, 2, -4):
        k = 0.4 + 0.6 * (1 - r / 30)
        img.circle(32, 32, r, shade((60, 220, 140), k))
        img.circle(32, 32, r - 2, (20, 30, 40))
    img.circle(32, 32, 6, (200, 255, 220))
    for i in range(24):                                          # iskry
        a = i * math.pi / 12
        d = rng.uniform(12, 30)
        img.set(int(32 + math.cos(a) * d), int(32 + math.sin(a) * d), (160, 255, 200))
    return img


def sprite_coin(size=32):
    img = Img(size, size)
    c, r = size / 2 - 0.5, size / 2 - 1
    img.circle(c, c, r, (200, 150, 20))
    img.circle(c, c, r - 3, (250, 215, 50))
    img.circle(c - r * 0.3, c - r * 0.3, r * 0.3, (255, 240, 150))
    img.rect(int(c - 2), int(c - r * 0.45), 4, int(r * 0.9), (200, 150, 20))
    return img


def sprite_portal(size=32):
    img = Img(size, size)
    c = size / 2 - 0.5
    for r in range(int(size / 2 - 1), 1, -3):
        k = 0.5 + 0.5 * (1 - r / (size / 2))
        img.circle(c, c, r, shade((80, 240, 160), k))
        img.circle(c, c, r - 1.5, (0, 0, 0, 0))
    img.circle(c, c, 3, (220, 255, 235))
    return img


# ------------------------------------------------------------------ kosmos: sprite'y

def sprite_ship(w=48, h=32):
    img = Img(w, h)
    hull, dark, light = (150, 160, 175), (90, 98, 110), (210, 215, 225)
    for x in range(w):
        # kadlub: klin zwezajacy sie w prawo
        half = max(1, int((w - x) * 0.55 * h / w) + (3 if x < 10 else 0))
        half = min(half, h // 2 - 1)
        for y in range(h // 2 - half, h // 2 + half):
            img.set(x, y, hull)
        img.set(x, h // 2 - half, light)
        img.set(x, h // 2 + half - 1, dark)
    img.rect(0, 4, 8, 6, (200, 60, 40))                          # skrzydla
    img.rect(0, h - 10, 8, 6, (200, 60, 40))
    img.rect(0, 12, 5, 8, (255, 140, 30))                        # dysza
    img.rect(1, 14, 3, 4, (255, 230, 120))
    img.circle(30, h / 2 - 0.5, 4, (70, 160, 240))               # kokpit
    img.circle(31, h / 2 - 1.5, 1.5, (200, 235, 255))
    return img


def sprite_asteroid(size):
    img = Img(size, size)
    c = size / 2 - 0.5
    base = (120, 110, 100)
    pts = [(rng.uniform(0.72, 1.0)) for _ in range(16)]           # nieregularny obrys
    for y in range(size):
        for x in range(size):
            dx, dy = x - c, y - c
            d = math.hypot(dx, dy) / (size / 2)
            a = math.atan2(dy, dx)
            i = int((a + math.pi) / (2 * math.pi) * 16) % 16
            j = (i + 1) % 16
            t = ((a + math.pi) / (2 * math.pi) * 16) % 1
            lim = pts[i] * (1 - t) + pts[j] * t
            if d <= lim:
                k = 0.6 + 0.4 * (1 - d) + (0.15 if dx - dy < 0 else 0)   # swiatlo z lewej-gory
                img.set(x, y, shade(base, k))
    for _ in range(size // 6):                                    # kratery
        cx, cy, r = rng.uniform(c * 0.4, c * 1.6), rng.uniform(c * 0.4, c * 1.6), rng.uniform(1.5, size / 10)
        for y in range(int(cy - r), int(cy + r) + 1):
            for x in range(int(cx - r), int(cx + r) + 1):
                if 0 <= x < size and 0 <= y < size and img.get(x, y)[3] and (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                    img.set(x, y, shade(tuple(img.get(x, y)[:3]), 0.7))
    return img


def sprite_bullet():
    img = Img(12, 4)
    img.rect(0, 1, 12, 2, (255, 230, 120))
    img.rect(4, 0, 8, 4, (255, 200, 60))
    img.rect(8, 1, 4, 2, (255, 255, 220))
    return img


def sprite_boom(frame, size=48):
    img = Img(size, size)
    c = size / 2 - 0.5
    r = (frame + 1) / 4 * (size / 2 - 1)
    cols = [(255, 240, 160), (255, 170, 40), (220, 70, 30), (90, 30, 20)]
    for k in range(4):
        rr = r * (1 - k * 0.22)
        if rr <= 0:
            continue
        col = cols[min(3, k + frame // 2)]
        for _ in range(int(rr * 8) + 8):
            a = rng.uniform(0, 2 * math.pi)
            d = rr * rng.uniform(0.6, 1.0)
            img.circle(c + math.cos(a) * d, c + math.sin(a) * d, max(1.0, rr * 0.28), col)
    return img


# ------------------------------------------------------------------ lekcja 14 (obrazki): sad z gory

def sprite_hero_top(step):
    """Bohater widziany z gory/przodu, 16x16; step 0/1 = klatki chodu (nogi na przemian)."""
    img = Img(16, 16)
    skin, hair, shirt, pants, shoe = (255, 205, 160), (120, 70, 30), (40, 120, 220), (40, 50, 110), (60, 40, 20)
    img.rect(5, 1, 6, 3, hair)                 # wlosy
    img.rect(5, 4, 6, 4, skin)                 # twarz
    img.set(6, 5, (30, 30, 40)); img.set(9, 5, (30, 30, 40))   # oczy
    img.rect(4, 8, 8, 4, shirt)                # koszulka
    img.rect(3, 8, 1, 3, skin); img.rect(12, 8, 1, 3, skin)    # rece
    if step == 0:
        img.rect(5, 12, 2, 3, pants); img.rect(9, 12, 2, 3, pants)
        img.rect(5, 15, 2, 1, shoe); img.rect(9, 15, 2, 1, shoe)
    else:
        img.rect(4, 12, 2, 2, pants); img.rect(10, 12, 2, 3, pants)
        img.rect(4, 14, 2, 1, shoe); img.rect(10, 15, 2, 1, shoe)
    return img


def sprite_apple():
    img = Img(8, 8)
    img.circle(3.5, 4.5, 3.2, (210, 40, 40))
    img.circle(2.5, 3.5, 1.0, (250, 120, 120))            # blik
    img.rect(3, 0, 2, 2, (100, 60, 20))                   # ogonek
    img.set(5, 1, (60, 160, 60))                          # listek
    return img


def sprite_bee(step):
    img = Img(12, 10)
    body, stripe, wing = (240, 200, 40), (40, 30, 20), (220, 235, 255)
    img.rect(2, 4, 8, 4, body)
    for x in (4, 7):
        img.rect(x, 4, 1, 4, stripe)
    img.rect(9, 3, 3, 4, stripe)                          # glowa
    img.set(10, 4, (255, 255, 255))                       # oko
    img.set(1, 6, stripe)                                 # zadlo
    if step == 0:
        img.rect(3, 1, 3, 3, wing); img.rect(6, 1, 3, 3, wing)
    else:
        img.rect(2, 3, 3, 1, wing); img.rect(7, 3, 3, 1, wing)
    return img


def sprite_tree():
    img = Img(24, 24)
    img.circle(11.5, 13.5, 10, (30, 90, 40))              # cien korony
    img.circle(11.5, 11.5, 10, (50, 150, 60))
    img.circle(8.5, 8.5, 4, (90, 190, 80))                # jasniejsza gora
    for (x, y) in ((5, 14), (15, 6), (17, 16), (9, 18)):  # jablka na drzewie
        img.set(x, y, (220, 50, 50))
    img.rect(10, 20, 4, 4, (100, 60, 20))                 # pien
    return img


def main():
    les = ROOT / "assets" / "obrazki"
    sprite_hero_top(0).save(str(les / "hero_0.png"))
    sprite_hero_top(1).save(str(les / "hero_1.png"))
    sprite_apple().save(str(les / "apple.png"))
    sprite_bee(0).save(str(les / "bee_0.png"))
    sprite_bee(1).save(str(les / "bee_1.png"))
    sprite_tree().save(str(les / "tree.png"))

    lab = ROOT / "assets" / "labirynt3d"
    tex_brick().save(str(lab / "brick.png"))
    tex_stone().save(str(lab / "stone.png"))
    tex_door().save(str(lab / "door.png"))
    tex_exit().save(str(lab / "exit.png"))
    sprite_coin().save(str(lab / "coin.png"))
    sprite_portal().save(str(lab / "portal.png"))

    kos = ROOT / "assets" / "kosmos"
    sprite_ship().save(str(kos / "ship.png"))
    sprite_asteroid(64).save(str(kos / "asteroid_l.png"))
    sprite_asteroid(40).save(str(kos / "asteroid_m.png"))
    sprite_asteroid(24).save(str(kos / "asteroid_s.png"))
    sprite_bullet().save(str(kos / "bullet.png"))
    for i in range(4):
        sprite_boom(i).save(str(kos / f"boom{i}.png"))


if __name__ == "__main__":
    main()
