#include "gfx/sprite.h"

#include <string.h>

#include "core/log.h"

namespace gfx {

namespace {
const char* TAG = "sprite";

uint16_t lookup(char key, const PaletteEntry* palette, int n)
{
    for (int i = 0; i < n; ++i) {
        if (palette[i].key == key) return palette[i].color;
    }
    LAKE_LOGW(TAG, "brak koloru '%c' w palecie", key);
    return TRANSPARENT;
}
}  // namespace

Sprite make_sprite(const char* const* rows, int w, int h, const PaletteEntry* palette, int palette_size)
{
    Sprite s;
    if (w <= 0 || h <= 0 || !rows) return s;

    uint16_t* px = new uint16_t[(size_t)w * h];
    if (!px) {
        LAKE_LOGE(TAG, "brak pamieci na sprite %dx%d", w, h);
        return s;
    }

    bool warned = false;
    for (int y = 0; y < h; ++y) {
        const char* row = rows[y] ? rows[y] : "";
        const int   len = (int)strlen(row);
        if (len != w && !warned) {
            LAKE_LOGW(TAG, "sprite %dx%d: wiersz %d ma %d znakow", w, h, y, len);
            warned = true;
        }
        for (int x = 0; x < w; ++x) {
            const char ch = x < len ? row[x] : '.';
            px[(size_t)y * w + x] = (ch == '.' || ch == ' ') ? TRANSPARENT : lookup(ch, palette, palette_size);
        }
    }
    s.w  = w;
    s.h  = h;
    s.px = px;
    return s;
}

}  // namespace gfx
