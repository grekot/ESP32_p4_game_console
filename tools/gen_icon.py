"""Ikona programu na Windows (emulator, program startowy, instalator): sim/app.ico.

Zaokraglony kwadrat w kolorach menu (slate + akcent) z sylwetka pada. Deterministyczna, Pillow.
    python tools/gen_icon.py
"""
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "sim" / "app.ico"
S = 1024  # rysujemy duzo wieksze i zmniejszamy - gladkie krawedzie


def main():
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    # tlo: pionowy gradient granat -> fiolet w zaokraglonym kwadracie
    grad = Image.new("RGBA", (S, S))
    gd = ImageDraw.Draw(grad)
    top, bottom = (30, 41, 82), (109, 40, 217)
    for y in range(S):
        t = y / (S - 1)
        gd.line([(0, y), (S, y)], fill=tuple(int(a + (b - a) * t) for a, b in zip(top, bottom)) + (255,))
    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle([24, 24, S - 24, S - 24], radius=210, fill=255)
    im.paste(grad, (0, 0), mask)

    d = ImageDraw.Draw(im)
    # korpus pada: prostokat z dwoma uchwytami
    body = (245, 247, 250, 255)
    d.rounded_rectangle([150, 360, 874, 700], radius=170, fill=body)
    d.ellipse([130, 470, 420, 860], fill=body)
    d.ellipse([604, 470, 894, 860], fill=body)
    # krzyzak
    dark = (30, 41, 82, 255)
    cx, cy, a, b = 330, 520, 150, 48
    d.rounded_rectangle([cx - a / 2, cy - b, cx + a / 2, cy + b], radius=14, fill=dark)
    d.rounded_rectangle([cx - b, cy - a / 2, cx + b, cy + a / 2], radius=14, fill=dark)
    # przyciski A B X Y
    colors = [(239, 68, 68), (34, 197, 94), (59, 130, 246), (234, 179, 8)]
    for (dx, dy), c in zip([(70, 0), (0, 70), (0, -70), (-70, 0)], colors):
        x, y, r = 694 + dx, 520 + dy, 40
        d.ellipse([x - r, y - r, x + r, y + r], fill=c + (255,))

    sizes = [(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (24, 24), (16, 16)]
    im.resize((256, 256), Image.LANCZOS).save(OUT, sizes=sizes)
    print(f"{OUT} ({OUT.stat().st_size} B)")


if __name__ == "__main__":
    main()
