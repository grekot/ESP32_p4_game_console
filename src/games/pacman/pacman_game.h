// Pacman - gra w labiryncie na plotnie 800x480 z grafika z Gemini (assets/pacman/, PNG z alfa).
//
// Labirynt 27x19 kratek po 24 px (648x456) po lewej, panel z punktami po prawej. Cztery plansze
// (pacman_mazes.h), cztery swiaty (tlo + kolor scian: neon, cukierki, dzungla, lawa) - poziom n gra na planszy
// (n-1) % 4. Zasady jak w oryginale: kulki (10 pkt), duze kulki (50 pkt) na kilka sekund odwracaja role - duchy
// uciekaja i mozna je zjesc (200/400/800/1600 w jednym ciagu), zjedzony duch wraca do domu jako same oczy.
// Cztery duchy z roznymi celami (czerwony goni, rozowy zachodzi 4 kratki przed gracza, blekitny liczy wektor
// od czerwonego, pomaranczowy goni z daleka i ucieka z bliska), tryby rozproszenia/poscigu na zmiane wg zegara.
// Owoc pojawia sie po 70 i 170 zjedzonych kulkach. Tunel w wierszu z otwartymi krawedziami zawija.
// Sterowanie: krzyzak (kierunek mozna wcisnac wczesniej - skret przy najblizszym skrzyzowaniu), Y = autopilot
// (demo i testy skryptowe), X na tytule = poziom startowy.
//
// Logika w pacman_game.cpp (ruch po kratkach, AI duchow, punkty, autopilot), rysowanie w pacman_render.cpp.
#pragma once

#include <stdint.h>

#include "engine/game.h"
#include "games/pacman/pacman_mazes.h"
#include "gfx/canvas.h"
#include "gfx/png.h"

namespace pacman {

// Obraz z alfa (RGB565 + krycie): PNG z Gemini albo warianty (odbicia, obroty) generowane w kodzie.
struct Pic {
    int             w = 0, h = 0;
    const uint16_t* px    = nullptr;
    const uint8_t*  alpha = nullptr;
};

class PacmanGame final : public engine::Game {
public:
    void init(gfx::Canvas& canvas) override;
    void update(float dt, const input::PadState& pad) override;
    void render(gfx::Canvas& canvas) override;
    void debug_line(char* buf, size_t n) const override;

    static constexpr int W = 800, H = 480;
    static constexpr int CELL = 24, COLS = MAZE_COLS, ROWS = MAZE_ROWS;
    static constexpr int MAZE_W = COLS * CELL, MAZE_H = ROWS * CELL;   // 648 x 456
    static constexpr int MAZE_X = 8, MAZE_Y = (H - MAZE_H) / 2;        // 8, 12
    static constexpr int PANEL_X = MAZE_X + MAZE_W + 8, PANEL_W = W - PANEL_X - 8;   // 664, 128
    static constexpr int SPR = 40, FRUIT_PX = 32;
    static constexpr int GHOSTS = 4, WORLDS = 8, FRUITS = 8;

    enum class State { Title, Ready, Playing, Dying, LevelClear, GameOver };
    enum Dir : uint8_t { RIGHT, DOWN, LEFT, UP, NONE };
    enum GhostMode : uint8_t { IN_HOUSE, LEAVING, NORMAL, FRIGHTENED, EYES, ENTERING };
    enum Phase : uint8_t { SCATTER, CHASE };

private:
    struct Actor {
        float x = 0, y = 0;        // srodek, wspolrzedne labiryntu (px)
        Dir   dir = LEFT;
        int   dec_col = -99, dec_row = -99;   // kratka, w ktorej podjeto ostatnia decyzje (raz na kratke)
        bool  moving = false;
    };
    struct Ghost {
        Actor     a;
        GhostMode mode = IN_HOUSE;
        float     house_t = 0;     // czas w domu (tylko do kolysania)
        float     bob = 0;
        int       target_x = 0, target_y = 0;   // do rysowania debug / decyzji
    };
    struct Popup { float x, y, t; char text[8]; uint16_t color; bool alive; };
    struct Record { int version; int score; int level; };

    static constexpr int MAX_POPUPS = 8;

    // --- stan gry ---
    State state_ = State::Title;
    int   level_ = 1, start_level_ = 1;
    int   maze_ = 0, world_ = 0;
    uint8_t cell_[ROWS][COLS]{};   // kopia planszy: '#', '.', 'o', ' ', '-', 'G'
    int   pellets_left_ = 0, pellets_total_ = 0, pellets_eaten_ = 0;
    int   door_x_ = 13, door_y_ = 8, house_x_ = 13, house_y_ = 9;   // drzwi i srodek domu (px liczone z kratek)
    int   fruit_x_ = 13, fruit_y_ = 11;
    int   start_x_ = 13, start_y_ = 14;
    bool  tunnel_row_[ROWS]{};

