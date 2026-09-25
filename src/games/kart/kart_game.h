// Kart - wyscigi gokartow w prawdziwym 3D (software'owy renderer gfx3d, low-poly, 800x480):
// tor z wzniesieniami zbudowany z 16 punktow kontrolnych (asfalt i trawa cieniowane Gouraud z kolorami wierzcholkow,
// krawezniki, linie, pasy startu i przyspieszenia), gokarty jako modele 3D (nadwozie z bryl scietych, obracajace sie
// kola skrecajace z kierownica, kierowca z kaskiem; przechyl w zakretach i na gorkach), brama startowa, trybuna,
// stosy opon w zakretach, banery, drzewa i krzaki z cieniami, skrzynki i przedmioty jako bryly, kamera za gokartem
// podazajaca za terenem (szerszy kat przy turbo, orbita na ekranie tytulowym), mgla, niebo z chmurami;
// dym driftu, kurz i plomien turbo jako obrazy z alfa rzutowane na scene.
// 4 gokarty (gracz + 3 rywali AI), 3 okrazenia, skrzynki (grzyb, banan, skorupa), drift z mini-turbo,
// pola przyspieszenia, ranking, mini-mapa. Fizyka jest 2D (plaszczyzna XZ), wysokosc terenu tylko wizualna.
//
// Jakosc adaptacyjna: gdy render trwa > 13 ms (plytka), gra skraca zasieg rysowania i wylacza chmury.
#pragma once

#include <stdint.h>

#include "engine/game.h"
#include "gfx/canvas.h"
#include "gfx/png.h"
#include "gfx3d/mesh.h"
#include "gfx3d/renderer.h"
#include "games/kart/kart_tracks.h"

namespace kart {

class KartGame final : public engine::Game {
public:
    void init(gfx::Canvas& canvas) override;
    void update(float dt, const input::PadState& pad) override;
    void render(gfx::Canvas& canvas) override;
    void debug_line(char* buf, size_t n) const override;

    static constexpr int W = 800, H = 480;
    static constexpr int WORLD = 1024;            // rozmiar swiata (jednostki fizyki), plaszczyzna XZ
    static constexpr int SURF = 128;              // mapa nawierzchni: komorki 8x8 swiata
    static constexpr int N_PATH = 256;            // probki linii srodkowej toru
    static constexpr int N_KARTS = 4;
    static constexpr int N_BOXES = 9, N_HAZARDS = 12, N_SHELLS = 6, N_TREES = 64, N_SPARKS = 48, N_PUFFS = 48;
    static constexpr int N_BUSHES = 40;
    static constexpr int N_SKIDS  = 240;          // slady opon (bufor cykliczny)
    static constexpr int N_OBST   = 160;          // przeszkody (drzewa, stosy opon, slupy, trybuna) - kolizje
    static constexpr int LAPS = 3;

    enum class State { Title, Countdown, Racing, Finished };
    enum Item : uint8_t { NONE = 0, MUSHROOM, BANANA, SHELL };

