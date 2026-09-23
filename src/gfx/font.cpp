#include "gfx/font.h"

#include <string.h>

namespace gfx {

namespace {

struct GlyphDef {
    char        ch;
    const char* rows[FONT_H];
};

// Definicje glifow jako ASCII-art: '#' = piksel zapalony. Czytelne i latwe do edycji.
const GlyphDef kGlyphs[] = {
    {'A', {" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'B', {"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "}},
    {'C', {" ####", "#    ", "#    ", "#    ", "#    ", "#    ", " ####"}},
    {'D', {"#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "}},
    {'E', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"}},
    {'F', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "}},
    {'G', {" ####", "#    ", "#    ", "# ###", "#   #", "#   #", " ####"}},
    {'H', {"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'I', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####"}},
    {'J', {"  ###", "   # ", "   # ", "   # ", "   # ", "#  # ", " ##  "}},
    {'K', {"#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"}},
    {'L', {"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}},
    {'M', {"#   #", "## ##", "# # #", "# # #", "#   #", "#   #", "#   #"}},
    {'N', {"#   #", "##  #", "# # #", "#  ##", "#   #", "#   #", "#   #"}},
    {'O', {" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'P', {"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "}},
    {'Q', {" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"}},
    {'R', {"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"}},
    {'S', {" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}},
    {'T', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'U', {"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'V', {"#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "}},
    {'W', {"#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #"}},
    {'X', {"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"}},
    {'Y', {"#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'Z', {"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"}},
    {'0', {" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "}},
    {'1', {"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {'2', {" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}},
    {'3', {"#####", "   # ", "  #  ", "   # ", "    #", "#   #", " ### "}},
    {'4', {"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "}},
    {'5', {"#####", "#    ", "#### ", "    #", "    #", "#   #", " ### "}},
    {'6', {"  ## ", " #   ", "#    ", "#### ", "#   #", "#   #", " ### "}},
    {'7', {"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "}},
    {'8', {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}},
    {'9', {" ### ", "#   #", "#   #", " ####", "    #", "   # ", " ##  "}},
    {':', {"     ", "  #  ", "  #  ", "     ", "  #  ", "  #  ", "     "}},
    {'-', {"     ", "     ", "     ", "#####", "     ", "     ", "     "}},
    {'.', {"     ", "     ", "     ", "     ", "     ", "  ## ", "  ## "}},
    {',', {"     ", "     ", "     ", "     ", "  ## ", "  ## ", " #   "}},
    {'!', {"  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "     ", "  #  "}},
    {'?', {" ### ", "#   #", "    #", "   # ", "  #  ", "     ", "  #  "}},
    {'<', {"   # ", "  #  ", " #   ", "#    ", " #   ", "  #  ", "   # "}},
    {'>', {" #   ", "  #  ", "   # ", "    #", "   # ", "  #  ", " #   "}},
    {'/', {"    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    "}},
    {'+', {"     ", "  #  ", "  #  ", "#####", "  #  ", "  #  ", "     "}},
    {'=', {"     ", "     ", "#####", "     ", "#####", "     ", "     "}},
    {'(', {"  #  ", " #   ", "#    ", "#    ", "#    ", " #   ", "  #  "}},
    {')', {"  #  ", "   # ", "    #", "    #", "    #", "   # ", "  #  "}},
    {'\'', {" ##  ", " ##  ", "  #  ", "     ", "     ", "     ", "     "}},
    {'%', {"##  #", "## # ", "  #  ", "  #  ", "  #  ", " # ##", "#  ##"}},
    {'*', {"     ", "# # #", " ### ", "#####", " ### ", "# # #", "     "}},
};

// Zbudowane bitmapy: bits[c][row], bit 4 = lewa kolumna
uint8_t s_bits[128][FONT_H];
bool    s_has[128];
bool    s_built = false;

void build()
{
    memset(s_bits, 0, sizeof(s_bits));
    memset(s_has, 0, sizeof(s_has));
    for (const GlyphDef& g : kGlyphs) {
        const unsigned char c = (unsigned char)g.ch;
        if (c >= 128) continue;
        for (int r = 0; r < FONT_H; ++r) {
            uint8_t bits = 0;
            const char* row = g.rows[r];
            for (int col = 0; col < FONT_W && row[col]; ++col) {
                if (row[col] == '#') bits |= (uint8_t)(1u << (FONT_W - 1 - col));
            }
            s_bits[c][r] = bits;
        }
        s_has[c] = true;
    }
    s_has[(unsigned char)' '] = true;
    s_built = true;
}

inline unsigned char normalize(char ch)
{
    unsigned char c = (unsigned char)ch;
    if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 'a' + 'A');
    if (c >= 128) c = '?';
    return c;
}

}  // namespace

void draw_text(Canvas& c, int x, int y, const char* text, uint16_t color, int scale)
{
    if (!s_built) build();
    if (!text || scale < 1) return;

    int cx = x;
    for (const char* p = text; *p; ++p) {
        const unsigned char ch = normalize(*p);
        if (s_has[ch]) {
            for (int r = 0; r < FONT_H; ++r) {
                const uint8_t bits = s_bits[ch][r];
                if (!bits) continue;
                for (int col = 0; col < FONT_W; ++col) {
                    if (bits & (1u << (FONT_W - 1 - col))) {
                        if (scale == 1) {
                            c.put(cx + col, y + r, color);
                        } else {
                            c.fill_rect(cx + col * scale, y + r * scale, scale, scale, color);
                        }
                    }
                }
            }
        }
        cx += FONT_ADVANCE * scale;
    }
}

void draw_text_shadow(Canvas& c, int x, int y, const char* text, uint16_t color, uint16_t shadow, int scale)
{
    draw_text(c, x + scale, y + scale, text, shadow, scale);
    draw_text(c, x, y, text, color, scale);
}

int text_width(const char* text, int scale)
{
    if (!text) return 0;
    return (int)strlen(text) * FONT_ADVANCE * scale - scale;
}

}  // namespace gfx
