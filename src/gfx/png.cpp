#include "gfx/png.h"

#include <stdlib.h>
#include <string.h>

#include "core/log.h"
#include "gfx/lodepng/lodepng.h"
#include "platform/platform.h"

namespace gfx {

namespace {
const char* TAG = "png";

// RGBA8888 -> RGB565 z kolorem-kluczem. Nieprzezroczysta magenta (255,0,255) jest przesuwana o 1 krok
// w niebieskim, zeby nie zniknela jako "dziura".
inline uint16_t to565(const uint8_t* p)
{
    if (p[3] < 128) return TRANSPARENT;
    uint16_t c = rgb565(p[0], p[1], p[2]);
    if (c == TRANSPARENT) c = rgb565(255, 0, 247);
    return c;
}
}  // namespace

bool png_size(const char* path, int& w, int& h)
{
    w = h = 0;
    size_t   size = 0;
    uint8_t* data = platform::read_file(path, size);
    if (!data) return false;
    unsigned uw = 0, uh = 0;
    LodePNGState state;
    lodepng_state_init(&state);
    const unsigned err = lodepng_inspect(&uw, &uh, &state, data, size);
    lodepng_state_cleanup(&state);
    platform::free_file(data);
    if (err) return false;
    w = (int)uw;
    h = (int)uh;
    return true;
}

Sprite load_png(const char* path, uint16_t* dst, size_t dst_capacity)
{
    Sprite s;
    size_t   size = 0;
    uint8_t* data = platform::read_file(path, size);
    if (!data) return s;   // brak pliku juz zalogowany przez platform

    unsigned char* rgba = nullptr;
    unsigned       w = 0, h = 0;
    const unsigned err = lodepng_decode32(&rgba, &w, &h, data, size);
    platform::free_file(data);
    if (err) {
        CONSOLE_LOGE(TAG, "%s: %s", path, lodepng_error_text(err));
        free(rgba);
        return s;
    }
    const size_t n = (size_t)w * h;
    if (n == 0 || w > 4096 || h > 4096) {
        CONSOLE_LOGE(TAG, "%s: zly rozmiar %ux%u", path, w, h);
        free(rgba);
        return s;
    }
    uint16_t* px = dst;
    if (px) {
        if (n > dst_capacity) {
            CONSOLE_LOGE(TAG, "%s: %ux%u = %u px nie miesci sie w buforze %u px", path, w, h, (unsigned)n,
                         (unsigned)dst_capacity);
            free(rgba);
            return s;
        }
    } else {
        px = new uint16_t[n];
    }
    for (size_t i = 0; i < n; ++i) {
        px[i] = to565(rgba + i * 4);
    }
    free(rgba);
    s.w  = (int)w;
    s.h  = (int)h;
    s.px = px;
    return s;
}

IndexedImage load_png_indexed(const char* path)
{
    IndexedImage img;
    size_t   size = 0;
    uint8_t* data = platform::read_file(path, size);
    if (!data) return img;
    LodePNGState state;
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_PALETTE;   // dekoduj do indeksow (bez rozwijania do RGBA)
    state.info_raw.bitdepth  = 8;
    unsigned char* out = nullptr;
    unsigned       w = 0, h = 0;
    const unsigned err = lodepng_decode(&out, &w, &h, &state, data, size);
    platform::free_file(data);
    if (err || state.info_png.color.colortype != LCT_PALETTE) {
        CONSOLE_LOGE(TAG, "%s: %s", path, err ? lodepng_error_text(err) : "PNG bez palety (zapisz jako 8-bit indeksowany)");
        free(out);
        lodepng_state_cleanup(&state);
        return img;
    }
    const size_t n = (size_t)w * h;
    uint8_t* buf = reinterpret_cast<uint8_t*>(platform::alloc_pixels((n + 1) / 2, /*fast=*/false));
    if (!buf) {
        CONSOLE_LOGE(TAG, "%s: brak pamieci na %u px", path, (unsigned)n);
        free(out);
        lodepng_state_cleanup(&state);
        return img;
    }
    memcpy(buf, out, n);
    free(out);
    const size_t np = state.info_png.color.palettesize;
    for (size_t i = 0; i < 256; ++i) {
        const unsigned char* p = state.info_png.color.palette + i * 4;
        img.palette[i] = i < np ? rgb565(p[0], p[1], p[2]) : 0;
    }
    lodepng_state_cleanup(&state);
    img.w   = (int)w;
    img.h   = (int)h;
    img.idx = buf;
    return img;
}

Image load_png_rgba(const char* path)
{
    Image    img;
    size_t   size = 0;
    uint8_t* data = platform::read_file(path, size);
    if (!data) return img;

    unsigned char* rgba = nullptr;
    unsigned       w = 0, h = 0;
    const unsigned err = lodepng_decode32(&rgba, &w, &h, data, size);
    platform::free_file(data);
    if (err) {
        CONSOLE_LOGE(TAG, "%s: %s", path, lodepng_error_text(err));
        free(rgba);
        return img;
    }
    const size_t n = (size_t)w * h;
    if (n == 0 || w > 4096 || h > 4096) {
        CONSOLE_LOGE(TAG, "%s: zly rozmiar %ux%u", path, w, h);
        free(rgba);
        return img;
    }
    // jeden blok: n pikseli RGB565 + n bajtow alfa (zaokraglone do pelnych pikseli)
    uint16_t* buf = platform::alloc_pixels(n + (n + 1) / 2, /*fast=*/false);
    if (!buf) {
        CONSOLE_LOGE(TAG, "%s: brak pamieci na %u px", path, (unsigned)n);
        free(rgba);
        return img;
    }
    uint8_t* alpha = reinterpret_cast<uint8_t*>(buf + n);
    for (size_t i = 0; i < n; ++i) {
        const uint8_t* p = rgba + i * 4;
        buf[i]   = rgb565(p[0], p[1], p[2]);
        alpha[i] = p[3];
    }
    free(rgba);
    img.w     = (int)w;
    img.h     = (int)h;
    img.px    = buf;
    img.alpha = alpha;
    return img;
}

}  // namespace gfx
