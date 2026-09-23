// Mapa kafelkow ze znakow ASCII + kolizje prostokatow z kafelkami + rysowanie z kamera.
// Wyciete z Lake Mario 1:1 (semantyka brzegow i kolejnosc operacji zachowane), zeby kolejne gry
// (i lekcja 12) dostaly platformowke "za darmo".
//
// Uklad: tile_at(col, row); (0,0) to lewy gorny kafelek; kolumny poza mapa sa SCIANA (krance poziomu),
// wiersze poza mapa sa PUSTE (nad ekranem i pod nim). Ktory znak jest staly, mowi funkcja solid_fn.
#pragma once

#include "gfx/canvas.h"

namespace engine {

class TileMap {
public:
    static constexpr int MAX_ROWS = 16;
    static constexpr int MAX_COLS = 256;

    using SolidFn  = bool (*)(char tile);
    using SpriteFn = const gfx::Sprite* (*)(char tile, int anim_frame);   // nullptr = nie rysuj

    // Ustawia rozmiar (przycinany do MAX_*) i wypelnia znakiem fill. tile_px = rozmiar kafelka w pikselach.
    void reset(int cols, int rows, int tile_px = 16, char fill = ' ');
    void set_solid_fn(SolidFn fn) { solid_fn_ = fn; }

    int cols() const      { return cols_; }
    int rows() const      { return rows_; }
    int tile_size() const { return tile_; }
    int width_px() const  { return cols_ * tile_; }
    int height_px() const { return rows_ * tile_; }

    char tile_at(int col, int row) const;         // ' ' poza mapa
    void set_tile(int col, int row, char tile);   // poza mapa: nic
    bool solid_at(int col, int row) const;        // kolumny poza mapa = true, wiersze poza = false

    // Numer kolumny/wiersza dla wspolrzednej w pikselach (floor, takze dla ujemnych).
    int col_of(float x) const;
    int row_of(float y) const;

    // Ruch w poziomie prostokata (x, y, w, h) o vx*dt z zatrzymaniem na scianie (vx = 0, hit_wall = true).
    void move_x(float& x, float y, float& vx, int w, int h, float dt, bool& hit_wall) const;

    // Ruch w pionie o vy*dt. Zwraca true, gdy prostokat stoi na podlozu po ruchu (sprawdzane pod dolna
    // krawedzia y + h). hit_col/hit_row: kafelek uderzony glowa przy ruchu w gore (najblizszy srodka), albo -1.
    bool move_y(float x, float& y, float& vy, int w, int h, float dt, int& hit_col, int& hit_row) const;

    // Rysuje widoczne kolumny (cam_x w px) sprite'ami z sprite_fn(tile, anim_frame).
    void draw(gfx::Canvas& c, int cam_x, SpriteFn sprite_fn, int anim_frame) const;

private:
    char    tiles_[MAX_ROWS][MAX_COLS] = {};
    int     cols_     = 0;
    int     rows_     = 0;
    int     tile_     = 16;
    SolidFn solid_fn_ = nullptr;
};

}  // namespace engine
