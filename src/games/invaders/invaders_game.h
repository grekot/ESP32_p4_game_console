// Space Invaders w nowoczesnej oprawie, plotno 800x480, grafika z Gemini (assets/invaders/, PNG z alfa).
//
// Formacja 5x10 obcych (kalmar 40 pkt, meduza 30, krab 20, osmiornica 10) przesuwa sie na boki i schodzi w dol
// na kazdej krawedzi, przyspiesza, gdy jej ubywa. Obcy strzelaja z dolu kolumn (od fali 3 czesc strzalow celuje
// w gracza), od fali 2 pojedynczy obcy nurkuja lukiem na gracza i wracaja na swoje miejsce. UFO przelatuje gora
// i zostawia bonus. Cztery krysztalowe bunkry kruszone pikselami przez pociski z obu stron. Co 5. fala = boss
// (pasek zycia, wachlarze pociskow, uszkodzony wyglad ponizej polowy). Bonusy: potrojny strzal, laser
// (przebijajace pociski, szybki ogien), oslona (pochlania trafienie), spowolnienie czasu, dodatkowe zycie.
// Combo: trafienia w odstepach < 0,7 s mnoza punkty (do x4). Cztery swiaty (tla) na zmiane co fale.
// Sterowanie: LEWO/PRAWO (albo galka), A = strzal (przytrzymanie = seria), Y = autopilot (demo i testy),
// X na tytule = fala startowa.
//
// Logika w invaders_game.cpp, rysowanie w invaders_render.cpp.
#pragma once

#include <stdint.h>

#include "engine/game.h"
#include "gfx/canvas.h"

namespace invaders {

// Obraz z alfa (RGB565 + krycie 0..255).
struct Pic {
    int             w = 0, h = 0;
    const uint16_t* px    = nullptr;
    const uint8_t*  alpha = nullptr;
};

class InvadersGame final : public engine::Game {
public:
    void init(gfx::Canvas& canvas) override;
    void update(float dt, const input::PadState& pad) override;
    void render(gfx::Canvas& canvas) override;
    void debug_line(char* buf, size_t n) const override;

    static constexpr int W = 800, H = 480;
    static constexpr int FCOLS = 10, FROWS = 5, ALIENS = FCOLS * FROWS;
    static constexpr int CELL_W = 60, CELL_H = 46, ALIEN_PX = 44;
    static constexpr int TYPES = 4, WORLDS = 4, POWERS = 5, BOOM_FRAMES = 4;
    static constexpr int BUNKERS = 4, BUNKER_W = 112, BUNKER_H = 56, BUNKER_Y = 350;
    static constexpr int SHIP_Y = 432;            // srodek statku gracza
    static constexpr int HUD_H = 34;
    static constexpr int INVADE_Y = 404;          // dolna krawedz obcego ponizej = inwazja (koniec gry)

    enum class State { Title, Ready, Playing, Dying, Clear, GameOver };
    enum Power : uint8_t { PW_TRIPLE, PW_LASER, PW_SHIELD, PW_SLOW, PW_LIFE };
    enum AlienMode : uint8_t { IN_FORMATION, DIVING, RETURNING };

private:
    struct Alien {
        bool      alive = false;
        uint8_t   type = 0;
        AlienMode mode = IN_FORMATION;
        float     x = 0, y = 0;         // srodek (dla formacji liczony z fx_/fy_)
        float     t = 0;                // czas nurkowania
        float     dive_x = 0, dive_amp = 0, dive_w = 0;
        bool      dive_shot = false;
        float     flash = 0;
    };
    struct Shot {                       // pocisk gracza albo obcych
        bool  alive = false;
        float x = 0, y = 0, vx = 0, vy = 0;
        bool  pierce = false;
        uint8_t kind = 0;               // obcy: 0 zwykly, 1 bossa
    };
    struct Boom { bool alive = false; float x = 0, y = 0, t = 0, dur = 0.45f, scale = 1.f; };
    struct Spark { bool alive = false; float x = 0, y = 0, vx = 0, vy = 0, life = 0, max = 0; uint16_t color = 0; };
    struct Popup { bool alive = false; float x = 0, y = 0, t = 0; char text[12]; uint16_t color = 0; };
    struct Pickup { bool alive = false; float x = 0, y = 0; uint8_t kind = 0; };
    struct Star { int16_t x, y; uint8_t layer, phase; };
    struct Record { int version; int score; int level; };

    static constexpr int MAX_BOLTS = 12, MAX_BOMBS = 24, MAX_BOOMS = 24, MAX_SPARKS = 220;
    static constexpr int MAX_POPUPS = 8, MAX_PICKUPS = 4, STARS = 90;

