#include "games/mario/mario_assets.h"

#include "gfx/sprite.h"

namespace mario {

namespace spr {
gfx::Sprite player_idle, player_walk1, player_walk2, player_jump;
gfx::Sprite enemy_a, enemy_b, enemy_squash;
gfx::Sprite ground, dirt, brick, question, used;
gfx::Sprite pipe_tl, pipe_tr, pipe_l, pipe_r;
gfx::Sprite coin_a, coin_b;
gfx::Sprite pole, pole_top, flag;
gfx::Sprite cloud, bush;
}  // namespace spr

namespace {

using gfx::rgb565;

// Wspolna paleta wszystkich sprite'ow ('.' = przezroczysty)
const gfx::PaletteEntry PAL[] = {
    {'k', rgb565(0, 0, 0)},         // czern
    {'w', rgb565(255, 255, 255)},   // biel
    {'e', rgb565(165, 165, 175)},   // szary
    {'E', rgb565(85, 85, 95)},      // ciemny szary
    {'r', rgb565(225, 45, 45)},     // czerwien
    {'R', rgb565(145, 20, 20)},     // ciemna czerwien
    {'o', rgb565(240, 140, 40)},    // pomarancz
    {'y', rgb565(250, 215, 50)},    // zolty
    {'Y', rgb565(200, 150, 20)},    // ciemny zolty
    {'g', rgb565(70, 190, 70)},     // zielen
    {'G', rgb565(30, 120, 40)},     // ciemna zielen
    {'b', rgb565(155, 95, 40)},     // braz
    {'B', rgb565(85, 50, 20)},      // ciemny braz
    {'t', rgb565(205, 175, 125)},   // bezowy
    {'s', rgb565(255, 205, 160)},   // skora
    {'u', rgb565(50, 100, 220)},    // niebieski
    {'U', rgb565(30, 55, 140)},     // ciemny niebieski
    {'p', rgb565(170, 80, 200)},    // fiolet
    {'P', rgb565(100, 40, 130)},    // ciemny fiolet
};
constexpr int PAL_N = sizeof(PAL) / sizeof(PAL[0]);

gfx::Sprite S16(const char* const (&rows)[16])
{
    return gfx::make_sprite(rows, 16, 16, PAL, PAL_N);
}

// ---------------------------------------------------------------- bohater (hitbox: kolumny 2..13)
const char* const PLAYER_IDLE[16] = {
    "......uuuuuu....",
    ".....uuuuuuuuu..",
    ".....uuuuuuuuuu.",
    ".....BBBsssks...",
    "....BsBsssskss..",
    "....BsBBssssss..",
    "....BBsssskkk...",
    "......ssssss....",
    ".....ooouooo....",
    "....oooouuoooo..",
    "...oooouuuuoooo.",
    "...ssuuuuuuuuss.",
    "...ssuuuyyuuuss.",
    ".....uuuu.uuuu..",
    "....BBBB..BBBB..",
    "...BBBBB..BBBBB.",
};
const char* const PLAYER_WALK1[16] = {
    "......uuuuuu....",
    ".....uuuuuuuuu..",
    ".....uuuuuuuuuu.",
    ".....BBBsssks...",
    "....BsBsssskss..",
    "....BsBBssssss..",
    "....BBsssskkk...",
    "......ssssss....",
    ".....ooouooo....",
    "....oooouuoooo..",
    "...oooouuuuoooo.",
    "...ssuuuuuuuuss.",
    "...ssuuuyyuuuss.",
    ".....uuuu.uuuu..",
    "...BBBB....BBBB.",
    "..BBBB......BBBB",
};
const char* const PLAYER_WALK2[16] = {
    "......uuuuuu....",
    ".....uuuuuuuuu..",
    ".....uuuuuuuuuu.",
    ".....BBBsssks...",
    "....BsBsssskss..",
    "....BsBBssssss..",
    "....BBsssskkk...",
    "......ssssss....",
    ".....ooouooo....",
    "....oooouuoooo..",
    "...oooouuuuoooo.",
    "...ssuuuuuuuuss.",
    "....suuuyyuuus..",
    "......uuuuuu....",
    "......BBBBBB....",
    ".....BBBBBBBB...",
};
const char* const PLAYER_JUMP[16] = {
    "......uuuuuu....",
    ".....uuuuuuuuu..",
    ".....uuuuuuuuuu.",
    ".....BBBsssks...",
    "....BsBsssskss..",
    "....BsBBssssss..",
    "....BBsssskkk...",
    "......ssssss....",
    "...s.ooouooo.s..",
    "...ssoooouuooss.",
    "...uuoooouuuuuu.",
    "....uuuuuuuuuu..",
    "....uuuuyyuuuu..",
    ".....uuuu.uuuu..",
    "....BBBB..BBBB..",
    "................",
};

// ---------------------------------------------------------------- przeciwnik: "glutek"
const char* const ENEMY_A[16] = {
    "................",
    "................",
    ".....pppppp.....",
    "...pppppppppp...",
    "..pppppppppppp..",
    ".pppwwppppwwppp.",
    ".ppwkwppppwkwpp.",
    ".pppwwppppwwppp.",
    ".pppppppppppppp.",
    ".pppppppppppppp.",
    ".ppPPppppppPPpp.",
    "..ppPPPPPPPPpp..",
    "...pppppppppp...",
    "..PP..PPPP..PP..",
    ".PPP..PPPP..PPP.",
    "................",
};
const char* const ENEMY_B[16] = {
    "................",
    "................",
    ".....pppppp.....",
    "...pppppppppp...",
    "..pppppppppppp..",
    ".pppwwppppwwppp.",
    ".pppwkppppwkppp.",
    ".pppwwppppwwppp.",
    ".pppppppppppppp.",
    ".pppppppppppppp.",
    ".ppPPppppppPPpp.",
    "..ppPPPPPPPPpp..",
    "...pppppppppp...",
    "....PPP..PPP....",
    "...PPPP..PPPP...",
    "................",
};
const char* const ENEMY_SQUASH[16] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "...pppppppppp...",
    ".ppppwwppppwwpp.",
    "ppppwkwppppwkwpp",
    "PPPPPPPPPPPPPPPP",
    "................",
};

// ---------------------------------------------------------------- kafelki
const char* const GROUND[16] = {
    "gggggggggggggggg",
    "gGggggGgggggGggg",
    "bbbbbbbbbbbbbbbb",
    "bbBbbbbbbbbBbbbb",
    "bbbbbbbbbbbbbbbb",
    "bbbbbbBbbbbbbbbb",
    "bBbbbbbbbbbbbbBb",
    "bbbbbbbbbbBbbbbb",
    "bbbbBbbbbbbbbbbb",
    "bbbbbbbbbbbbbbbb",
    "bbbbbbbbBbbbbbbb",
    "bBbbbbbbbbbbbBbb",
    "bbbbbbbbbbbbbbbb",
    "bbbbbBbbbbbbbbbb",
    "bbbbbbbbbbbBbbbb",
    "BBBBBBBBBBBBBBBB",
};
const char* const DIRT[16] = {
    "bbbbbbbbbbbbbbbb",
    "bbBbbbbbbbbBbbbb",
    "bbbbbbbbbbbbbbbb",
    "bbbbbbBbbbbbbbbb",
    "bBbbbbbbbbbbbbBb",
    "bbbbbbbbbbBbbbbb",
    "bbbbBbbbbbbbbbbb",
    "bbbbbbbbbbbbbbbb",
    "bbbbbbbbBbbbbbbb",
    "bBbbbbbbbbbbbBbb",
    "bbbbbbbbbbbbbbbb",
    "bbbbbBbbbbbbbbbb",
    "bbbbbbbbbbbBbbbb",
    "bbBbbbbbbbbbbbbb",
    "bbbbbbbbBbbbbbbb",
    "BBBBBBBBBBBBBBBB",
};
const char* const BRICK[16] = {
    "bbbbbbbBbbbbbbbb",
    "bbbbbbbBbbbbbbbb",
    "bbbbbbbBbbbbbbbb",
    "BBBBBBBBBBBBBBBB",
    "bbbBbbbbbbbBbbbb",
    "bbbBbbbbbbbBbbbb",
    "bbbBbbbbbbbBbbbb",
    "BBBBBBBBBBBBBBBB",
    "bbbbbbbBbbbbbbbb",
    "bbbbbbbBbbbbbbbb",
    "bbbbbbbBbbbbbbbb",
    "BBBBBBBBBBBBBBBB",
    "bbbBbbbbbbbBbbbb",
    "bbbBbbbbbbbBbbbb",
    "bbbBbbbbbbbBbbbb",
    "BBBBBBBBBBBBBBBB",
};
const char* const QUESTION[16] = {
    "YYYYYYYYYYYYYYYY",
    "YyyyyyyyyyyyyyyY",
    "YyYyyyyyyyyyyYyY",
    "YyyyyykkkkyyyyyY",
    "YyyyykkyykkyyyyY",
    "YyyyykkyykkyyyyY",
    "YyyyyyyyykkyyyyY",
    "YyyyyyyykkyyyyyY",
    "YyyyyyykkyyyyyyY",
    "YyyyyyykkyyyyyyY",
    "YyyyyyyyyyyyyyyY",
    "YyyyyyykkyyyyyyY",
    "YyyyyyykkyyyyyyY",
    "YyYyyyyyyyyyyYyY",
    "YyyyyyyyyyyyyyyY",
    "YYYYYYYYYYYYYYYY",
};
const char* const USED[16] = {
    "BBBBBBBBBBBBBBBB",
    "BttttttttttttttB",
    "BtBttttttttttBtB",
    "BttttttttttttttB",
    "BttttttttttttttB",
    "BttttttttttttttB",
    "BttttttttttttttB",
    "BttttttttttttttB",
    "BttttttttttttttB",
    "BttttttttttttttB",
    "BttttttttttttttB",
    "BttttttttttttttB",
    "BttttttttttttttB",
    "BtBttttttttttBtB",
    "BttttttttttttttB",
    "BBBBBBBBBBBBBBBB",
};
const char* const PIPE_TL[16] = {
    "GGGGGGGGGGGGGGGG",
    "Gwwggggggggggggg",
    "Gwwggggggggggggg",
    "Gwwggggggggggggg",
    "Gwwggggggggggggg",
    "Gwwggggggggggggg",
    "Gwwggggggggggggg",
    "GGGGGGGGGGGGGGGG",
    "..Gwwggggggggggg",
    "..Gwwggggggggggg",
    "..Gwwggggggggggg",
    "..Gwwggggggggggg",
    "..Gwwggggggggggg",
    "..Gwwggggggggggg",
    "..Gwwggggggggggg",
    "..Gwwggggggggggg",
};
const char* const PIPE_TR[16] = {
    "GGGGGGGGGGGGGGGG",
    "gggggggggggggGGG",
    "gggggggggggggGGG",
    "gggggggggggggGGG",
    "gggggggggggggGGG",
    "gggggggggggggGGG",
    "gggggggggggggGGG",
    "GGGGGGGGGGGGGGGG",
    "gggggggggggGGG..",
    "gggggggggggGGG..",
    "gggggggggggGGG..",
    "gggggggggggGGG..",
    "gggggggggggGGG..",
    "gggggggggggGGG..",
    "gggggggggggGGG..",
    "gggggggggggGGG..",
};
const char* const PIPE_L[16] = {
    "..Gwwggggggggggg", "..Gwwggggggggggg", "..Gwwggggggggggg", "..Gwwggggggggggg",
    "..Gwwggggggggggg", "..Gwwggggggggggg", "..Gwwggggggggggg", "..Gwwggggggggggg",
    "..Gwwggggggggggg", "..Gwwggggggggggg", "..Gwwggggggggggg", "..Gwwggggggggggg",
    "..Gwwggggggggggg", "..Gwwggggggggggg", "..Gwwggggggggggg", "..Gwwggggggggggg",
};
const char* const PIPE_R[16] = {
    "gggggggggggGGG..", "gggggggggggGGG..", "gggggggggggGGG..", "gggggggggggGGG..",
    "gggggggggggGGG..", "gggggggggggGGG..", "gggggggggggGGG..", "gggggggggggGGG..",
    "gggggggggggGGG..", "gggggggggggGGG..", "gggggggggggGGG..", "gggggggggggGGG..",
    "gggggggggggGGG..", "gggggggggggGGG..", "gggggggggggGGG..", "gggggggggggGGG..",
};
const char* const COIN_A[16] = {
    "................",
    "................",
    "......yyyy......",
    ".....yYYYYy.....",
    "....yYwwYYYy....",
    "....yYwYYYYy....",
    "....yYwYYYYy....",
    "....yYwYYYYy....",
    "....yYwYYYYy....",
    "....yYwYYYYy....",
    "....yYwYYYYy....",
    "....yYwwYYYy....",
    ".....yYYYYy.....",
    "......yyyy......",
    "................",
    "................",
};
const char* const COIN_B[16] = {
    "................",
    "................",
    ".......yy.......",
    "......yYYy......",
    "......yYYy......",
    "......ywYy......",
    "......ywYy......",
    "......ywYy......",
    "......ywYy......",
    "......ywYy......",
    "......ywYy......",
    "......yYYy......",
    "......yYYy......",
    ".......yy.......",
    "................",
    "................",
};
const char* const POLE[16] = {
    ".......eE.......", ".......eE.......", ".......eE.......", ".......eE.......",
    ".......eE.......", ".......eE.......", ".......eE.......", ".......eE.......",
    ".......eE.......", ".......eE.......", ".......eE.......", ".......eE.......",
    ".......eE.......", ".......eE.......", ".......eE.......", ".......eE.......",
};
const char* const POLE_TOP[16] = {
    "......gggg......",
    ".....gggggg.....",
    ".....ggGGgg.....",
    "......gggg......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
    ".......eE.......",
};
// flaga wisi na lewo od masztu (kafelek 'f' stoi w kolumnie przed 'F')
const char* const FLAG[16] = {
    "................",
    "........rrrrrrrr",
    "......rrrrrrrrrr",
    "....rrrrrrrrrrrr",
    "..rrrrrrwwrrrrrr",
    "..rrrrrrwwrrrrrr",
    "....rrrrrrrrrrrr",
    "......rrrrrrrrrr",
    "........rrrrrrrr",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
};
const char* const CLOUD[16] = {
    "................",
    "................",
    "......wwww......",
    "....wwwwwwww....",
    "...wwwwwwwwww...",
    "..wwwwwwwwwwww..",
    ".wwwwwwwwwwwwww.",
    ".wwwwwwwwwwwwww.",
    "..eeeeeeeeeeee..",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
};
const char* const BUSH[16] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "......gggg......",
    "....gggggggg....",
    "...ggGgggggGgg..",
    "..gggggggggggg..",
    ".ggGgggggggGggg.",
    ".ggggggggggggggg",
    "GGGGGGGGGGGGGGGG",
    "................",
};

}  // namespace