    struct Kart {
        float x = 0, y = 0;
        float angle = 0;         // kierunek jazdy (rad), 0 = +x
        float speed = 0;         // jednostki swiata / s
        float lane = 0;          // AI: przesuniecie od linii srodkowej
        float ai_timer = 0;
        int   path_idx = 0;
        int   lap = 1;
        bool  half = false;      // minal polowe okrazenia (zabezpieczenie licznika)
        float progress = 0;      // lap * N_PATH + idx - do rankingu
        int   place = 1;
        Item  item = NONE;
        float item_t = 0;        // AI: ile czeka z uzyciem
        float boost_t = 0;       // >0: przyspieszenie (grzyb, mini-turbo, pole)
        float spin_t = 0;        // >0: wirowanie po trafieniu
        float drift_t = 0;       // ile trwa drift (mini-turbo po puszczeniu)
        bool  drifting = false;
        float shield_t = 0;      // nietykalnosc po trafieniu
        bool  finished = false;
        float finish_time = 0;
        int   color = 0;         // 0 czerwony (gracz), 1 niebieski, 2 zielony, 3 zolty
    };
    struct Hazard  { float x, y; bool alive; };                    // banan
    struct Shell   { float x, y, dx, dy, t; bool alive; int owner; };
    struct ItemBox { float x, y, respawn; };
    struct Tree    { float x, y; int kind; };
    struct Bush    { float x, y; int kind; float scale; float h; };  // dekoracja przy krawezniku (tylko render), h = wysokosc podloza
    struct Spark   { float x, y, vx, vy, t; uint16_t color; };    // w przestrzeni ekranu
    struct Puff    { float x, y, t, life, size; bool alive; uint8_t kind; };   // dym driftu (0) / kurz (1), w swiecie
    struct Skid    { float x[4], y[4], h[4]; float age; bool alive; };        // slad opony: czworokat na asfalcie (h = wysokosc)
    struct Obstacle { float x, y, r; };                                        // okrag kolizji obiektu przy torze

private:
    // stan wyscigu
    State  state_ = State::Title;
    Kart   karts_[N_KARTS];
    Hazard hazards_[N_HAZARDS]{};
    Shell  shells_[N_SHELLS]{};
    ItemBox boxes_[N_BOXES]{};
    Tree   trees_[N_TREES]{};
    Spark  sparks_[N_SPARKS]{};
    Puff   puffs_[N_PUFFS]{};
    float  countdown_ = 0, race_t_ = 0, go_t_ = 0, anim_ = 0;
    float  cam_angle_ = 0, cam_x_ = 0, cam_y_ = 0;
    float  best_lap_ = 0, lap_start_t_ = 0;
    bool   autopilot_ = false;         // Y trzymany: AI prowadzi gokart gracza (demo, testy)
    int    puff_phase_ = 0;
    Skid   skids_[N_SKIDS]{};
    Obstacle obst_[N_OBST]{};
    int    n_obst_ = 0;
    int    skid_next_ = 0;
    float  skid_prev_[N_KARTS][2][2]{};   // ostatni punkt sladu tylnych kol (lewe, prawe): x, y
    bool   skid_on_[N_KARTS][2]{};
    float  steer_vis_[N_KARTS]{}, wheel_spin_[N_KARTS]{}, yaw_rate_[N_KARTS]{}, prev_angle_[N_KARTS]{};

    // tor (kart_tracks.h): wybor na ekranie tytulowym, rekordy okrazen per tor w pamieci trwalej ("kart_best")
    static constexpr int MAX_TRACKS = 8;
    int     track_ = 0;
    int     scene_track_ = -1;         // tor, dla ktorego zbudowano siatki
    float   rec_lap_[MAX_TRACKS]{};    // najlepsze okrazenie na torze (0 = brak)
    bool    title_lr_[2]{};            // LEWO/PRAWO na ekranie tytulowym (zbocza)
    uint16_t sky_top_ = 0, sky_mid_ = 0, sky_horiz_ = 0, fog_col_ = 0;   // kolory motywu toru
    gfx::IndexedImage atlas_th_[THEME_COUNT];   // atlasy motywow (wczytywane przy pierwszym uzyciu)
    gfx::Image        mountains_th_[THEME_COUNT];
    bool              theme_loaded_[THEME_COUNT]{};
    float   path_x_[N_PATH], path_y_[N_PATH];
    uint8_t surf_[SURF * SURF];        // 0 trawa, 1 asfalt/krawezniki, 2 pole przyspieszenia

    // scena 3D
    gfx3d::Renderer r3d_;
    gfx3d::Mesh     road_, marks_, terrain_, props_, tree_bb_[4], tree_shadow_m_, bush_bb_[2];
    gfx::IndexedImage atlas_;            // atlas tekstur (assets/kart/atlas.png, 8-bit z paleta)
    gfx3d::Mesh     kart_body_[4], helmet_[4], wheel_front_, wheel_rear_;
    gfx3d::Mesh     itembox_m_, banana_m_, shell_m_, mushroom_m_, shadow_m_;
    Bush            bushes_[N_BUSHES]{};
    float           tree_h_[N_TREES]{};    // wysokosc podloza pod drzewem (liczona raz)
    bool            scene_ok_ = false;

