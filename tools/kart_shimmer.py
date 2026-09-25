"""Pomiar migotania tekstur Karta w oddali (bez mipmap drobny detal "iskrzy" przy ruchu kamery).

Dla kilku chwil jazdy autopilotem na torze 0 robi zrzuty dwoch KOLEJNYCH klatek (N, N+1) i liczy srednia
bezwzgledna roznice jasnosci w pasie pod horyzontem (dalekie pobocze i teren), z pominieciem HUD. Kamera przesuwa
sie w obu wersjach tak samo, wiec porownanie "przed / po" zmianie tekstur mowi, ile migotania dodaly same tekstury.

    python tools/kart_shimmer.py            (emulator musi byc zbudowany: sim/build/console_sim.exe)
"""
import os
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops, ImageStat

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "sim", "build", "console_sim.exe")
FRAMES = [400, 700, 1000, 1300, 1600]
BAND = (0, 150, 640, 240)   # x0, y0, x1, y1: pas pod horyzontem, bez mini-mapy i panelu miejsc


def shot(frame, path, extra):
    args = [EXE, "--game", "kart"] + extra + ["--hold", "A", "20", "21", "--hold", "Y", "80", str(frame + 1),
                                              "--frames", str(frame), "--shot", path]
    subprocess.run(args, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    return Image.open(path).convert("L").crop(BAND)


def main():
    extra = sys.argv[1:]   # np. --hold RIGHT 3 4  (inny tor)
    tmp = tempfile.mkdtemp()
    vals = []
    for f in FRAMES:
        a = shot(f, os.path.join(tmp, "a.bmp"), extra)
        b = shot(f + 1, os.path.join(tmp, "b.bmp"), extra)
        v = ImageStat.Stat(ImageChops.difference(a, b)).mean[0]
        vals.append(v)
        print("  klatka %4d: roznica %.2f" % (f, v))
    print("srednio %.2f (skala 0-255; im mniej, tym mniej migotania)" % (sum(vals) / len(vals)))


if __name__ == "__main__":
    main()