    Actor pac_;
    Dir   wanted_ = LEFT;
    Ghost ghost_[GHOSTS];
    Phase phase_ = SCATTER;
    int   phase_idx_ = 0;
    float phase_t_ = 0;
    float fright_t_ = 0;          // > 0: duchy przestraszone
    float flash_t_ = 0;           // ostatnie tyle sekund strachu duchy migaja
    int   stall_frames_ = 0;      // gracz stoi tyle klatek po zjedzeniu kulki (1) / duzej kulki (3) - jak w oryginale
    // Wyjscia z domu jak w oryginale: licznik kulek "preferowanego" ducha (rozowy, blekitny, pomaranczowy - pierwszy
    // w domu), po stracie zycia licznik globalny (7/17/32), a przy braku jedzenia zegar 4 s (3 s od poziomu 5).
    int   dot_count_[GHOSTS]{};
    int   global_dots_ = 0;
    bool  global_mode_ = false;
    float no_dot_t_ = 0;
    int   ghost_chain_ = 0;       // zjedzone duchy w jednym ciagu (200, 400, 800, 1600)
    float fruit_t_ = 0;           // > 0: owoc na planszy
    int   fruit_kind_ = 0;
    bool  fruit_shown_[2]{};      // owoc po 70 i 170 kulkach - raz na poziom
    int   fruits_taken_ = 0;      // ile owocow zjedzono w tej grze (ikony w panelu)
    uint8_t fruit_hist_[8]{};

    int   score_ = 0, lives_ = 3;
    bool  extra_life_ = false;
    float state_t_ = 0, anim_ = 0, level_time_ = 0, pause_t_ = 0;
    bool  autopilot_ = false;
    Record top_{};
    Popup popups_[MAX_POPUPS]{};
    int   eaten_ghost_ = -1;      // duch zjedzony przed chwila (chwila zatrzymania)
    int   deaths_ = 0;

    // --- grafika ---
    bool      assets_loaded_ = false;
    int       baked_level_ = -1;
    uint16_t* bake_  = nullptr;   // tlo + sciany labiryntu (MAZE_W x MAZE_H), kopiowane co klatke
    uint8_t*  mask_  = nullptr;   // maska do rysowania swiecacych scian
    uint16_t* title_ = nullptr;   // ilustracja tytulowa 800x480
    Pic hero_[4][4]{};            // kierunek x klatka paszczy
    Pic die_[4]{};
    Pic icon_hero_{};
    Pic ghost_img_[GHOSTS][2][2]{};   // duch x klatka x (0 prawo, 1 lewo)
    Pic scared_[2][2]{};
    Pic eyes_[2]{};
    Pic fruit_img_[FRUITS]{};
    Pic fruit_icon_[FRUITS]{};

    // logika (pacman_game.cpp)
    void start_game();
    void load_level();
    void reset_actors();
    void update_title(const input::PadState& pad);
    void update_playing(float dt, const input::PadState& pad);
    void move_pac(float dt);
    void move_ghost(Ghost& g, float dt, int idx);
    void ghost_decide(Ghost& g, int idx, int col, int row);
    void ghost_target(const Ghost& g, int idx, int& tx, int& ty) const;
    void set_fright();
    void eat_at(int col, int row);
    void kill_pac();
    void eat_ghost(int idx);
    void next_phase();
    bool walkable(int col, int row, bool ghost_door) const;
    void cell_of(const Actor& a, int& col, int& row) const;
    float phase_duration(int idx) const;
    int   preferred_ghost() const;
    bool  no_up_zone(int col, int row) const;
    Dir  autopilot_dir(int col, int row);
    void add_score(int pts);
    void popup(float x, float y, const char* text, uint16_t color);
    void save_record();

    // rysowanie (pacman_render.cpp)
    void load_assets();
    void bake_maze();
    void draw_maze(gfx::Canvas& c);
    void draw_pellets(gfx::Canvas& c);
    void draw_fruit(gfx::Canvas& c);
    void draw_ghosts(gfx::Canvas& c);
    void draw_pac(gfx::Canvas& c);
    void draw_popups(gfx::Canvas& c);
    void draw_panel(gfx::Canvas& c);
    void draw_title(gfx::Canvas& c);
    void draw_overlay(gfx::Canvas& c);
};

}  // namespace pacman