    // stan wylacznie wizualny (HUD, kamera) - nie wplywa na fizyke ani na slad --trace
    float fov_vis_   = 1.f;      // kat widzenia wygladzany (szerszy przy turbo)
    int   lap_seen_  = 1;        // do komunikatu o nowym okrazeniu
    float lap_flash_ = 0;
    float last_anim_ = 0;

    // grafika 2D (PNG z alfa): niebo, efekty, ikony HUD
    gfx::Image banana_, shell_, mushroom_, clouds_, mountains_, smoke_, glow_, flame_;
    bool       assets_loaded_ = false;

    // jakosc adaptacyjna
    int   quality_     = 0;
    float render_ms_   = 0;
    int   slow_frames_ = 0;

    // logika
    void new_race();
    void build_track();
    void select_track(int t);          // przebudowa toru i sceny (ekran tytulowy)
    void place_karts_on_grid();
    int  surface_at(float x, float y) const;
    void update_kart(Kart& k, float dt, float steer, bool gas, bool brake, bool drift);
    void ai_control(Kart& k, int index, float dt, float& steer, bool& gas, bool& brake, bool& drift);
    void update_progress(Kart& k);
    void update_ranking();
    void update_items(float dt, bool player_use);
    void use_item(Kart& k, int index);
    void hit_kart(Kart& k);
    void kart_collisions();
    void add_obstacle(float x, float y, float r);   // wolane przy budowie sceny (render), uzywane w fizyce
    void obstacle_collisions(Kart& k);
    void add_spark(float x, float y, float vx, float vy, float t, uint16_t color);
    void add_puff(float x, float y, float size, float life, int kind);
    void update_skids(float dt);
    void update_effects(float dt);

    // scena 3D (kart_render.cpp)
    float ground_height(float x, float z) const;     // wysokosc terenu (tylko wizualna)
    // wysokosc NAWIERZCHNI w punkcie: jezdnia jest plaska w poprzek (wysokosc srodka toru), pobocze przechodzi do terenu -
    // dokladnie tak, jak zbudowana siatka road_. Wszystko, co stoi na drodze (bolidy, przedmioty, slady, cienie), musi
    // uzywac tej funkcji, nie ground_height (inaczej z Z-buforem kola gina pod asfaltem).
    float surface_height(float x, float z) const;
    float path_curvature(int idx) const;             // zmiana kierunku toru wokol probki (rad, znak = strona)
    void  path_frame(int idx, float& tx, float& tz, float& rx, float& rz) const;   // kierunek i prawo w probce
    void  build_scene();               // raz: renderer, modele bolidow i przedmiotow; potem build_track_scene
    void  build_track_scene();         // co tor: tekstury motywu, droga, teren, obiekty, drzewa
    void  build_road();
    void  build_terrain();
    void  build_props();
    void  build_decor();
    void  build_kart_models();
    void  draw_scene(gfx::Canvas& c);
    void  draw_kart_3d(int index);
    void  draw_sky(gfx::Canvas& c, float horizon);
    void  draw_effects(gfx::Canvas& c);
    void  draw_image(gfx::Canvas& c, const gfx::Image& img, int frame_w, int frame, float cx, float bottom, float scale,
                     int alpha_mul = 255, bool flip = false, bool filtered = true);
    void  draw_strip(gfx::Canvas& c, const gfx::Image& strip, int top_row, float angle_offset, bool opaque_only);
    void  draw_hud(gfx::Canvas& c);
    void  draw_hud_race(gfx::Canvas& c);
    void  draw_hud_title(gfx::Canvas& c);
    void  draw_hud_finish(gfx::Canvas& c);
    void  draw_minimap(gfx::Canvas& c);
    static float wrap_angle_static(float a);
};

}  // namespace kart
