#include "engine/tilemap.h"

#include "engine/math2d.h"

namespace engine {

void TileMap::reset(int cols, int rows, int tile_px, char fill)
{
    cols_ = cols < 0 ? 0 : (cols > MAX_COLS ? MAX_COLS : cols);
    rows_ = rows < 0 ? 0 : (rows > MAX_ROWS ? MAX_ROWS : rows);
    tile_ = tile_px > 0 ? tile_px : 16;
    for (int r = 0; r < MAX_ROWS; ++r) {
        for (int c = 0; c < MAX_COLS; ++c) {
            tiles_[r][c] = fill;
        }
    }
}

char TileMap::tile_at(int col, int row) const
{
    if (col < 0 || col >= cols_ || row < 0 || row >= rows_) return ' ';
    return tiles_[row][col];
}

void TileMap::set_tile(int col, int row, char tile)
{
    if (col < 0 || col >= cols_ || row < 0 || row >= rows_) return;
    tiles_[row][col] = tile;
}

bool TileMap::solid_at(int col, int row) const
{
    if (col < 0 || col >= cols_) return true;    // sciany na krancach poziomu
    if (row < 0 || row >= rows_) return false;   // nad ekranem i pod nim - pusto
    return solid_fn_ ? solid_fn_(tiles_[row][col]) : false;
}

int TileMap::col_of(float x) const { return tile_of(x, tile_); }
int TileMap::row_of(float y) const { return tile_of(y, tile_); }

void TileMap::move_x(float& x, float y, float& vx, int w, int h, float dt, bool& hit_wall) const
{
    hit_wall = false;
    x += vx * dt;
    const int r0 = row_of(y), r1 = row_of(y + (float)h - 1);
    if (vx > 0) {
        const int col = col_of(x + (float)w - 1);
        for (int r = r0; r <= r1; ++r) {
            if (solid_at(col, r)) { x = (float)(col * tile_ - w); vx = 0; hit_wall = true; break; }
        }
    } else if (vx < 0) {
        const int col = col_of(x);
        for (int r = r0; r <= r1; ++r) {
            if (solid_at(col, r)) { x = (float)((col + 1) * tile_); vx = 0; hit_wall = true; break; }
        }
    }
}

bool TileMap::move_y(float x, float& y, float& vy, int w, int h, float dt, int& hit_col, int& hit_row) const
{
    hit_col = hit_row = -1;
    y += vy * dt;
    const int c0 = col_of(x), c1 = col_of(x + (float)w - 1);
    if (vy > 0) {
        // Kafelek pod DOLNA KRAWEDZIA (y + h), nie pod ostatnim pikselem (y + h - 1): po przyciagnieciu
        // do gory kafelka stopy stoja dokladnie na jego krawedzi i wersja "y + h - 1" trafiala w pusty
        // wiersz powyzej, dajac migotanie "stoi / w powietrzu" co 2-3 klatki.
        const int row = row_of(y + (float)h);
        for (int c = c0; c <= c1; ++c) {
            if (solid_at(c, row)) { y = (float)(row * tile_ - h); vy = 0; return true; }
        }
        return false;
    }
    if (vy < 0) {
        const int row = row_of(y);
        // kafelek najblizszy srodka obiektu - zeby uderzac w "ten" blok, w ktory sie celuje
        const float cx = x + (float)w * 0.5f;
        float best = 1e9f;
        for (int c = c0; c <= c1; ++c) {
            if (solid_at(c, row)) {
                const float d = absf((float)(c * tile_ + tile_ / 2) - cx);
                if (d < best) { best = d; hit_col = c; hit_row = row; }
            }
        }
        if (hit_col >= 0) { y = (float)((row + 1) * tile_); vy = 0; }
        return false;
    }
    // vy == 0: czy nadal stoimy
    const int row = row_of(y + (float)h);
    for (int c = c0; c <= c1; ++c) {
        if (solid_at(c, row)) return true;
    }
    return false;
}

void TileMap::draw(gfx::Canvas& c, int cam_x, SpriteFn sprite_fn, int anim_frame) const
{
    if (!sprite_fn) return;
    const int c0 = cam_x / tile_;
    const int c1 = c0 + c.width() / tile_ + 1;
    for (int r = 0; r < rows_; ++r) {
        for (int col = c0; col <= c1; ++col) {
            const gfx::Sprite* s = sprite_fn(tile_at(col, r), anim_frame);
            if (s) c.blit(*s, col * tile_ - cam_x, r * tile_);
        }
    }
}

}  // namespace engine
