// Grafika gry Lake Mario - sprite'y 16x16 budowane z ASCII-artu (mario_assets.cpp).
#pragma once

#include "gfx/canvas.h"
#include "gfx/palette.h"

namespace mario {

constexpr int TILE = 16;

// Kolor nieba (ze wspolnej palety konsoli)
constexpr uint16_t SKY = gfx::pal::SKY;

namespace spr {
extern gfx::Sprite player_idle, player_walk1, player_walk2, player_jump;
extern gfx::Sprite enemy_a, enemy_b, enemy_squash;
extern gfx::Sprite ground, dirt, brick, question, used;
extern gfx::Sprite pipe_tl, pipe_tr, pipe_l, pipe_r;
extern gfx::Sprite coin_a, coin_b;
extern gfx::Sprite pole, pole_top, flag;
extern gfx::Sprite cloud, bush;
}  // namespace spr

// Buduje wszystkie sprite'y (wolac raz).
void load_assets();

// Sprite dla znaku kafelka z mapy poziomu (nullptr = nic nie rysuj). anim_frame: 0/1 dla monet.
const gfx::Sprite* tile_sprite(char tile, int anim_frame);

// Czy kafelek jest "staly" (kolizje)
bool tile_is_solid(char tile);

}  // namespace mario
