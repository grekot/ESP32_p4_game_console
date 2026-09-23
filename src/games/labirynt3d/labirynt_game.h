// Labirynt 3D - pseudo-3D metoda raycastingu (jak Wolfenstein 3D): swiat to mapa kafelkow z liter,
// dla kazdej kolumny ekranu puszczamy jeden promien i rysujemy pasek sciany o wysokosci zaleznej
// od odleglosci. Tekstury scian i przedmioty to PNG z assets/labirynt3d/ (fallback: kolory).
//
// Rysuje na plotnie 400x240 (canvas_scale() == 2): 400 promieni na klatke to koszt, ktory ESP32-P4
// udzwignie przy 60 FPS; w 800x480 bylyby 4x wieksze wypelnienie.
#pragma once

#include <stdint.h>

#include "engine/game.h"
#include "gfx/canvas.h"

namespace labirynt {

class LabiryntGame final : public engine::Game {
public:
    void init(gfx::Canvas& canvas) override;
    void update(float dt, const input::PadState& pad) override;
    void render(gfx::Canvas& canvas) override;
    int  canvas_scale() const override { return 2; }
    void debug_line(char* buf, size_t n) const override;

private:
    enum class State { Title, Playing, Won };

    struct Item {
        float x, y;        // srodek, w kafelkach
        bool  taken;
        bool  portal;      // true = wyjscie, false = moneta
    };

    static constexpr int W = 400, H = 240;
    static constexpr int MAX_ITEMS = 48;

    static constexpr int MAP_COLS = 32, MAP_ROWS = 24;
    char            tiles_[MAP_ROWS][MAP_COLS];   // wlasna tablica: engine::TileMap ma max 16 wierszy
    float           px_ = 0, py_ = 0;    // pozycja gracza w kafelkach
    float           dir_x_ = 1, dir_y_ = 0;
    float           plane_x_ = 0, plane_y_ = 0.66f;
    float           angle_ = 0;
    Item            items_[MAX_ITEMS]{};
    int             item_count_ = 0;
    int             coins_ = 0, coins_total_ = 0, doors_ = 0;
    float           time_ = 0, bob_ = 0, anim_ = 0;
    State           state_ = State::Title;
    float           zbuf_[W];
    uint16_t        ceil_row_[H], floor_row_[H];   // gradient sufitu/podlogi

    gfx::Sprite tex_[4];        // 0 cegla '#', 1 kamien '=', 2 drzwi 'D', 3 wyjscie (nieuzywane jako sciana)
    gfx::Sprite coin_, portal_;
    bool        assets_loaded_ = false;

    void new_game();
    void load_level();
    void set_angle(float a);
    char tile_at(int c, int r) const { return (c < 0 || r < 0 || c >= MAP_COLS || r >= MAP_ROWS) ? '#' : tiles_[r][c]; }
    bool solid_at(int c, int r) const;
    void try_move(float dx, float dy);
    void draw_walls(gfx::Canvas& c);
    void draw_sprites(gfx::Canvas& c);
    void draw_minimap(gfx::Canvas& c) const;
    void draw_hud(gfx::Canvas& c);
};

}  // namespace labirynt
