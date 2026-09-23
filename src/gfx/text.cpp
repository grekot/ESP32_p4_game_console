#include "gfx/text.h"

#include <string.h>

#include "lvgl.h"

namespace gfx {

namespace {

struct FontEntry {
    int              px;
    const lv_font_t* font;
};

// Rozmiary wlaczone w lv_conf.h. Kolejnosc rosnaca - wybieramy najblizszy.
const FontEntry FONTS[] = {
    { 12, &lv_font_montserrat_12 }, { 14, &lv_font_montserrat_14 }, { 16, &lv_font_montserrat_16 },
    { 20, &lv_font_montserrat_20 }, { 24, &lv_font_montserrat_24 }, { 28, &lv_font_montserrat_28 },
    { 32, &lv_font_montserrat_32 }, { 40, &lv_font_montserrat_40 }, { 48, &lv_font_montserrat_48 },
};
constexpr int FONT_N = (int)(sizeof(FONTS) / sizeof(FONTS[0]));

const FontEntry& pick(int px)
{
    int best = 0;
    for (int i = 1; i < FONT_N; ++i) {
        const int d  = FONTS[i].px - px;
        const int db = FONTS[best].px - px;
        if ((d < 0 ? -d : d) < (db < 0 ? -db : db)) best = i;
    }
    return FONTS[best];
}

// Dekoder UTF-8: zwraca kod znaku i przesuwa wskaznik. Bledne sekwencje -> '?'.
uint32_t next_codepoint(const char*& p)
{
    const unsigned char c0 = (unsigned char)*p;
    if (c0 == 0) return 0;
    ++p;
    if (c0 < 0x80) return c0;
    int      extra = 0;
    uint32_t cp    = 0;
    if ((c0 & 0xE0) == 0xC0)      { extra = 1; cp = c0 & 0x1F; }
    else if ((c0 & 0xF0) == 0xE0) { extra = 2; cp = c0 & 0x0F; }
    else if ((c0 & 0xF8) == 0xF0) { extra = 3; cp = c0 & 0x07; }
    else return '?';
    for (int i = 0; i < extra; ++i) {
        const unsigned char c = (unsigned char)*p;
        if ((c & 0xC0) != 0x80) return '?';
        cp = (cp << 6) | (c & 0x3F);
        ++p;
    }
    return cp;
}

// Bufor na jeden glif w A8. Najwiekszy glif Montserrat 48 to ok. 45x50 px.
constexpr int GLYPH_BUF_W = 96;
constexpr int GLYPH_BUF_H = 96;
uint8_t s_glyph_px[GLYPH_BUF_W * GLYPH_BUF_H];

inline uint16_t blend565(uint16_t dst, uint16_t src, uint32_t a)
{
    if (a >= 255) return src;
    if (a == 0) return dst;
    const uint32_t ia = 255 - a;
    const uint32_t r = (((src >> 11) & 0x1F) * a + ((dst >> 11) & 0x1F) * ia) / 255;
    const uint32_t g = (((src >> 5) & 0x3F) * a + ((dst >> 5) & 0x3F) * ia) / 255;
    const uint32_t b = ((src & 0x1F) * a + (dst & 0x1F) * ia) / 255;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

void draw_glyph(Canvas& c, int x, int y, lv_font_glyph_dsc_t& g, uint16_t color)
{
    if (g.box_w <= 0 || g.box_h <= 0) return;
    if (g.box_w > GLYPH_BUF_W || g.box_h > GLYPH_BUF_H) return;

    // lv_draw_buf_init ustawia tez domyslne handlers - bez nich LVGL zatrzymuje sie na asercji.
    lv_draw_buf_t  buf;
    const uint32_t stride32 = lv_draw_buf_width_to_stride(g.box_w, LV_COLOR_FORMAT_A8);
    if (lv_draw_buf_init(&buf, g.box_w, g.box_h, LV_COLOR_FORMAT_A8, stride32, s_glyph_px, sizeof(s_glyph_px)) !=
        LV_RESULT_OK) {
        return;
    }
    // Zwraca wskaznik na lv_draw_buf_t (dla czcionek wbudowanych: nasz `buf`), piksele A8 sa w ->data.
    const lv_draw_buf_t* out = static_cast<const lv_draw_buf_t*>(lv_font_get_glyph_bitmap(&g, &buf));
    if (!out || !out->data) return;
    const uint8_t* a8     = out->data;
    const int      stride = (int)out->header.stride;

    const int W = c.width(), H = c.height();
    uint16_t* px = c.data();
    for (int row = 0; row < g.box_h; ++row) {
        const int yy = y + row;
        if (yy < 0 || yy >= H) continue;
        const uint8_t* arow = a8 + (size_t)row * stride;
        uint16_t*      drow = px + (size_t)yy * W;
        for (int col = 0; col < g.box_w; ++col) {
            const int xx = x + col;
            if (xx < 0 || xx >= W) continue;
            const uint32_t a = arow[col];
            if (a) drow[xx] = blend565(drow[xx], color, a);
        }
    }
}

}  // namespace

int text_font_px(int px) { return pick(px).px; }

int text_height_px(int px) { return (int)lv_font_get_line_height(pick(px).font); }

int text_width_px(const char* utf8, int px)
{
    if (!utf8) return 0;
    const lv_font_t* font = pick(px).font;
    const char* p  = utf8;
    uint32_t    cp = next_codepoint(p);
    int         w  = 0;
    while (cp) {
        const char* peek = p;
        const uint32_t next = next_codepoint(peek);
        lv_font_glyph_dsc_t g;
        if (lv_font_get_glyph_dsc(font, &g, cp, next)) w += g.adv_w;
        cp = next;
        p  = peek;
    }
    return w;
}

int draw_text_px(Canvas& c, int x, int y, const char* utf8, uint16_t color, int px)
{
    if (!utf8) return 0;
    const lv_font_t* font = pick(px).font;
    const int baseline_y = y + (int)font->line_height - (int)font->base_line;

    const char* p  = utf8;
    uint32_t    cp = next_codepoint(p);
    int         cx = x;
    while (cp) {
        const char* peek = p;
        const uint32_t next = next_codepoint(peek);
        lv_font_glyph_dsc_t g;
        if (lv_font_get_glyph_dsc(font, &g, cp, next)) {
            // Uklad jak w lv_draw_label: glif rysowany od lewej krawedzi + ofs_x, nad linia bazowa.
            draw_glyph(c, cx + g.ofs_x, baseline_y - g.box_h - g.ofs_y, g, color);
            cx += g.adv_w;
        }
        cp = next;
        p  = peek;
    }
    return cx - x;
}

}  // namespace gfx
