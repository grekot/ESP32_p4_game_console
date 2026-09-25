"""Czcionki uzupelniajace z polskimi literami dla LVGL (UI konsoli i gfx/text.h w grach).

Wbudowane czcionki Montserrat w LVGL maja tylko ASCII (+ symbole). Zamiast generowac cale czcionki od nowa
(zmienilby sie wyglad kazdego napisu i wszystkie zrzuty testow), ten skrypt robi male czcionki z SAMYMI
18 polskimi znakami (a c e l n o s z z, male i wielkie) w formacie lv_font_fmt_txt, z polem `fallback`
wskazujacym na wbudowana lv_font_montserrat_N. LVGL szuka znaku najpierw w czcionce polskiej, potem w
zapasowej - tekst ASCII rysuje sie bajt w bajt tak samo jak dotad, a polskie litery pochodza z tego samego
kroju (Montserrat Medium, plik z LVGL: scripts/built_in_font/Montserrat-Medium.ttf, licencja OFL).

Wynik: src/ui/fonts/console_fonts_pl.c (+ naglowek console_fonts_pl.h). Deterministyczny. Wymaga Pillow.

    python tools/gen_pl_fonts.py
"""
import os
import re

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TTF_CANDIDATES = [
    os.path.join(ROOT, "managed_components", "lvgl__lvgl", "scripts", "built_in_font", "Montserrat-Medium.ttf"),
    os.path.join(ROOT, "third_party", "lvgl", "scripts", "built_in_font", "Montserrat-Medium.ttf"),
]
LVGL_FONT_DIRS = [
    os.path.join(ROOT, "managed_components", "lvgl__lvgl", "src", "font"),
    os.path.join(ROOT, "third_party", "lvgl", "src", "font"),
]
OUT_DIR = os.path.join(ROOT, "src", "ui", "fonts")
SIZES = [12, 14, 16, 20, 24, 28, 32, 40, 48]
# Polskie litery (kolejnosc kodow Unicode rosnaca - wymog cmap SPARSE)
CHARS = sorted("ąćęłńóśźżĄĆĘŁŃÓŚŹŻ")
BPP = 4


def find(paths):
    for p in paths:
        if os.path.exists(p):
            return p
    raise SystemExit("Nie znaleziono: %s (najpierw pio run albo tools/fetch_lvgl.ps1)" % paths[0])


def lvgl_metrics(size):
    """line_height i base_line z wbudowanej czcionki LVGL - czcionka polska musi miec identyczne."""
    d = find(LVGL_FONT_DIRS)
    src = open(os.path.join(d, "lv_font_montserrat_%d.c" % size), encoding="utf-8").read()
    lh = int(re.search(r"\.line_height\s*=\s*(\d+)", src).group(1))
    bl = int(re.search(r"\.base_line\s*=\s*(\d+)", src).group(1))
    return lh, bl


def render_glyph(font, ch):
    """Zwraca (adv_w w 1/16 px, box_w, box_h, ofs_x, ofs_y, piksele 0..15 wierszami)."""
    left, top, right, bottom = font.getbbox(ch, anchor="ls")
    w, h = right - left, bottom - top
    adv = int(round(font.getlength(ch) * 16))
    if w <= 0 or h <= 0:
        return adv, 0, 0, 0, 0, []
    img = Image.new("L", (w, h), 0)
    ImageDraw.Draw(img).text((-left, -top), ch, font=font, fill=255, anchor="ls")
    px = list(img.get_flattened_data() if hasattr(img, "get_flattened_data") else img.getdata())
    # przyciecie pustych brzegow (getbbox bywa o piksel za duzy)
    rows = [px[y * w:(y + 1) * w] for y in range(h)]
    while rows and max(rows[0]) < 8:
        rows.pop(0); top += 1
    while rows and max(rows[-1]) < 8:
        rows.pop(); bottom -= 1
    if not rows:
        return adv, 0, 0, 0, 0, []
    cols = list(zip(*rows))
    l0 = 0
    while l0 < len(cols) and max(cols[l0]) < 8:
        l0 += 1
    r0 = len(cols)
    while r0 > l0 and max(cols[r0 - 1]) < 8:
        r0 -= 1
    rows = [r[l0:r0] for r in rows]
    left += l0
    w, h = r0 - l0, len(rows)
    vals = [min(15, (v + 8) >> 4) for r in rows for v in r]
    return adv, w, h, left, -bottom, vals


