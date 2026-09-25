"""Generator labiryntow Pacmana 27x19 (narzedzie deweloperskie, wynik wkleja sie do pacman_mazes.h).

Model: wezly korytarzy na nieparzystych wspolrzednych (13 kolumn x 9 wierszy), sciany na parzystych.
Start: pelna krata krawedzi (kazde pole 2x2 wezlow to petla wokol filara 1x1). Potem losowo usuwane sa krawedzie
(razem z lustrzanym odbiciem, labirynt jest symetryczny lewo-prawo), o ile kazdy wezel zachowa stopien >= 2
(zero slepych zaulkow) i graf zostanie spojny. Dom duchow w srodku (drzwi (13,8), wnetrze (12..14,9)) jest staly,
korytarze nad i pod domem zawsze otwarte. Tunel w wierszu 9 (albo innym nieparzystym, --tunnel).

    python tools/pacman_maze_gen.py --seed 7 --remove 0.45 [--tunnel 9]

Wypisuje 19 wierszy w skladni C i statystyki (kulki, filary 1x1). Poprawnosc sprawdza pacman_maze_check.py.
"""
import argparse
import random

COLS, ROWS = 27, 19
NX, NY = 13, 9   # wezly: x = 1 + 2*i, y = 1 + 2*j


def node_ok(i, j):
    x, y = 1 + 2 * i, 1 + 2 * j
    if y == 9 and x in (11, 13, 15):   # dom duchow
        return False
    return True


def mirror_i(i):
    return NX - 1 - i


def edges_all():
    e = set()
    for j in range(NY):
        for i in range(NX):
            if not node_ok(i, j):
                continue
            if i + 1 < NX and node_ok(i + 1, j):
                e.add(((i, j), (i + 1, j)))
            if j + 1 < NY and node_ok(i, j + 1):
                e.add(((i, j), (i, j + 1)))
    return e


def mirror_edge(e):
    (a, b) = e
    ma = (mirror_i(a[0]), a[1])
    mb = (mirror_i(b[0]), b[1])
    return tuple(sorted((ma, mb)))


def degree(edges, tunnel_nodes):
    d = {}
    for a, b in edges:
        d[a] = d.get(a, 0) + 1
        d[b] = d.get(b, 0) + 1
    for n in tunnel_nodes:
        d[n] = d.get(n, 0) + 1
    return d


def connected(edges, nodes, tunnel_nodes):
    adj = {n: set() for n in nodes}
    for a, b in edges:
        adj[a].add(b)
        adj[b].add(a)
    if len(tunnel_nodes) == 2:
        adj[tunnel_nodes[0]].add(tunnel_nodes[1])
        adj[tunnel_nodes[1]].add(tunnel_nodes[0])
    start = next(iter(nodes))
    seen = {start}
    stack = [start]
    while stack:
        n = stack.pop()
        for m in adj[n]:
            if m not in seen:
                seen.add(m)
                stack.append(m)
    return len(seen) == len(nodes)


def generate(seed, remove_frac, tunnel_row):
    rnd = random.Random(seed)
    nodes = {(i, j) for j in range(NY) for i in range(NX) if node_ok(i, j)}
    edges = edges_all()
    tj = (tunnel_row - 1) // 2
    tunnel_nodes = [(0, tj), (NX - 1, tj)]
    # korytarze nad i pod domem zawsze otwarte
    keep = set()
    for y in (7, 11):
        j = (y - 1) // 2
        keep.add(((5, j), (6, j)))
        keep.add(((6, j), (7, j)))
    cand = [e for e in edges if e not in keep and e[0][0] <= 6]   # lewa polowa + srodek
    rnd.shuffle(cand)
    target = int(len(edges) * remove_frac)
    removed = 0
    for e in cand:
        if removed >= target:
            break
        m = mirror_edge(e)
        trial = set(edges)
        trial.discard(e)
        trial.discard(m)
        if trial == edges:
            continue
        d = degree(trial, tunnel_nodes)
        if any(d.get(n, 0) < 2 for n in nodes):
            continue
        if not connected(trial, nodes, tunnel_nodes):
            continue
        edges = trial
        removed += 1 if e == m else 2
    return edges, tunnel_nodes


def render(edges, tunnel_row):
    g = [["#"] * COLS for _ in range(ROWS)]
    for j in range(NY):
        for i in range(NX):
            if node_ok(i, j):
                g[1 + 2 * j][1 + 2 * i] = "."
    for (a, b) in edges:
        (i1, j1), (i2, j2) = a, b
        x, y = 1 + i1 + i2, 1 + j1 + j2   # pole miedzy wezlami
        g[y][x] = "."
    # dom duchow
    for x in range(11, 16):
        g[8][x] = "#"
        g[10][x] = "#"
    g[8][13] = "-"
    g[9][11] = "#"
    g[9][15] = "#"
    for x in (12, 13, 14):
        g[9][x] = "G"
    g[7][13] = " "   # nad drzwiami bez kulki (tam wychodza duchy)
    g[11][13] = "F"
    g[13][13] = "P"
    for x, y in ((1, 3), (25, 3), (1, 15), (25, 15)):
        g[y][x] = "o"
    # tunel
    g[tunnel_row][0] = " "
    g[tunnel_row][COLS - 1] = " "
    return ["".join(r) for r in g]


def stats(rows):
    pellets = sum(r.count(".") for r in rows)
    pillars = 0
    for y in range(2, ROWS - 1, 2):
        for x in range(2, COLS - 1, 2):
            if rows[y][x] == "#" and all(rows[y + dy][x + dx] != "#" for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                pillars += 1
    return pellets, pillars


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--remove", type=float, default=0.45)
    ap.add_argument("--tunnel", type=int, default=9)
    ap.add_argument("--count", type=int, default=1, help="ile kolejnych ziaren wypisac")
    a = ap.parse_args()
    for s in range(a.seed, a.seed + a.count):
        edges, _ = generate(s, a.remove, a.tunnel)
        rows = render(edges, a.tunnel)
        pellets, pillars = stats(rows)
        print("// seed %d remove %.2f tunnel %d: kulek %d, filarow 1x1 %d" % (s, a.remove, a.tunnel, pellets, pillars))
        for r in rows:
            print('        "%s",' % r)
        print()


if __name__ == "__main__":
    main()