void load_assets()
{
    static bool loaded = false;
    if (loaded) return;
    loaded = true;

    spr::player_idle  = S16(PLAYER_IDLE);
    spr::player_walk1 = S16(PLAYER_WALK1);
    spr::player_walk2 = S16(PLAYER_WALK2);
    spr::player_jump  = S16(PLAYER_JUMP);
    spr::enemy_a      = S16(ENEMY_A);
    spr::enemy_b      = S16(ENEMY_B);
    spr::enemy_squash = S16(ENEMY_SQUASH);
    spr::ground       = S16(GROUND);
    spr::dirt         = S16(DIRT);
    spr::brick        = S16(BRICK);
    spr::question     = S16(QUESTION);
    spr::used         = S16(USED);
    spr::pipe_tl      = S16(PIPE_TL);
    spr::pipe_tr      = S16(PIPE_TR);
    spr::pipe_l       = S16(PIPE_L);
    spr::pipe_r       = S16(PIPE_R);
    spr::coin_a       = S16(COIN_A);
    spr::coin_b       = S16(COIN_B);
    spr::pole         = S16(POLE);
    spr::pole_top     = S16(POLE_TOP);
    spr::flag         = S16(FLAG);
    spr::cloud        = S16(CLOUD);
    spr::bush         = S16(BUSH);
}

const gfx::Sprite* tile_sprite(char tile, int anim_frame)
{
    switch (tile) {
        case '#': return &spr::ground;
        case '=': return &spr::dirt;
        case 'B': return &spr::brick;
        case '?': return &spr::question;
        case 'U': return &spr::used;
        case '[': return &spr::pipe_tl;
        case ']': return &spr::pipe_tr;
        case '{': return &spr::pipe_l;
        case '}': return &spr::pipe_r;
        case 'o': return (anim_frame & 1) ? &spr::coin_b : &spr::coin_a;
        case 'F': return &spr::pole;
        case '^': return &spr::pole_top;
        case 'f': return &spr::flag;
        case 'C': return &spr::cloud;
        case 'h': return &spr::bush;
        default:  return nullptr;
    }
}

bool tile_is_solid(char tile)
{
    switch (tile) {
        case '#': case '=': case 'B': case '?': case 'U':
        case '[': case ']': case '{': case '}':
            return true;
        default:
            return false;
    }
}

}  // namespace mario