def pack4(vals):
    out = []
    for i in range(0, len(vals), 2):
        hi = vals[i]
        lo = vals[i + 1] if i + 1 < len(vals) else 0
        out.append((hi << 4) | lo)
    return out


def main():
    ttf = find(TTF_CANDIDATES)
    os.makedirs(OUT_DIR, exist_ok=True)
    cps = [ord(c) for c in CHARS]
    c = []
    c.append("// WYGENEROWANE przez tools/gen_pl_fonts.py - nie edytowac recznie.\n"
             "// Polskie litery (18 znakow) dla Montserrat Medium %s px, 4 bpp; reszta znakow z fallback =\n"
             "// wbudowana lv_font_montserrat_N (ASCII rysuje sie identycznie jak bez tej czcionki).\n"
             % ", ".join(str(s) for s in SIZES))
    c.append('#include "ui/fonts/console_fonts_pl.h"\n')
    total = 0
    for size in SIZES:
        font = ImageFont.truetype(ttf, size, layout_engine=ImageFont.Layout.BASIC)
        lh, bl = lvgl_metrics(size)
        bitmap = []
        dsc = ["    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},  /* id 0 */"]
        for cp in cps:
            adv, w, h, ox, oy, vals = render_glyph(font, chr(cp))
            dsc.append("    {.bitmap_index = %d, .adv_w = %d, .box_w = %d, .box_h = %d, .ofs_x = %d, .ofs_y = %d},  /* U+%04X */"
                       % (len(bitmap), adv, w, h, ox, oy, cp))
            bitmap += pack4(vals)
        total += len(bitmap)
        n = "pl%d" % size
        c.append("\n/* ---------------------------------------------------------------- %d px */\n" % size)
        c.append("static LV_ATTRIBUTE_LARGE_CONST const uint8_t %s_bitmap[] = {\n" % n)
        for i in range(0, len(bitmap), 16):
            c.append("    " + ", ".join("0x%02x" % b for b in bitmap[i:i + 16]) + ",\n")
        c.append("};\n")
        c.append("static const lv_font_fmt_txt_glyph_dsc_t %s_glyph_dsc[] = {\n%s\n};\n" % (n, "\n".join(dsc)))
        c.append("static const uint16_t %s_unicode_list[] = { %s };\n"
                 % (n, ", ".join("0x%x" % (cp - cps[0]) for cp in cps)))
        c.append("static const lv_font_fmt_txt_cmap_t %s_cmaps[] = {\n"
                 "    { .range_start = 0x%x, .range_length = %d, .glyph_id_start = 1, .unicode_list = %s_unicode_list,\n"
                 "      .glyph_id_ofs_list = NULL, .list_length = %d, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY },\n};\n"
                 % (n, cps[0], cps[-1] - cps[0] + 1, n, len(cps)))
        c.append("static const lv_font_fmt_txt_dsc_t %s_dsc = {\n"
                 "    .glyph_bitmap = %s_bitmap, .glyph_dsc = %s_glyph_dsc, .cmaps = %s_cmaps, .kern_dsc = NULL,\n"
                 "    .kern_scale = 0, .cmap_num = 1, .bpp = %d, .kern_classes = 0, .bitmap_format = 0,\n};\n"
                 % (n, n, n, n, BPP))
        c.append("const lv_font_t console_font_pl_%d = {\n"
                 "    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt, .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,\n"
                 "    .line_height = %d, .base_line = %d, .subpx = LV_FONT_SUBPX_NONE, .underline_position = -1,\n"
                 "    .underline_thickness = 1, .dsc = &%s_dsc, .fallback = &lv_font_montserrat_%d,\n};\n"
                 % (size, lh, bl, n, size))
    with open(os.path.join(OUT_DIR, "console_fonts_pl.c"), "w", encoding="ascii", newline="\n") as f:
        f.write("".join(c))
    h = ["// WYGENEROWANE przez tools/gen_pl_fonts.py - czcionki z polskimi literami (fallback: Montserrat LVGL).\n",
         "#pragma once\n\n#include \"lvgl.h\"\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"]
    for size in SIZES:
        h.append("extern const lv_font_t console_font_pl_%d;\n" % size)
    h.append("\n#ifdef __cplusplus\n}\n#endif\n")
    with open(os.path.join(OUT_DIR, "console_fonts_pl.h"), "w", encoding="ascii", newline="\n") as f:
        f.write("".join(h))
    print("console_fonts_pl.c: %d rozmiarow, %d znakow, bitmapy %d B" % (len(SIZES), len(cps), total))


if __name__ == "__main__":
    main()