    // --- stan gry ---
    State state_ = State::Title;
    int   level_ = 1, start_level_ = 1, world_ = 0;
    bool  boss_level_ = false;
    Alien alien_[ALIENS];
    int   alive_ = 0, total_ = 0;
    float fx_ = 0, fy_ = 0;             // lewy gorny rog formacji
    float fdir_ = 1, fdrop_ = 0;        // kierunek, pozostale zejscie w dol [px]
    float fanim_t_ = 0;
    int   fframe_ = 0;
    float bomb_t_ = 0, dive_t_ = 0;

    float ship_x_ = W / 2;
    float fire_cd_ = 0;
    float invuln_ = 0;
    Shot  bolt_[MAX_BOLTS];
    Shot  bomb_[MAX_BOMBS];

    bool  ufo_alive_ = false;
    float ufo_x_ = 0, ufo_dir_ = 1, ufo_t_ = 0;

    bool  boss_alive_ = false;
    float boss_x_ = W / 2, boss_y_ = 120, boss_t_ = 0, boss_fire_t_ = 0, boss_flash_ = 0, boss_die_t_ = 0;
    int   boss_hp_ = 0, boss_max_ = 1;

    float pw_time_[POWERS]{};           // pozostaly czas bonusu (PW_SHIELD: > 0 = oslona aktywna)
    Pickup pickup_[MAX_PICKUPS];

    int   score_ = 0, lives_ = 3, next_life_ = 20000;
    int   combo_ = 0;
    float combo_t_ = 0;
    float state_t_ = 0, anim_ = 0, shake_ = 0, level_time_ = 0;
    bool  autopilot_ = false;
    int   kills_ = 0;
    Record top_{};

    Boom   boom_[MAX_BOOMS];
    Spark  spark_[MAX_SPARKS];
    Popup  popup_[MAX_POPUPS];
    Star   star_[STARS];

    uint8_t* bunker_mask_[BUNKERS]{};   // krycie pikseli bunkra (kopia alfy PNG, kruszona)

    // --- grafika ---
    bool      assets_loaded_ = false;
    int       bg_world_ = -1;
    uint16_t* bg_    = nullptr;         // tlo swiata 800x480
    uint16_t* title_ = nullptr;
    Pic alien_img_[TYPES][2]{};
    Pic ufo_img_{}, boss_img_[2]{}, life_img_{}, ship_img_{}, icon_ship_{}, bunker_img_{}, bomb_img_{}, bolt_img_{};
    Pic power_img_[POWERS]{};
    Pic boom_img_[BOOM_FRAMES]{};

    // logika (invaders_game.cpp)
    void start_game();
    void load_level();
    void reset_bunkers();
    void update_title(const input::PadState& pad);
    void update_playing(float dt, const input::PadState& pad);
    void update_formation(float dt, float slow);
    void update_dives(float dt, float slow);
    void update_shots(float dt, float slow);
    void update_ufo(float dt);
    void update_boss(float dt, float slow);
    void update_effects(float dt);
    void player_fire();
    void alien_fire(float slow);
    void start_dive();
    void kill_alien(int i);
    void hit_player();
    void spawn_pickup(float x, float y, int kind);
    void take_pickup(int kind);
    bool bunker_hit(float x, float y, int radius);
    void bunker_erase_rect(float x0, float y0, float x1, float y1);
    void explode(float x, float y, float scale, uint16_t color, int sparks);
    void add_score(int pts, float x, float y);
    void popup(float x, float y, const char* text, uint16_t color);
    void save_record();
    float formation_speed() const;
    float alien_x(int i) const;
    float alien_y(int i) const;
    int   autopilot_move();
    bool  bunker_above(float x) const;

    // rysowanie (invaders_render.cpp)
    void load_assets();
    void load_background();
    void draw_background(gfx::Canvas& c);
    void draw_bunkers(gfx::Canvas& c);
    void draw_aliens(gfx::Canvas& c, int ox, int oy);
    void draw_boss(gfx::Canvas& c, int ox, int oy);
    void draw_ufo(gfx::Canvas& c, int ox, int oy);
    void draw_ship(gfx::Canvas& c, int ox, int oy);
    void draw_shots(gfx::Canvas& c, int ox, int oy);
    void draw_effects(gfx::Canvas& c, int ox, int oy);
    void draw_hud(gfx::Canvas& c);
    void draw_title(gfx::Canvas& c);
    void draw_overlay(gfx::Canvas& c);
};

}  // namespace invaders
