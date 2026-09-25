"""Sprawdza labirynty Pacmana z src/games/pacman/pacman_mazes.h (bez kompilacji).

Dla kazdego labiryntu: szerokosc 27 kolumn x 19 wierszy, jeden start 'P', dom duchow ('-' drzwi, 'G' wnetrze),
osiagalnosc kazdego korytarza z pola startu (BFS), slepe zaulki (korytarz z jednym sasiadem - w Pacmanie
zabronione), obszary 2x2 otwarte (mylace przy ruchu po kratkach), liczba kulek i duzych kulek, wiersze tunelu
(otwarte na krawedzi - musza byc otwarte z obu stron). Kod wyjscia 1, gdy cos jest zle.

    python tools/pacman_maze_check.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src", "games", "pacman", "pacman_mazes.h")

COLS, ROWS = 27, 19
OPEN = set(".o PF-G")   # co nie jest sciana (drzwi i wnetrze domu: tylko dla duchow)


def load():
    text = open(SRC, encoding="ascii").read()
    mazes = []
    for block in re.finditer(r'\{\s*"([^"]*)",\s*\{((?:\s*"[^"]*",?\s*)+)\}\s*\}', text):
        name = block.group(1)
        rows = re.findall(r'"([^"]*)"', block.group(2))
        mazes.append((name, rows))
    return mazes


def check(name, rows):
    bad = []
    if len(rows) != ROWS:
        bad.append("wierszy %d, ma byc %d" % (len(rows), ROWS))
    for i, r in enumerate(rows):
        if len(r) != COLS:
            bad.append("wiersz %d ma %d znakow, ma byc %d" % (i, len(r), COLS))
    if bad:
        return bad, None

    def at(x, y):
        if y < 0 or y >= ROWS:
            return "#"
        if x < 0 or x >= COLS:
            return rows[y][0] if x < 0 else rows[y][-1]   # tunel: poza krawedzia = to samo co krawedz
        return rows[y][x]

    def walk(x, y):
        return at(x, y) in ".o PF"

    starts = [(x, y) for y in range(ROWS) for x in range(COLS) if rows[y][x] == "P"]
    if len(starts) != 1:
        bad.append("start P: %d (ma byc 1)" % len(starts))
    doors = [(x, y) for y in range(ROWS) for x in range(COLS) if rows[y][x] == "-"]
    if len(doors) != 1:
        bad.append("drzwi '-': %d (ma byc 1)" % len(doors))
    house = [(x, y) for y in range(ROWS) for x in range(COLS) if rows[y][x] == "G"]
    if len(house) < 3:
        bad.append("wnetrze domu 'G': %d pol (min 3)" % len(house))
    if doors:
        dx, dy = doors[0]
        if not walk(dx, dy - 1):
            bad.append("nad drzwiami (%d,%d) nie ma korytarza" % (dx, dy - 1))
        if at(dx, dy + 1) != "G":
            bad.append("pod drzwiami (%d,%d) nie ma wnetrza domu" % (dx, dy + 1))

    # tunel: wiersz otwarty na krawedzi musi byc otwarty z obu stron
    tunnels = []
    for y in range(ROWS):
        l, r = walk(0, y), walk(COLS - 1, y)
        if l != r:
            bad.append("wiersz %d otwarty tylko z jednej strony" % y)
        if l and r:
            tunnels.append(y)

    # BFS od startu
    seen = set()
    if starts:
        stack = [starts[0]]
        while stack:
            x, y = stack.pop()
            if (x, y) in seen:
                continue
            seen.add((x, y))
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                nx %= COLS
                if walk(nx, ny) and (nx, ny) not in seen:
                    stack.append((nx, ny))
    unreachable = [(x, y) for y in range(ROWS) for x in range(COLS) if walk(x, y) and (x, y) not in seen]
    if unreachable:
        bad.append("nieosiagalne pola: %s" % unreachable[:8])

    # slepe zaulki i obszary 2x2
    for y in range(ROWS):
        for x in range(COLS):
            if not walk(x, y):
                continue
            n = sum(1 for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)) if walk(nx % COLS, ny))
            if n <= 1:
                bad.append("slepy zaulek (%d,%d)" % (x, y))
            if walk(x + 1, y) and walk(x, y + 1) and walk(x + 1, y + 1) and x + 1 < COLS and y + 1 < ROWS:
                bad.append("otwarty blok 2x2 w (%d,%d)" % (x, y))
    pellets = sum(r.count(".") for r in rows)
    power = sum(r.count("o") for r in rows)
    info = "kulek %d, duzych %d, tunele w wierszach %s, dom %d pol" % (pellets, power, tunnels, len(house))
    return bad, info


def main():
    mazes = load()
    if not mazes:
        print("nie znaleziono labiryntow w", SRC)
        return 1
    rc = 0
    for name, rows in mazes:
        bad, info = check(name, rows)
        print("%-12s %s" % (name, info or ""))
        for b in bad:
            print("   BLAD:", b)
            rc = 1
    return rc


if __name__ == "__main__":
    sys.exit(main())
