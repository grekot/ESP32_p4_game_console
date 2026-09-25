// Kart - scena 3D (gfx3d) i HUD: budowa siatek (tor z wzniesieniami teksturowany i cieniowany Gouraud, oznaczenia,
// teren, obiekty przy torze, drzewa i krzaki jako billboardy, bolidy z malowaniami), kamera za gokartem, cienie
// rzutowane, slady opon, niebo z chmurami i gorami (pasy 2D), efekty (dym, kurz, plomien, iskry) i HUD.
#include "games/kart/kart_game.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "engine/rng.h"
#include "gfx/palette.h"
#include "gfx/text.h"
#include "platform/platform.h"

namespace kart {

using gfx3d::Mat4;
using gfx3d::Vec3;

namespace {

const char* TAG = "kart";

constexpr float ROAD_HALF_R  = 32.f;   // asfalt (jak w fizyce: ROAD_HALF)
constexpr float LINE_IN_R    = 30.f;   // wewnetrzna krawedz bialej linii (30..32)
constexpr float MID_R        = 1.6f;   // polszerokosc przerywanej linii srodkowej
constexpr float CURB_HALF_R  = 38.f;   // + krawezniki (32..38)
constexpr float SHOULDER_R   = 70.f;   // pobocze (trawa przy drodze)
constexpr float FAR_R        = 115.f;  // dalsze pobocze; dalej teren z siatki
constexpr float TERRAIN_STEP = 32.f;   // komorka siatki terenu
constexpr int   TERRAIN_N    = 32;     // 32 x 32 komorki = 1024 jednostek

constexpr float SUN_ANGLE = 0.9f;      // kierunek slonca w plaszczyznie XZ (rad) - swiatlo i tarcza na niebie zgodne
constexpr float SUN_ELEV  = 1.15f;     // skladowa pionowa kierunku do slonca (przed normalizacja) - ok. 50 stopni,
                                       // krotsze cienie rzutowane
inline Vec3 sun_dir() { return Vec3(cosf(SUN_ANGLE), SUN_ELEV, sinf(SUN_ANGLE)).normalized(); }

// Kafelki atlasu (tools/gen_kart_atlas.py): { u0, v0, u1, v1 } z wcieciem 0.5 teksela (bez przeciekania sasiadow)
struct Tile { float u0, v0, u1, v1; };
constexpr Tile tile(float x, float y, float w, float h) { return { x + 0.5f, y + 0.5f, x + w - 0.5f, y + h - 0.5f }; }
constexpr Tile T_ASPHALT = tile(0, 0, 128, 128), T_GRASS = tile(128, 0, 128, 128), T_GRASS_DRY = tile(256, 0, 128, 128);
constexpr Tile T_CROWD = tile(384, 0, 128, 32);
constexpr Tile T_BANNER[3] = { tile(384, 32, 128, 32), tile(384, 64, 128, 32), tile(384, 96, 128, 32) };
// drzewa: dab, sosna, brzoza, klon (tools/gen_kart_atlas.py; z Gemini, gdy jest assets_src/kart/trees_sheet.jpg)
constexpr Tile T_TREE[4] = { tile(0, 128, 128, 128), tile(128, 128, 128, 128), tile(256, 256, 128, 128), tile(384, 256, 128, 128) };
constexpr float TREE_SIZE[4] = { 30.f, 28.f, 27.f, 29.f };
constexpr Tile T_BUSH[2] = { tile(256, 128, 64, 64), tile(320, 128, 64, 64) };
constexpr Tile T_TREAD = tile(256, 192, 64, 64), T_GATE = tile(0, 384, 448, 40);
constexpr float LIVERY_X0 = 0.f, LIVERY_Y0 = 256.f;   // 4 malowania 64x128 obok siebie

inline uint16_t pack565(int r, int g, int b) { return (uint16_t)((r << 11) | (g << 5) | b); }

inline uint16_t blend565(uint16_t dst, uint16_t src, int a)   // a = 0..255 krycie src
{
    if (a >= 255) return src;
    const int ia = 255 - a;
    const int r = (((src >> 11) & 31) * a + ((dst >> 11) & 31) * ia) / 255;
    const int g = (((src >> 5) & 63) * a + ((dst >> 5) & 63) * ia) / 255;
    const int b = ((src & 31) * a + (dst & 31) * ia) / 255;
    return pack565(r, g, b);
}

inline uint16_t mix565i(uint16_t a, uint16_t b, int k32)
{
    const int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
    const int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
    return pack565((ar * (32 - k32) + br * k32) >> 5, (ag * (32 - k32) + bg * k32) >> 5, (ab * (32 - k32) + bb * k32) >> 5);
}

// deterministyczny szum 0..1 z dwoch liczb calkowitych (ten sam na PC i na plytce)
inline float noise01(int a, int b)
{
    uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u + 0x9E3779B9u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.f;
}

const uint16_t PANEL     = gfx::rgb565(12, 16, 28);
const uint16_t PANEL_HI  = gfx::rgb565(120, 140, 190);
const uint16_t TEXT_DIM  = gfx::rgb565(160, 170, 195);

const uint16_t KART_COLORS[4] = { gfx::rgb565(228, 40, 44), gfx::rgb565(44, 104, 228), gfx::rgb565(56, 186, 78),
                                  gfx::rgb565(250, 200, 44) };
const uint16_t KART_DARK[4]   = { gfx::rgb565(110, 20, 24), gfx::rgb565(22, 46, 122), gfx::rgb565(24, 96, 40),
                                  gfx::rgb565(150, 110, 18) };
const uint16_t HELMET[4]      = { gfx::rgb565(242, 242, 246), gfx::rgb565(242, 242, 246), gfx::rgb565(255, 222, 92),
                                  gfx::rgb565(40, 40, 52) };
const uint16_t SUIT[4]        = { gfx::rgb565(150, 30, 34), gfx::rgb565(30, 60, 150), gfx::rgb565(34, 120, 52),
                                  gfx::rgb565(170, 130, 28) };

const uint16_t ASPHALT    = gfx::rgb565(84, 84, 92), GRASS = gfx::rgb565(74, 150, 60);
const uint16_t CURB_RED   = gfx::rgb565(222, 44, 44), CURB_WHITE = gfx::rgb565(236, 236, 236);
const uint16_t LINE_COL   = gfx::rgb565(212, 212, 218), DASH_COL = gfx::rgb565(140, 140, 150);
const uint16_t BANNER_COLS[6] = { gfx::rgb565(240, 110, 30), gfx::rgb565(40, 170, 220), gfx::rgb565(150, 60, 200),
                                  gfx::rgb565(240, 60, 90),  gfx::rgb565(60, 190, 110), gfx::rgb565(250, 200, 40) };

void fill_rect_alpha(gfx::Canvas& c, int x, int y, int w, int h, uint16_t color, int a)
{
    const int x0 = engine::imax(x, 0), y0 = engine::imax(y, 0);
    const int x1 = engine::imin(x + w, c.width()), y1 = engine::imin(y + h, c.height());
    uint16_t* px = c.data();
    for (int yy = y0; yy < y1; ++yy) {
        uint16_t* row = px + (size_t)yy * c.width();
        for (int xx = x0; xx < x1; ++xx) row[xx] = blend565(row[xx], color, a);
    }
}

// Zaokraglony prostokat z kryciem: wiersz po wierszu, wciecie w narozach z rownania okregu.
void fill_round_rect_alpha(gfx::Canvas& c, int x, int y, int w, int h, int r, uint16_t color, int a)
{
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    for (int yy = 0; yy < h; ++yy) {
        float d = 0.f;
        if (yy < r) d = (float)(r - yy) - 0.5f;
        else if (yy >= h - r) d = (float)(yy - (h - r)) + 0.5f;
        int inset = 0;
        if (d > 0.f) inset = r - (int)floorf(sqrtf(fmaxf((float)r * r - d * d, 0.f)) + 0.5f);
        fill_rect_alpha(c, x + inset, y + yy, w - 2 * inset, 1, color, a);
    }
}

// Panel HUD: ciemne tlo z zaokragleniem, delikatna jasna kreska u gory
void hud_panel(gfx::Canvas& c, int x, int y, int w, int h, int a = 165)
{
    fill_round_rect_alpha(c, x + 2, y + 3, w, h, 12, gfx::BLACK, a / 3);   // cien
    fill_round_rect_alpha(c, x, y, w, h, 12, PANEL, a);
    fill_rect_alpha(c, x + 12, y, w - 24, 1, PANEL_HI, 110);
}

void text_shadow(gfx::Canvas& c, int x, int y, const char* s, uint16_t col, int px)
{
    gfx::draw_text_px(c, x + 2, y + 2, s, gfx::rgb565(6, 8, 14), px);
    gfx::draw_text_px(c, x, y, s, col, px);
}

// Pret o kwadratowym przekroju miedzy dwoma punktami (wahacze, wydechy, ramiona, kolumna kierownicy).
void add_rod(gfx3d::Mesh& m, const Vec3& a, const Vec3& b, float r, uint16_t col)
{
    const Vec3 d   = (b - a).normalized();
    const Vec3 ref = fabsf(d.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    const Vec3 u   = gfx3d::cross(d, ref).normalized(), v = gfx3d::cross(d, u);
    Vec3 k[8];
    for (int i = 0; i < 8; ++i) {
        const Vec3& c = (i & 4) ? b : a;
        k[i] = c + u * ((i & 1) ? r : -r) + v * ((i & 2) ? r : -r);
    }
    m.add_hexa(k, col, col, gfx3d::shade565(col, 0.75f));
}

// Dwustronny prostokat teksturowany (baner, publicznosc): dwa billboardy o przeciwnych normalnych
void add_sign(gfx3d::Mesh& m, const Vec3& base, float w, float h, float nx, float nz, const Tile& t, uint16_t fallback)
{
    const float uv[4] = { t.u0, t.v0, t.u1, t.v1 };
    m.add_billboard({ base.x + nx * 0.15f, base.y, base.z + nz * 0.15f }, w, h, nx, nz, uv, fallback);
    m.add_billboard({ base.x - nx * 0.15f, base.y, base.z - nz * 0.15f }, w, h, -nx, -nz, uv, fallback);
}

// Kask: gladka elipsoida (wspolne wierzcholki -> cieniowanie Gouraud i odblask bez fasetek) z obszarami koloru na
// tej samej powierzchni zamiast doklejonych kostek: wizjer (przod, pas nad "podbrodkiem"), pas przez srodek skorupy
// w kolorze bolidu, ciemny otwor na szyje. Os lokalna: X prawo, Y gora, Z przod; r = polosie (szerokosc, wysokosc,
// dlugosc). segments x rings: 20 x 11 = 400 trojkatow.
void add_helmet(gfx3d::Mesh& m, const Vec3& c, const Vec3& r, uint16_t shell, uint16_t stripe, uint16_t visor)
{
    constexpr int S = 20, R = 11;
    int idx[R + 1][S];
    const int top = m.add_vertex({ c.x, c.y + r.y, c.z });
    const int bot = m.add_vertex({ c.x, c.y - r.y, c.z });
    for (int i = 1; i < R; ++i) {
        const float phi = 3.1415926f * (float)i / R;
        for (int j = 0; j < S; ++j) {
            const float th = 6.2831853f * (float)j / S;   // 0 = przod (+Z)
            idx[i][j] = m.add_vertex({ c.x + r.x * sinf(phi) * sinf(th), c.y + r.y * cosf(phi), c.z + r.z * sinf(phi) * cosf(th) });
        }
    }
    const uint16_t neck = gfx::rgb565(20, 20, 26), trim = gfx::rgb565(60, 62, 72);
    auto region = [&](float phi, float th) -> uint16_t {
        const float dy = cosf(phi), dz = sinf(phi) * cosf(th);   // kierunek na sferze (y, z)
        if (dy < -0.62f) return neck;
        if (dz > 0.42f && dy > -0.18f && dy < 0.36f) return visor;                          // wizjer
        if (dz > 0.30f && ((dy > -0.24f && dy < -0.18f) || (dy > 0.36f && dy < 0.44f))) return trim;   // obwodka
        if (fabsf(sinf(th)) < 0.2f && dy > 0.05f) return stripe;   // pas przez srodek: stala szerokosc katowa (2 segmenty)
        return shell;
    };
    for (int j = 0; j < S; ++j) {
        const int j1 = (j + 1) % S;
        const float thm = 6.2831853f * ((float)j + 0.5f) / S;
        m.add_tri_out(top, idx[1][j], idx[1][j1], region(0.1f, thm), Vec3(0, 1, 0));
        for (int i = 1; i < R - 1; ++i) {
            const float phm = 3.1415926f * ((float)i + 0.5f) / R;
            const Vec3 out = Vec3(sinf(phm) * sinf(thm), cosf(phm), sinf(phm) * cosf(thm));
            m.add_quad_out(idx[i][j], idx[i + 1][j], idx[i + 1][j1], idx[i][j1], region(phm, thm), out);
        }
        m.add_tri_out(bot, idx[R - 1][j1], idx[R - 1][j], neck, Vec3(0, -1, 0));
    }
}

// Rura o przekroju wielokata (ramiona kierowcy, kolumna kierownicy): wspolne wierzcholki pierscieni, zeby
// cieniowanie gladkie dawalo okragly wyglad (add_rod ma przekroj kwadratowy).
void add_tube(gfx3d::Mesh& m, const Vec3& a, const Vec3& b, float r, int sides, uint16_t col)
{
    const Vec3 d   = (b - a).normalized();
    const Vec3 ref = fabsf(d.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    const Vec3 u   = gfx3d::cross(d, ref).normalized(), v = gfx3d::cross(d, u);
    int ra[12], rb[12];
    if (sides > 12) sides = 12;
    for (int i = 0; i < sides; ++i) {
        const float t = 6.2831853f * (float)i / sides;
        const Vec3 o = u * (cosf(t) * r) + v * (sinf(t) * r);
        ra[i] = m.add_vertex(a + o);
        rb[i] = m.add_vertex(b + o);
    }
    const int ca = m.add_vertex(a), cb = m.add_vertex(b);
    for (int i = 0; i < sides; ++i) {
        const int i1 = (i + 1) % sides;
        const float t = 6.2831853f * ((float)i + 0.5f) / sides;
        m.add_quad_out(ra[i], rb[i], rb[i1], ra[i1], col, u * cosf(t) + v * sinf(t));
        m.add_tri_out(ca, ra[i1], ra[i], col, d * -1.f);
        m.add_tri_out(cb, rb[i], rb[i1], col, d);
    }
}

}  // namespace

float KartGame::wrap_angle_static(float a)
{
    while (a > 3.14159265f) a -= 6.28318530718f;
    while (a < -3.14159265f) a += 6.28318530718f;
    return a;
}

// ============================================================================ teren

float KartGame::ground_height(float x, float z) const
{
    // lagodne wzniesienia: suma sinusow (deterministyczna, gladka, tania); ostatni skladnik = duze, dalekie pagorki
    // tor: skala i faza (kart_tracks.h); dla toru 0 (hills 1, phase 0) wartosci identyczne jak przed wieloma torami
    const float ph = TRACKS[track_].phase;
    return TRACKS[track_].hills *
           (7.f * sinf(x * 0.0105f + 1.1f + ph) + 6.f * sinf(z * 0.0123f + 2.3f - ph) + 3.5f * sinf((x + z) * 0.019f + 0.5f + ph) +
            2.f * sinf((x - 2.f * z) * 0.027f + ph) + 9.f * sinf(x * 0.0061f + 0.7f - ph) * sinf(z * 0.0072f + 1.9f + ph));
}

float KartGame::surface_height(float x, float z) const
{
    // najblizszy odcinek linii srodkowej -> wysokosc srodka toru wzdluz (liniowo) i odleglosc w poprzek
    int   best = 0;
    float bd   = 1e30f;
    for (int i = 0; i < N_PATH; ++i) {
        const float dx = path_x_[i] - x, dz = path_y_[i] - z;
        const float d = dx * dx + dz * dz;
        if (d < bd) { bd = d; best = i; }
    }
    // odcinek: do nastepnej albo poprzedniej probki, zaleznie od tego, po ktorej stronie lezy punkt
    const int   n  = (best + 1) % N_PATH, p = (best + N_PATH - 1) % N_PATH;
    float       ax = path_x_[best], az = path_y_[best], bx = path_x_[n], bz = path_y_[n];
    if ((x - ax) * (bx - ax) + (z - az) * (bz - az) < 0.f) { bx = path_x_[p]; bz = path_y_[p]; }
    const float ex = bx - ax, ez = bz - az, el = ex * ex + ez * ez + 1e-6f;
    const float t  = engine::clampf(((x - ax) * ex + (z - az) * ez) / el, 0.f, 1.f);
    const float cx = ax + ex * t, cz = az + ez * t;
    const float hc = ground_height(cx, cz);
    const float d  = sqrtf((x - cx) * (x - cx) + (z - cz) * (z - cz));
    if (d <= CURB_HALF_R) return hc;                                   // jezdnia i krawezniki: plasko w poprzek
    const float g = ground_height(x, z);
    const float mid = (g + hc) * 0.5f;                                   // wierzcholek pobocza (SHOULDER_R) lezy w polowie
    if (d <= SHOULDER_R) return hc + (mid - hc) * (d - CURB_HALF_R) / (SHOULDER_R - CURB_HALF_R);
    if (d <= FAR_R) return mid + (g - mid) * (d - SHOULDER_R) / (FAR_R - SHOULDER_R);
    return g;
}

void KartGame::path_frame(int idx, float& tx, float& tz, float& rx, float& rz) const
{
    const int n = (idx + 1) % N_PATH, p = (idx + N_PATH - 1) % N_PATH;
    tx = path_x_[n] - path_x_[p];
    tz = path_y_[n] - path_y_[p];
    const float l = sqrtf(tx * tx + tz * tz) + 1e-6f;
    tx /= l; tz /= l;
    rx = -tz; rz = tx;   // prawo (kat rosnie = skret w prawo)
}

float KartGame::path_curvature(int idx) const
{
    float t0x, t0z, t1x, t1z, r;
    path_frame((idx + N_PATH - 2) % N_PATH, t0x, t0z, r, r);
    path_frame((idx + 2) % N_PATH, t1x, t1z, r, r);
    return wrap_angle_static(atan2f(t1z, t1x) - atan2f(t0z, t0x));
}

void KartGame::build_scene()
{
    scene_ok_ = r3d_.init(18000);
    if (!scene_ok_) return;
    build_track_scene();

    // --- przedmioty ---
    itembox_m_.init(40, 24);
    itembox_m_.add_box({ 0, 0, 0 }, { 6, 6, 6 }, gfx::rgb565(255, 150, 215), gfx::rgb565(120, 200, 255));
    itembox_m_.add_box({ 0, 0, 0 }, { 2.2f, 7.4f, 2.2f }, gfx::rgb565(255, 235, 120), gfx::rgb565(255, 225, 110));   // "znak zapytania"
    itembox_m_.unlit = true;
    banana_m_.init(60, 100);
    banana_m_.add_sphere({ 0, 1.6f, 0 }, 2.4f, 8, 4, gfx::rgb565(250, 215, 60), gfx::rgb565(220, 170, 40));
    banana_m_.smooth = true;
    banana_m_.specular = 0.4f;
    banana_m_.compute_smooth_normals();
    shell_m_.init(60, 100);
    shell_m_.add_sphere({ 0, 3.f, 0 }, 3.2f, 8, 4, gfx::rgb565(50, 160, 70), gfx::rgb565(235, 225, 195));
    shell_m_.smooth = true;
    shell_m_.specular = 0.6f;
    shell_m_.compute_smooth_normals();
    mushroom_m_.init(80, 120);
    mushroom_m_.add_cylinder_y({ 0, 0, 0 }, 1.6f, 3.2f, 6, gfx::rgb565(250, 232, 205));
    mushroom_m_.add_sphere({ 0, 3.6f, 0 }, 3.4f, 8, 4, gfx::rgb565(230, 40, 40), gfx::rgb565(230, 40, 40));
    mushroom_m_.smooth = true;
    mushroom_m_.specular = 0.5f;
    mushroom_m_.compute_smooth_normals();
    shadow_m_.init(20, 20);
    shadow_m_.add_disc_y({ 0, 0, 0 }, 9.5f, 11.f, 10, gfx::BLACK);

    build_kart_models();
    CONSOLE_LOGI(TAG, "scena 3D: droga %d+%d tri, teren %d tri, obiekty %d tri, gokart %d+%d tri, atlas %s", road_.triangle_count(),
                 marks_.triangle_count(), terrain_.triangle_count(), props_.triangle_count(), kart_body_[0].triangle_count(),
                 wheel_rear_.triangle_count() * 4, atlas_.idx ? "OK" : "BRAK");
}

void KartGame::build_track_scene()
{
    // motyw toru: kolory nieba i mgly, atlas i panorama gor (wczytane raz na motyw, zyja do konca programu)
    const int th = TRACKS[track_].theme;
    const ThemeDef& M = THEMES[th];
    sky_top_   = gfx::rgb565(M.sky_top[0], M.sky_top[1], M.sky_top[2]);
    sky_mid_   = gfx::rgb565(M.sky_mid[0], M.sky_mid[1], M.sky_mid[2]);
    sky_horiz_ = gfx::rgb565(M.sky_horiz[0], M.sky_horiz[1], M.sky_horiz[2]);
    fog_col_   = gfx::rgb565(M.fog[0], M.fog[1], M.fog[2]);
    if (!theme_loaded_[th]) {
        theme_loaded_[th] = true;
        char path[48];
        snprintf(path, sizeof(path), "kart/atlas_%s.png", M.id);
        atlas_th_[th] = gfx::load_png_indexed(path);
        if (!atlas_th_[th].idx) atlas_th_[th] = gfx::load_png_indexed("kart/atlas.png");
        snprintf(path, sizeof(path), "kart/mountains_%s.png", M.id);
        mountains_th_[th] = gfx::load_png_rgba(path);
        if (!mountains_th_[th].px) mountains_th_[th] = gfx::load_png_rgba("kart/mountains.png");
    }
    atlas_     = atlas_th_[th];
    mountains_ = mountains_th_[th];
    if (atlas_.idx && atlas_.w == 512) r3d_.set_texture(atlas_.idx, 9, atlas_.h, atlas_.palette, fog_col_);
    else CONSOLE_LOGW(TAG, "brak atlasu motywu %s - scena bez tekstur (kolory zastepcze)", M.id);
    n_obst_ = 0;   // przeszkody (drzewa, opony, trybuna) dopisuje budowa obiektow ponizej
    build_road();
    build_terrain();
    build_props();
    build_decor();
    scene_track_ = track_;
}

void KartGame::build_road()
{
    // Segmenty specjalne (linia startu, pola przyspieszenia) maja asfalt rysowany plasko w marks_ (wzor), reszta
    // asfaltu i cala trawa sa w road_ - teksturowane (atlas) i cieniowane Gouraud po normalnych (wzniesienia).
    auto special = [&](int i) {
        if (i == 0 || i == N_PATH - 1) return true;
        for (int b = 0; b < 2; ++b) if (i >= TRACKS[track_].boost[b] && i < TRACKS[track_].boost[b] + 3) return true;
        return false;
    };

    // --- road_: 10 wierzcholkow w poprzek (pasy trawy i asfaltu rozdzielone - krawezniki i linie to luki) ---
    const float OFF_R[10] = { -FAR_R, -SHOULDER_R, -CURB_HALF_R, -LINE_IN_R, -MID_R, MID_R, LINE_IN_R, CURB_HALF_R, SHOULDER_R, FAR_R };
    road_.init(N_PATH * 10 + 8, N_PATH * 12 + 8);
    road_.smooth = true;
    // tablice robocze na stercie: stos zadania konsoli na plytce ma 16 kB, a .bss szkoda na dane jednorazowe
    int* ring = new int[N_PATH * 10];
    for (int i = 0; i < N_PATH; ++i) {
        float tx, tz, rx, rz;
        path_frame(i, tx, tz, rx, rz);
        const float hc = ground_height(path_x_[i], path_y_[i]);
        for (int k = 0; k < 10; ++k) {
            const float off = OFF_R[k];
            const float x = path_x_[i] + rx * off, z = path_y_[i] + rz * off;
            const float ao = fabsf(off);
            float y = hc;                                                    // droga i krawezniki plaskie w poprzek
            if (ao > CURB_HALF_R + 1.f) y = ao > SHOULDER_R + 1.f ? ground_height(x, z) : (ground_height(x, z) + hc) * 0.5f;
            ring[i * 10 + k] = road_.add_vertex({ x, y, z });
        }
    }
    const Vec3 up(0, 1, 0);
    // u w poprzek pasa: asfalt 2 teksele/jednostke na calej szerokosci (jeden kafelek na 64 jednostki), trawa: kafelek na pas
    auto u_of = [&](int k, float off) -> float {
        if (k == 3 || k == 5) return T_ASPHALT.u0 + (off + ROAD_HALF_R) / (2.f * ROAD_HALF_R) * (T_ASPHALT.u1 - T_ASPHALT.u0);
        const float ao = fabsf(off);
        const Tile& t = (k == 0 || k == 8) ? T_GRASS_DRY : T_GRASS;
        const float lo = (k == 0 || k == 8) ? SHOULDER_R : CURB_HALF_R, hi = (k == 0 || k == 8) ? FAR_R : SHOULDER_R;
        const float f = (ao - lo) / (hi - lo);
        return t.u0 + (off < 0 ? 1.f - f : f) * (t.u1 - t.u0);
    };
    constexpr int   SEG_PER_TILE = 6;
    const float     VSEG = 126.f / SEG_PER_TILE;
    const int STRIPS[6] = { 0, 1, 3, 5, 7, 8 };   // pary (k, k+1) tworzace pasy; 2, 4, 6 to luki na oznaczenia
    for (int i = 0; i < N_PATH; ++i) {
        const int n = (i + 1) % N_PATH;
        const float v0 = (i % SEG_PER_TILE) * VSEG + 1.f, v1 = v0 + VSEG - 0.5f;
        for (int s = 0; s < 6; ++s) {
            const int k = STRIPS[s];
            if ((k == 3 || k == 5) && special(i)) continue;
            const bool asphalt = (k == 3 || k == 5);
            const Tile& t = asphalt ? T_ASPHALT : ((k == 0 || k == 8) ? T_GRASS_DRY : T_GRASS);
            const float ua = u_of(k, OFF_R[k]), ub = u_of(k, OFF_R[k + 1]);
            const float uv[4][2] = { { ua, t.v0 + v0 }, { ub, t.v0 + v0 }, { ub, t.v0 + v1 }, { ua, t.v0 + v1 } };
            road_.add_quad_uv(ring[i * 10 + k], ring[i * 10 + k + 1], ring[n * 10 + k + 1], ring[n * 10 + k], up, uv, asphalt ? ASPHALT : GRASS);
        }
    }
    road_.compute_smooth_normals();
    delete[] ring;

    // --- marks_: krawezniki, linie boczne, przerywana srodkowa, szachownica startu, strzalki pol przyspieszenia ---
    const float OFF_M[8] = { -CURB_HALF_R, -ROAD_HALF_R, -LINE_IN_R, -MID_R, MID_R, LINE_IN_R, ROAD_HALF_R, CURB_HALF_R };
    marks_.init(N_PATH * 8 + 8 * 18 + 8, N_PATH * 10 + 8 * 16 + 8);
    int* mring = new int[N_PATH * 8];
    for (int i = 0; i < N_PATH; ++i) {
        float tx, tz, rx, rz;
        path_frame(i, tx, tz, rx, rz);
        const float hc = ground_height(path_x_[i], path_y_[i]);
        for (int k = 0; k < 8; ++k) mring[i * 8 + k] = marks_.add_vertex({ path_x_[i] + rx * OFF_M[k], hc, path_y_[i] + rz * OFF_M[k] });
    }
    for (int i = 0; i < N_PATH; ++i) {
        const int n = (i + 1) % N_PATH;
        const int* a = mring + i * 8;
        const int* b = mring + n * 8;
        const uint16_t curb = ((i / 3) & 1) ? CURB_RED : CURB_WHITE;
        marks_.add_quad_out(a[0], a[1], b[1], b[0], curb, up);
        marks_.add_quad_out(a[1], a[2], b[2], b[1], LINE_COL, up);
        marks_.add_quad_out(a[5], a[6], b[6], b[5], LINE_COL, up);
        marks_.add_quad_out(a[6], a[7], b[7], b[6], curb, up);
        if (!special(i)) marks_.add_quad_out(a[3], a[4], b[4], b[3], ((i / 4) & 1) ? DASH_COL : ASPHALT, up);
    }
    delete[] mring;
    for (int i = 0; i < N_PATH; ++i) {
        if (!special(i)) continue;
        const int n = (i + 1) % N_PATH;
        int a[9], b[9];
        for (int col = 0; col <= 8; ++col) {
            const float off = -LINE_IN_R + col * (2.f * LINE_IN_R / 8.f);
            float tx, tz, rx, rz;
            path_frame(i, tx, tz, rx, rz);
            a[col] = marks_.add_vertex({ path_x_[i] + rx * off, ground_height(path_x_[i], path_y_[i]), path_y_[i] + rz * off });
            path_frame(n, tx, tz, rx, rz);
            b[col] = marks_.add_vertex({ path_x_[n] + rx * off, ground_height(path_x_[n], path_y_[n]), path_y_[n] + rz * off });
        }
        const bool start = (i == 0 || i == N_PATH - 1);
        int bi = 0;
        for (int k = 0; k < 2; ++k) if (i >= TRACKS[track_].boost[k] && i < TRACKS[track_].boost[k] + 3) bi = TRACKS[track_].boost[k];
        for (int col = 0; col < 8; ++col) {
            uint16_t c;
            if (start) c = ((i + col) & 1) ? gfx::rgb565(242, 242, 242) : gfx::rgb565(26, 26, 30);
            else {
                const int d = col < 4 ? 3 - col : col - 4;                    // odleglosc od osi -> szewrony
                c = (((i - bi) + d) & 1) ? gfx::rgb565(252, 148, 28) : gfx::rgb565(255, 222, 72);
            }
            marks_.add_quad_out(a[col], a[col + 1], b[col + 1], b[col], c, up);
        }
    }
}

void KartGame::build_terrain()
{
    // siatka z pominieciem komorek pod droga (wypelnia je pobocze); kazda komorka = caly kafelek trawy (obrot losowy,
    // wyzej = sucha trawa), cieniowanie Gouraud po normalnych
    terrain_.init((TERRAIN_N + 1) * (TERRAIN_N + 1), TERRAIN_N * TERRAIN_N * 2);
    terrain_.smooth = true;
    constexpr int GW = TERRAIN_N + 1;
    int* grid = new int[GW * GW];
    for (int gz = 0; gz <= TERRAIN_N; ++gz)
        for (int gx = 0; gx <= TERRAIN_N; ++gx) {
            const float x = gx * TERRAIN_STEP, z = gz * TERRAIN_STEP;
            grid[gz * GW + gx] = terrain_.add_vertex({ x, ground_height(x, z), z });
        }
    const Vec3 up(0, 1, 0);
    for (int gz = 0; gz < TERRAIN_N; ++gz) {
        for (int gx = 0; gx < TERRAIN_N; ++gx) {
            const float cx = (gx + 0.5f) * TERRAIN_STEP, cz = (gz + 0.5f) * TERRAIN_STEP;
            float best = 1e9f;
            for (int i = 0; i < N_PATH; i += 2) {
                const float dx = path_x_[i] - cx, dz = path_y_[i] - cz;
                const float d = dx * dx + dz * dz;
                if (d < best) best = d;
            }
            if (best < (FAR_R - 30.f) * (FAR_R - 30.f)) continue;   // pod droga/poboczem
            const float h = ground_height(cx, cz);
            const Tile& t = (h + noise01(gx, gz) * 14.f > 14.f) ? T_GRASS_DRY : T_GRASS;
            const float c[4][2] = { { t.u0, t.v0 }, { t.u1, t.v0 }, { t.u1, t.v1 }, { t.u0, t.v1 } };
            const int rot = (int)(noise01(gz, gx) * 3.99f);
            float uv[4][2];
            for (int k = 0; k < 4; ++k) { uv[k][0] = c[(k + rot) & 3][0]; uv[k][1] = c[(k + rot) & 3][1]; }
            terrain_.add_quad_uv(grid[gz * GW + gx], grid[gz * GW + gx + 1], grid[(gz + 1) * GW + gx + 1], grid[(gz + 1) * GW + gx], up, uv, GRASS);
        }
    }
    terrain_.compute_smooth_normals();
    delete[] grid;
}

void KartGame::build_props()
{
    // Statyczne obiekty w wspolrzednych swiata (jedna siatka): brama startowa, trybuna, stosy opon, banery.
    props_.init(2400, 3600);
    // bryla o osiach: right (w poprzek toru), up, fwd (wzdluz toru)
    auto hexa_at = [&](const Vec3& c, float rx, float rz, float fx, float fz, float hu, float hy, float hf, uint16_t top, uint16_t side) {
        Vec3 k[8];
        for (int i = 0; i < 8; ++i) {
            const float u = (i & 1) ? hu : -hu, y = (i & 2) ? hy : -hy, f = (i & 4) ? hf : -hf;
            k[i] = { c.x + rx * u + fx * f, c.y + y, c.z + rz * u + fz * f };
        }
        props_.add_hexa(k, top, side, gfx3d::shade565(side, 0.7f));
    };
    auto at = [&](int idx, float side_off, float fwd_off, float& x, float& z, float& rx, float& rz, float& fx, float& fz) {
        path_frame(idx, fx, fz, rx, rz);
        x = path_x_[idx] + rx * side_off + fx * fwd_off;
        z = path_y_[idx] + rz * side_off + fz * fwd_off;
    };

    // --- brama startowa nad linia mety: slupy, baner z tekstura (szachownica + KART) z obu stron, belki ---
    {
        float x, z, rx, rz, fx, fz;
        at(0, 0.f, 0.f, x, z, rx, rz, fx, fz);
        const float hc = ground_height(path_x_[0], path_y_[0]);
        const uint16_t pillar = gfx::rgb565(228, 228, 236);
        for (int s = -1; s <= 1; s += 2) {
            const float px = x + rx * 47.f * s, pz = z + rz * 47.f * s;
            props_.add_cylinder_y({ px, ground_height(px, pz) - 0.5f, pz }, 1.6f, 28.f, 8, pillar);
            add_obstacle(px, pz, 2.2f);
            hexa_at({ px, ground_height(px, pz) + 0.6f, pz }, rx, rz, fx, fz, 3.2f, 0.7f, 3.2f, gfx::rgb565(60, 60, 70), gfx::rgb565(60, 60, 70));
        }
        add_sign(props_, { x, hc + 25.5f, z }, 92.f, 8.2f, fx, fz, T_GATE, gfx::rgb565(200, 200, 200));
        hexa_at({ x, hc + 34.2f, z }, rx, rz, fx, fz, 48.f, 0.7f, 1.3f, gfx::rgb565(236, 52, 52), gfx::rgb565(200, 36, 36));
        hexa_at({ x, hc + 25.f, z }, rx, rz, fx, fz, 48.f, 0.5f, 1.3f, gfx::rgb565(60, 60, 70), gfx::rgb565(50, 50, 60));
    }

    // --- trybuna po lewej stronie prostej startowej (4 stopnie, publicznosc z tekstury, dach) ---
    {
        const int gi = 9;
        float x, z, rx, rz, fx, fz;
        at(gi, 0.f, 0.f, x, z, rx, rz, fx, fz);
        const float hc = ground_height(x - rx * 90.f, z - rz * 90.f);
        for (int row = 0; row < 4; ++row) {
            const float off = -(76.f + row * 9.f);
            const float hy  = 2.4f * (row + 1);
            const Vec3  c{ x + rx * off, hc + hy, z + rz * off };
            hexa_at(c, rx, rz, fx, fz, 4.5f, hy, 36.f, gfx::rgb565(176, 178, 190), gfx::rgb565(140, 142, 156));
            for (int half = -1; half <= 1; half += 2)   // dwa kafelki publicznosci na rzad (kazdy 36 jednostek)
                add_sign(props_, { c.x + fx * 18.f * half - rx * 1.5f, hc + hy * 2.f, c.z + fz * 18.f * half - rz * 1.5f }, 36.f, 6.5f, rx, rz, T_CROWD,
                         gfx::rgb565(200, 80, 80));
        }
        for (int s = -4; s <= 4; ++s) add_obstacle(x - rx * 78.f + fx * 9.f * s, z - rz * 78.f + fz * 9.f * s, 5.f);
        const Vec3 roof{ x - rx * 92.f, hc + 30.f, z - rz * 92.f };
        hexa_at(roof, rx, rz, fx, fz, 22.f, 0.6f, 40.f, gfx::rgb565(70, 74, 92), gfx::rgb565(54, 56, 70));
        for (int s = -1; s <= 1; s += 2) {
            const float px = x - rx * 112.f + fx * 38.f * s, pz = z - rz * 112.f + fz * 38.f * s;
            props_.add_cylinder_y({ px, hc, pz }, 0.9f, 30.f, 6, gfx::rgb565(200, 200, 210));
        }
    }

    // --- stosy opon po zewnetrznej stronie ostrych zakretow ---
    int stacks = 0;
    for (int i = 0; i < N_PATH && stacks < 44; i += 2) {
        const float curv = path_curvature(i);
        if (fabsf(curv) < 0.17f) continue;
        if (i < 22 || i > N_PATH - 8) continue;   // nie przy bramie i trybunie
        const float side = curv > 0 ? -1.f : 1.f;   // kat rosnie = skret w prawo -> zewnetrzna po lewej
        float x, z, rx, rz, fx, fz;
        at(i, side * 45.f, 0.f, x, z, rx, rz, fx, fz);
        static const uint16_t TYRE_COLS[4] = { gfx::rgb565(40, 40, 46), gfx::rgb565(214, 40, 40), gfx::rgb565(40, 40, 46), gfx::rgb565(230, 230, 230) };
        const float gy = ground_height(x, z) - 0.3f;
        props_.add_cylinder_y({ x, gy, z }, 2.7f, 2.2f, 8, gfx::rgb565(40, 40, 46));           // dolna opona zawsze czarna
        props_.add_cylinder_y({ x, gy + 2.2f, z }, 2.7f, 2.2f, 8, TYRE_COLS[stacks & 3]);      // gorna: czarna / czerwona / biala
        add_obstacle(x, z, 2.7f);
        ++stacks;
    }

    // --- banery reklamowe na prostych (tekstura z obu stron) ---
    int banners = 0, last = -100;
    for (int i = 20; i < N_PATH - 10 && banners < 6; ++i) {
        if (i - last < 28) continue;
        if (fabsf(path_curvature(i)) > 0.06f) continue;
        const float side = (banners & 1) ? 1.f : -1.f;
        float x, z, rx, rz, fx, fz;
        at(i, side * 54.f, 0.f, x, z, rx, rz, fx, fz);
        const float hc = ground_height(x, z);
        for (int s = -1; s <= 1; s += 2) {
            props_.add_cylinder_y({ x + fx * 9.f * s, hc - 0.3f, z + fz * 9.f * s }, 0.6f, 7.5f, 6, gfx::rgb565(190, 190, 200));
            add_obstacle(x + fx * 9.f * s, z + fz * 9.f * s, 1.f);
        }
        add_sign(props_, { x, hc + 7.f, z }, 21.f, 5.25f, rx, rz, T_BANNER[banners % 3], BANNER_COLS[banners % 6]);
        last = i;
        ++banners;
    }
}

void KartGame::build_decor()
{
    // --- drzewa i krzaki: billboardy z atlasu (jeden prostokat obracany do kamery), cien = dysk w masce ---
    for (int k = 0; k < 4; ++k) {
        tree_bb_[k].init(8, 4);
        const float uv[4] = { T_TREE[k].u0, T_TREE[k].v0, T_TREE[k].u1, T_TREE[k].v1 };
        tree_bb_[k].add_billboard({ 0, 0, 0 }, TREE_SIZE[k], TREE_SIZE[k], 0, 1, uv, gfx::rgb565(50, 140, 60));
        tree_bb_[k].unlit = tree_bb_[k].alpha_test = true;
    }
    for (int k = 0; k < 2; ++k) {
        bush_bb_[k].init(8, 4);
        const float buv[4] = { T_BUSH[k].u0, T_BUSH[k].v0, T_BUSH[k].u1, T_BUSH[k].v1 };
        bush_bb_[k].add_billboard({ 0, 0, 0 }, 10.f, 10.f, 0, 1, buv, gfx::rgb565(50, 140, 60));
        bush_bb_[k].unlit = bush_bb_[k].alpha_test = true;
    }
    tree_shadow_m_.init(12, 10);
    tree_shadow_m_.add_disc_y({ 0, 0, 0 }, 9.f, 7.f, 8, gfx::BLACK);

    // drzewa w miejscu trybuny znikaja (trybuna stoi 70-115 jednostek na lewo od probek 0-18)
    for (int t = 0; t < N_TREES; ++t) {
        if (trees_[t].x < 0) continue;
        for (int i = 0; i <= 18; ++i) {
            float tx, tz, rx, rz;
            path_frame(i, tx, tz, rx, rz);
            const float dx = trees_[t].x - path_x_[i], dz = trees_[t].y - path_y_[i];
            const float side = dx * rx + dz * rz, along = dx * tx + dz * tz;
            if (side < -60.f && side > -125.f && fabsf(along) < 12.f) { trees_[t].x = trees_[t].y = -1000.f; break; }
        }
    }

    // --- krzaki tuz za kraweznikiem (dwa odcienie), nie w ostrych zakretach (tam stoja opony) ---
    int placed = 0;
    for (int n = 0; n < 200 && placed < N_BUSHES; ++n) {
        const int idx = (n * 37 + 11) % N_PATH;
        if (idx < 20 || idx > N_PATH - 8) continue;
        if (fabsf(path_curvature(idx)) > 0.12f) continue;
        const float side = (n & 1) ? 1.f : -1.f;
        float tx, tz, rx, rz;
        path_frame(idx, tx, tz, rx, rz);
        const float off = side * (44.f + noise01(n, 3) * 5.f);
        Bush& b = bushes_[placed++];
        b.x     = path_x_[idx] + rx * off;
        b.y     = path_y_[idx] + rz * off;
        b.kind  = (int)(noise01(n, 5) * 1.99f);
        b.scale = 0.8f + noise01(n, 7) * 0.5f;
        b.h     = surface_height(b.x, b.y);
    }
    for (int i = placed; i < N_BUSHES; ++i) bushes_[i] = { -1000.f, -1000.f, 0, 1.f, 0.f };
    for (int t = 0; t < N_TREES; ++t) {
        tree_h_[t] = trees_[t].x < 0 ? 0.f : surface_height(trees_[t].x, trees_[t].y);
        if (trees_[t].x >= 0) add_obstacle(trees_[t].x, trees_[t].y, 2.6f);   // pien
    }
}

void KartGame::build_kart_models()
{
    // Uklad lokalny gokarta: X prawo, Y gora, Z przod; podloze y = 0, dlugosc ~19, rozstaw kol ~12.
    // Kadlub = lancuch bryl scietych o wspolnych przekrojach (ogon -> kokpit -> nos -> szpic) z malowaniem z atlasu
    // na wierzchu (numer, pasy, sponsorzy), pontony z wlotami, airbox za glowa, dwa skrzydla z plytkami, wahacze do
    // kol, kierowca z barkami, karkiem, ramionami i rekawicami. Nadwozie cieniowane gladko z odblaskiem (lakier).
    const uint16_t metal = gfx::rgb565(196, 198, 208), black = gfx::rgb565(28, 28, 34), white = gfx::rgb565(242, 242, 246);
    const uint16_t carbon = gfx::rgb565(44, 46, 54);
    for (int c = 0; c < 4; ++c) {
        gfx3d::Mesh& m = kart_body_[c];
        m.init(680, 940);
        const uint16_t body = KART_COLORS[c], dark = KART_DARK[c], suit = SUIT[c];
        // malowanie: u w poprzek (64 teksele), v wzdluz: szpic (z = 9.2) -> v0, ogon (z = -9.4) -> v1
        const float lu0 = LIVERY_X0 + c * 64.f + 1.f, lu1 = lu0 + 62.f;
        auto lv = [&](float z) { return LIVERY_Y0 + 1.f + (9.2f - z) / 18.6f * 126.f; };
        auto liv = [&](float z_front, float z_back, float (&out)[4]) { out[0] = lu0; out[1] = lv(z_front); out[2] = lu1; out[3] = lv(z_back); };
        float uv[4];

        // --- kadlub ---
        liv(-6.2f, -9.4f, uv);
        m.add_loft({ 0, 2.6f, -9.4f }, 1.6f, 0.8f, { 0, 3.0f, -6.2f }, 2.9f, 1.5f, body, uv);      // ogon (pokrywa silnika)
        liv(-0.6f, -6.2f, uv);
        m.add_loft({ 0, 3.0f, -6.2f }, 2.9f, 1.5f, { 0, 3.05f, -0.6f }, 3.1f, 1.6f, body, uv);     // kokpit
        liv(6.4f, -0.6f, uv);
        m.add_loft({ 0, 3.05f, -0.6f }, 3.1f, 1.6f, { 0, 2.55f, 6.4f }, 1.5f, 0.85f, body, uv);    // nos
        liv(9.2f, 6.4f, uv);
        m.add_loft({ 0, 2.55f, 6.4f }, 1.5f, 0.85f, { 0, 2.25f, 9.2f }, 0.85f, 0.4f, body, uv);    // szpic
        m.add_box({ 0, 1.1f, -1.0f }, { 7.4f, 0.4f, 14.6f }, carbon);                           // plyta podlogowa
        m.add_box({ 0, 1.3f, -8.2f }, { 6.6f, 0.5f, 2.6f }, carbon);                            // dyfuzor (tyl)

        // --- pontony z wlotami i bialym pasem, lusterka ---
        for (int s = -1; s <= 1; s += 2) {
            const float x = s * 4.4f;
            m.add_loft({ x, 2.45f, -4.4f }, 1.2f, 1.25f, { x * 0.98f, 2.35f, 1.8f }, 1.05f, 0.95f, body);
            m.add_box({ x * 0.98f, 2.35f, 1.95f }, { 1.7f, 1.5f, 0.3f }, black);                 // wlot powietrza
            m.add_loft({ x, 3.78f, -4.3f }, 0.9f, 0.08f, { x * 0.98f, 3.38f, 1.6f }, 0.75f, 0.08f, white);
            m.add_box({ s * 3.5f, 4.7f, -0.9f }, { 0.9f, 0.45f, 0.6f }, body);                  // lusterko
            m.add_box({ s * 3.5f, 4.7f, -0.62f }, { 0.7f, 0.3f, 0.1f }, gfx::rgb565(150, 190, 230));
        }

        // --- silnik, airbox, wydechy ---
        m.add_box({ 0, 4.6f, -7.4f }, { 2.6f, 1.2f, 2.2f }, metal);
        m.add_loft({ 0, 6.0f, -6.6f }, 0.9f, 0.7f, { 0, 7.3f, -4.2f }, 1.3f, 0.95f, body);      // airbox nad glowa kierowcy
        m.add_box({ 0, 7.3f, -4.1f }, { 1.8f, 1.3f, 0.3f }, black);                             // wlot airboxu
        add_rod(m, { -1.5f, 4.2f, -7.6f }, { -2.3f, 4.5f, -9.9f }, 0.42f, metal);
        add_rod(m, { 1.5f, 4.2f, -7.6f }, { 2.3f, 4.5f, -9.9f }, 0.42f, metal);

        // --- tylne skrzydlo (dwa platy, plytki, dwa srodkowe slupki) ---
        m.add_box({ 0, 7.7f, -8.8f }, { 11.0f, 0.36f, 2.4f }, body);
        m.add_box({ 0, 6.4f, -9.3f }, { 9.0f, 0.3f, 1.5f }, dark);
        m.add_box({ -5.6f, 7.1f, -8.8f }, { 0.3f, 2.6f, 3.0f }, white);
        m.add_box({ 5.6f, 7.1f, -8.8f }, { 0.3f, 2.6f, 3.0f }, white);
        m.add_box({ -1.4f, 6.4f, -8.3f }, { 0.4f, 2.4f, 0.9f }, dark);
        m.add_box({ 1.4f, 6.4f, -8.3f }, { 0.4f, 2.4f, 0.9f }, dark);

        // --- przednie skrzydlo (plat + klapa + plytki) ---
        m.add_box({ 0, 1.35f, 9.6f }, { 11.4f, 0.34f, 1.9f }, body);
        m.add_box({ 0, 1.85f, 8.9f }, { 9.6f, 0.3f, 0.9f }, white);
        m.add_box({ -5.75f, 2.0f, 9.5f }, { 0.3f, 1.5f, 2.5f }, white);
        m.add_box({ 5.75f, 2.0f, 9.5f }, { 0.3f, 1.5f, 2.5f }, white);

        // --- zawieszenie: wahacze do piast (pozycje kol jak w draw_kart_3d) ---
        for (int s = -1; s <= 1; s += 2) {
            const float x = (float)s;
            add_rod(m, { x * 2.4f, 2.9f, 4.7f }, { x * 4.9f, 2.1f, 5.6f }, 0.2f, carbon);        // przod: widelec
            add_rod(m, { x * 2.4f, 2.9f, 6.6f }, { x * 4.9f, 2.1f, 5.6f }, 0.2f, carbon);
            add_rod(m, { x * 2.4f, 2.2f, 5.6f }, { x * 4.9f, 1.9f, 5.6f }, 0.16f, metal);       // drazek kierowniczy
            add_rod(m, { x * 2.6f, 3.0f, -5.4f }, { x * 5.1f, 2.4f, -6.4f }, 0.25f, carbon);     // tyl
            add_rod(m, { x * 2.6f, 3.0f, -7.4f }, { x * 5.1f, 2.4f, -6.4f }, 0.25f, carbon);
        }
        add_rod(m, { -5.1f, 2.4f, -6.4f }, { 5.1f, 2.4f, -6.4f }, 0.3f, metal);                 // os tylna

        // --- kierowca ---
        m.add_box({ 0, 4.5f, -4.5f }, { 3.8f, 2.6f, 0.7f }, black);                             // oparcie fotela
        m.add_loft({ 0, 4.9f, -3.9f }, 1.6f, 0.9f, { 0, 5.3f, -1.4f }, 1.7f, 1.2f, suit);       // tulow (od tylu do przodu)
        m.add_box({ 0, 6.35f, -2.7f }, { 4.4f, 0.8f, 1.9f }, suit);                             // barki
        m.add_box({ 0, 6.9f, -2.6f }, { 1.1f, 0.5f, 1.1f }, gfx::rgb565(214, 170, 140));        // kark
        add_tube(m, { -2.0f, 6.3f, -2.3f }, { -1.5f, 5.85f, 1.3f }, 0.46f, 8, suit);            // ramiona (okragle)
        add_tube(m, { 2.0f, 6.3f, -2.3f }, { 1.5f, 5.85f, 1.3f }, 0.46f, 8, suit);
        m.add_sphere({ -1.5f, 5.85f, 1.6f }, 0.58f, 8, 5, dark);                                  // rekawice (kule)
        m.add_sphere({ 1.5f, 5.85f, 1.6f }, 0.58f, 8, 5, dark);
        m.add_box({ 0, 5.85f, 1.9f }, { 3.4f, 1.5f, 0.4f }, black);                              // kierownica
        m.add_box({ 0, 5.85f, 1.9f }, { 0.9f, 0.5f, 0.45f }, metal);                             // srodek kierownicy
        add_tube(m, { 0, 5.5f, 2.0f }, { 0, 4.4f, 3.6f }, 0.24f, 6, metal);                     // kolumna
        m.smooth   = true;      // normalne usrednione w narozach = zaokraglone cieniowanie lakieru
        m.specular = 0.38f;
        m.compute_smooth_normals();

        gfx3d::Mesh& h = helmet_[c];
        h.init(210, 410);
        // elipsoida lekko wydluzona do przodu; wizjer ciemny z odblaskiem, pas w kolorze bolidu (add_helmet)
        add_helmet(h, { 0, 8.3f, -2.45f }, { 1.95f, 2.0f, 2.25f }, HELMET[c], body, gfx::rgb565(20, 26, 44));
        h.smooth = true;
        h.specular = 0.85f;
        h.compute_smooth_normals();
    }
    // opona prawie czarna (bieznik z tekstury), felga ciemny grafit (jasny kapsel liczy sie w add_wheel)
    const uint16_t tyre = gfx::rgb565(22, 22, 26), hub = gfx::rgb565(88, 90, 100);
    const float tread[4] = { T_TREAD.u0, T_TREAD.v0, T_TREAD.u1, T_TREAD.v1 };
    // 16 bokow (bylo 12 - obrys opony byl widocznie kanciasty z bliska)
    wheel_front_.init(150, 220);
    wheel_front_.add_wheel({ 0, 0, 0 }, 2.1f, 2.6f, 16, tyre, hub, 0.6f, tread);
    wheel_rear_.init(150, 220);
    wheel_rear_.add_wheel({ 0, 0, 0 }, 2.4f, 3.4f, 16, tyre, hub, 0.58f, tread);
}

// ============================================================================ scena

void KartGame::draw_kart_3d(int index)
{
    // Kazde kolo stoi na wysokosci nawierzchni w SWOIM punkcie (nie schodzi pod asfalt z Z-buforem), nadwozie bierze
    // pochylenie i przechyl z czterech kol; przechyl "kosmetyczny" w zakretach obraca tylko nadwozie wokol osi kol.
    const Kart& k = karts_[index];
    float angle = k.angle;
    if (k.spin_t > 0) angle += k.spin_t * 12.f;   // wirowanie
    const float hop = k.spin_t > 0 ? sinf(k.spin_t * 5.7f) * 2.5f : 0.f;
    const float fx = cosf(angle), fz = sinf(angle), rx = -fz, rz = fx;

    // pozycje kol w swiecie (XZ z kierunku jazdy) i wysokosc nawierzchni pod kazdym
    static constexpr float WX[4] = { -6.2f, 6.2f, -6.8f, 6.8f }, WZ[4] = { 5.6f, 5.6f, -6.4f, -6.4f }, WR[4] = { 2.1f, 2.1f, 2.4f, 2.4f };
    float wx[4], wz[4], wh[4];
    for (int w = 0; w < 4; ++w) {
        wx[w] = k.x + rx * WX[w] + fx * WZ[w];
        wz[w] = k.y + rz * WX[w] + fz * WZ[w];
        wh[w] = surface_height(wx[w], wz[w]);
    }
    const float front = (wh[0] + wh[1]) * 0.5f, rear = (wh[2] + wh[3]) * 0.5f;
    const float left  = (wh[0] + wh[2]) * 0.5f, right = (wh[1] + wh[3]) * 0.5f;
    const float pitch  = atanf((front - rear) / 12.f);
    const float roll_g = atanf((right - left) / 13.f);
    const float gy     = (front + rear) * 0.5f + 0.05f;   // srodek nadwozia: srednia z kol (+ margines na styk kola z droga)
    float roll_c = engine::clampf(yaw_rate_[index] * 0.06f, -0.09f, 0.09f);
    if (k.drifting) roll_c += (yaw_rate_[index] > 0 ? 0.05f : -0.05f);

    const Mat4 body_tilt = Mat4::translation({ 0, 2.3f, 0 }) * Mat4::rotation_z(roll_c) * Mat4::translation({ 0, -2.3f, 0 });
    const Mat4 base = Mat4::translation({ k.x, gy + hop, k.y }) * Mat4::heading(angle) * Mat4::rotation_x(-pitch) * Mat4::rotation_z(roll_g) * body_tilt;
    const Mat4 spin  = Mat4::rotation_x(-wheel_spin_[index]);
    const Mat4 steer = Mat4::rotation_y(steer_vis_[index] * 0.42f);
    Mat4 wm[4];
    for (int w = 0; w < 4; ++w) {
        wm[w] = Mat4::translation({ wx[w], wh[w] + WR[w] + 0.05f + hop, wz[w] }) * Mat4::heading(angle) * Mat4::rotation_x(-pitch);
        if (w < 2) wm[w] = wm[w] * steer;
        wm[w] = wm[w] * spin;
    }

    // cien rzutowany wzdluz slonca na plaszczyzne podloza pod gokartem (maska -> przyciemnienie po warstwie 0)
    if (quality_ < 2) {
        const Mat4 sh = Mat4::shadow_onto_plane(sun_dir(), gy + 0.1f);
        r3d_.draw_shadow(kart_body_[k.color & 3], sh * base);
        for (int w = 0; w < 4; ++w) r3d_.draw_shadow(w < 2 ? wheel_front_ : wheel_rear_, sh * wm[w]);
    } else {
        r3d_.draw_shadow(shadow_m_, Mat4::translation({ k.x, gy + 0.2f, k.y }) * Mat4::heading(k.angle));
    }
    r3d_.draw_mesh(kart_body_[k.color & 3], base, 1);
    r3d_.draw_mesh(helmet_[k.color & 3], base, 1);
    for (int w = 0; w < 4; ++w) r3d_.draw_mesh(w < 2 ? wheel_front_ : wheel_rear_, wm[w], 1);
}

void KartGame::draw_scene(gfx::Canvas& c)
{
    const Kart& p = karts_[0];
    const float kart_y = surface_height(p.x, p.y);
    gfx3d::Camera cam;
    const char* view = getenv("KART_CAM");   // podglad modelu (narzedzie): "kat_stopnie,odleglosc,wysokosc"
    if (view) {
        float ang = 30.f, dist = 22.f, hgt = 8.f;
        sscanf(view, "%f,%f,%f", &ang, &dist, &hgt);
        const float a = p.angle + ang * 0.0174533f;
        const float h0 = surface_height(p.x, p.y);
        cam.pos    = { p.x + cosf(a) * dist, h0 + hgt, p.y + sinf(a) * dist };
        cam.target = { p.x, h0 + 4.5f, p.y };
        cam.fov_y  = 0.8f;
        fov_vis_   = 0.8f;
    } else if (state_ == State::Title) {
        // ekran tytulowy: kamera kolysze sie za polami startowymi (nie wjezdza w gokarty ani w brame)
        const float a  = p.angle + 3.14159265f + sinf(anim_ * 0.4f) * 0.9f;
        const float cx = (p.x + karts_[1].x) * 0.5f, cz = (p.y + karts_[1].y) * 0.5f;
        cam.pos    = { cx + cosf(a) * 54.f, kart_y + 21.f, cz + sinf(a) * 54.f };
        cam.target = { cx + cosf(p.angle) * 10.f, kart_y + 3.f, cz + sinf(p.angle) * 10.f };
        cam.fov_y  = 0.95f;
        fov_vis_   = 0.95f;
    } else {
        const float fx = cosf(cam_angle_), fz = sinf(cam_angle_);
        cam.pos    = { p.x - fx * 45.f, kart_y + 18.f, p.y - fz * 45.f };
        const float cam_ground = surface_height(cam.pos.x, cam.pos.z);
        if (cam.pos.y < cam_ground + 7.f) cam.pos.y = cam_ground + 7.f;
        cam.target = { p.x + fx * 30.f, kart_y + 4.5f, p.y + fz * 30.f };
        // szerszy kat przy turbo (wrazenie predkosci), wygladzany miedzy klatkami
        const float fov_goal = 1.0f + (p.boost_t > 0 ? 0.16f : 0.f) + engine::clampf((p.speed - 120.f) / 600.f, 0.f, 0.06f);
        fov_vis_ += (fov_goal - fov_vis_) * 0.12f;
        cam.fov_y = fov_vis_;
    }

    gfx3d::Light light;
    light.dir     = sun_dir();   // zgodnie z tarcza slonca na niebie
    light.ambient = 0.46f;
    light.diffuse = 0.74f;
    gfx3d::Fog fog;
    fog.color = fog_col_;
    fog.start = quality_ == 0 ? 420.f : 300.f;
    fog.end   = quality_ == 0 ? 1300.f : (quality_ == 1 ? 900.f : 600.f);
    r3d_.set_flat_only(quality_ >= 1);
    r3d_.enable_zbuffer(quality_ == 0 && !getenv("KART_NOZ"));   // KART_NOZ: pomiar kosztu Z-bufora na PC

    r3d_.begin(c, cam, light, fog);
    draw_sky(c, r3d_.horizon_y());

    // warstwa 0: podloze, oznaczenia, slady opon; cienie do maski (przyciemnienie po warstwie 0)
    r3d_.draw_mesh(terrain_, Mat4::identity(), 0, false);
    r3d_.draw_mesh(road_, Mat4::identity(), 0, false);
    r3d_.draw_mesh(marks_, Mat4::identity(), 0, false);
    for (const Skid& s : skids_) {
        if (!s.alive) continue;
        const uint16_t col = gfx3d::mix565(gfx::rgb565(34, 34, 40), ASPHALT, s.age / 30.f);
        const Vec3 q[4] = { { s.x[0], s.h[0], s.y[0] }, { s.x[1], s.h[1], s.y[1] }, { s.x[2], s.h[2], s.y[2] }, { s.x[3], s.h[3], s.y[3] } };
        r3d_.draw_tri(q[0], q[1], q[2], col, 0, false, 30.f);
        r3d_.draw_tri(q[0], q[2], q[3], col, 0, false, 30.f);
    }
    const float shx = -cosf(SUN_ANGLE) * 5.f, shz = -sinf(SUN_ANGLE) * 5.f;
    const float near2 = quality_ == 0 ? 520.f * 520.f : 300.f * 300.f;
    if (quality_ < 2)
        for (int i = 0; i < N_TREES; ++i) {
            const Tree& t = trees_[i];
            if (t.x < 0) continue;
            const float dx = t.x - cam.pos.x, dz = t.y - cam.pos.z;
            if (dx * dx + dz * dz > near2) continue;
            const float s = 0.85f + noise01(i, 9) * 0.4f;
            r3d_.draw_shadow(tree_shadow_m_, Mat4::translation({ t.x + shx * s, tree_h_[i] + 0.15f, t.y + shz * s }) * Mat4::scale(s, 1.f, s));
        }

    // warstwa 1: obiekty (billboardy obracane do kamery)
    r3d_.draw_mesh(props_, Mat4::identity(), 1);
    for (int i = 0; i < N_TREES; ++i) {
        const Tree& t = trees_[i];
        if (t.x < 0) continue;
        const float s = 0.85f + noise01(i, 9) * 0.4f;
        const float a = atan2f(cam.pos.z - t.y, cam.pos.x - t.x);
        // rodzaj: kind (0/1 z logiki gry, nie ruszamy - slady testow) + druga para z szumu po indeksie
        const int kind = (t.kind & 1) + (noise01(i, 13) > 0.5f ? 2 : 0);
        r3d_.draw_mesh(tree_bb_[kind], Mat4::translation({ t.x, tree_h_[i] - 0.3f, t.y }) * Mat4::heading(a) * Mat4::scale(s, s, s), 1, false);
    }
    if (quality_ < 2)
        for (int i = 0; i < N_BUSHES; ++i) {
            const Bush& b = bushes_[i];
            if (b.x < 0) continue;
            const float dx = b.x - cam.pos.x, dz = b.y - cam.pos.z;
            if (dx * dx + dz * dz > near2) continue;
            const float a = atan2f(cam.pos.z - b.y, cam.pos.x - b.x);
            r3d_.draw_mesh(bush_bb_[b.kind & 1], Mat4::translation({ b.x, b.h - 0.3f, b.y }) * Mat4::heading(a) * Mat4::scale(b.scale, b.scale * 0.8f, b.scale), 1, false);
        }
    for (int i = 0; i < N_BOXES; ++i) {
        if (boxes_[i].respawn > 0) continue;
        const float bob = sinf(anim_ * 3.f + i * 1.3f) * 1.2f;
        const float gy  = surface_height(boxes_[i].x, boxes_[i].y);
        r3d_.draw_mesh(itembox_m_, Mat4::translation({ boxes_[i].x, gy + 6.5f + bob, boxes_[i].y }) * Mat4::rotation_y(anim_ * 1.6f + i) * Mat4::rotation_x(0.35f), 1);
    }
    for (const Hazard& h : hazards_) {
        if (!h.alive) continue;
        r3d_.draw_mesh(banana_m_, Mat4::translation({ h.x, surface_height(h.x, h.y), h.y }) * Mat4::scale(1.4f, 0.75f, 1.f), 1);
    }
    for (const Shell& s : shells_) {
        if (!s.alive) continue;
        r3d_.draw_mesh(shell_m_, Mat4::translation({ s.x, surface_height(s.x, s.y), s.y }) * Mat4::rotation_y(anim_ * 14.f), 1);
    }
    for (int i = 0; i < N_KARTS; ++i) draw_kart_3d(i);
    r3d_.end();

    draw_effects(c);
}

// ============================================================================ niebo i efekty 2D

void KartGame::draw_strip(gfx::Canvas& c, const gfx::Image& strip, int top_row, float angle_offset, bool opaque_only)
{
    if (!strip.px) return;
    uint16_t* px = c.data();
    if (top_row >= H) return;
    const float inv_pi2 = 1.f / 6.28318530718f;
    const float focal = (H / 2.f) / tanf(fov_vis_ * 0.5f);
    const float base_angle = state_ == State::Title ? karts_[0].angle + sinf(anim_ * 0.4f) * 0.9f : cam_angle_;
    for (int x = 0; x < W; ++x) {
        const float a   = base_angle + atanf((float)(x - W / 2) / focal) + angle_offset;
        int         col = (int)floorf(a * inv_pi2 * strip.w) % strip.w;
        if (col < 0) col += strip.w;
        const uint16_t* sp = strip.px + col;
        const uint8_t*  sa = strip.alpha + col;
        for (int ry = 0; ry < strip.h; ++ry) {
            const int y = top_row + ry;
            if (y < 0 || y >= H) continue;
            const int a8 = sa[(size_t)ry * strip.w];
            if (a8 == 0) continue;
            uint16_t& d = px[(size_t)y * W + x];
            if (opaque_only) { if (a8 >= 128) d = sp[(size_t)ry * strip.w]; }
            else d = blend565(d, sp[(size_t)ry * strip.w], a8);
        }
    }
}

void KartGame::draw_sky(gfx::Canvas& c, float horizon)
{
    uint16_t* px = c.data();
    const int hz = engine::iclamp((int)horizon, 1, H - 1);
    for (int y = 0; y < H; ++y) {
        uint16_t col;
        if (y >= hz) col = fog_col_;
        else {
            // trzy progi: glebokie niebo u gory, blekit w polowie, mgielka przy horyzoncie
            const int t = y * 64 / hz;
            col = t < 32 ? mix565i(sky_top_, sky_mid_, t) : mix565i(sky_mid_, sky_horiz_, t - 32);
        }
        uint32_t* row = reinterpret_cast<uint32_t*>(px + (size_t)y * W);
        const uint32_t v = ((uint32_t)col << 16) | col;
        for (int x = 0; x < W / 2; ++x) row[x] = v;
    }
    const float base_angle = state_ == State::Title ? karts_[0].angle + sinf(anim_ * 0.4f) * 0.9f : cam_angle_;
    {   // slonce z poswiata - w kierunku swiatla sceny
        const float rel = wrap_angle_static(SUN_ANGLE - base_angle);
        if (fabsf(rel) < 1.2f) {
            const float focal = (H / 2.f) / tanf(fov_vis_ * 0.5f);
            const float sx = W / 2 + tanf(rel) * focal;
            const float sy = hz - 150.f;
            if (glow_.px) draw_image(c, glow_, glow_.w, 0, sx, sy + glow_.h / 2.f, 1.3f, 255, false, false);
            c.fill_circle((int)sx, (int)sy, 24, gfx::rgb565(255, 252, 232));
        }
    }
    if (quality_ == 0) draw_strip(c, clouds_, hz - 240, anim_ * 0.012f, false);
    draw_strip(c, mountains_, hz - mountains_.h + 2, 0.f, quality_ >= 2);
}

void KartGame::draw_image(gfx::Canvas& c, const gfx::Image& img, int frame_w, int frame, float cx, float bottom, float scale,
                          int alpha_mul, bool flip, bool filtered)
{
    if (!img.px || scale <= 0.005f) return;
    const int dw = (int)(frame_w * scale + 0.5f), dh = (int)(img.h * scale + 0.5f);
    if (dw <= 0 || dh <= 0) return;
    const int x0 = (int)floorf(cx - dw / 2.f), y0 = (int)floorf(bottom - dh);
    const int cx0 = engine::imax(x0, 0), cy0 = engine::imax(y0, 0), cx1 = engine::imin(x0 + dw, W), cy1 = engine::imin(y0 + dh, H);
    if (cx0 >= cx1 || cy0 >= cy1) return;
    uint16_t*      px    = c.data();
    const int      fx0   = frame * frame_w;
    const uint32_t stepx = (uint32_t)(((int64_t)frame_w << 16) / dw), stepy = (uint32_t)(((int64_t)img.h << 16) / dh);
    const bool     bil   = filtered && quality_ == 0 && (scale > 1.05f || scale < 0.95f);
    const int      maxu  = frame_w - 1, maxv = img.h - 1;
    uint32_t ty = (uint32_t)(cy0 - y0) * stepy;
    for (int y = cy0; y < cy1; ++y, ty += stepy) {
        uint16_t* drow = px + (size_t)y * W;
        uint32_t  tx   = (uint32_t)(cx0 - x0) * stepx;
        const int v    = engine::imin((int)(ty >> 16), maxv);
        const int fv   = (int)((ty >> 8) & 255);
        const int v1   = engine::imin(v + 1, maxv);
        for (int x = cx0; x < cx1; ++x, tx += stepx) {
            int u = engine::imin((int)(tx >> 16), maxu);
            if (flip) u = maxu - u;
            if (!bil) {
                const size_t i = (size_t)v * img.w + fx0 + u;
                int a = img.alpha[i];
                if (!a) continue;
                a = a * alpha_mul >> 8;
                drow[x] = blend565(drow[x], img.px[i], a);
            } else {
                const int fu = flip ? 255 - (int)((tx >> 8) & 255) : (int)((tx >> 8) & 255);
                const int u1 = flip ? engine::imax(u - 1, 0) : engine::imin(u + 1, maxu);
                const size_t i00 = (size_t)v * img.w + fx0 + u, i10 = (size_t)v * img.w + fx0 + u1;
                const size_t i01 = (size_t)v1 * img.w + fx0 + u, i11 = (size_t)v1 * img.w + fx0 + u1;
                const int w00 = (256 - fu) * (256 - fv), w10 = fu * (256 - fv), w01 = (256 - fu) * fv, w11 = fu * fv;
                const int a00 = img.alpha[i00] * w00, a10 = img.alpha[i10] * w10, a01 = img.alpha[i01] * w01, a11 = img.alpha[i11] * w11;
                const int asum = a00 + a10 + a01 + a11;
                if (!asum) continue;
                const uint16_t c00 = img.px[i00], c10 = img.px[i10], c01 = img.px[i01], c11 = img.px[i11];
                const int r = (((c00 >> 11) & 31) * a00 + ((c10 >> 11) & 31) * a10 + ((c01 >> 11) & 31) * a01 + ((c11 >> 11) & 31) * a11) / asum;
                const int g = (((c00 >> 5) & 63) * a00 + ((c10 >> 5) & 63) * a10 + ((c01 >> 5) & 63) * a01 + ((c11 >> 5) & 63) * a11) / asum;
                const int b = ((c00 & 31) * a00 + (c10 & 31) * a10 + (c01 & 31) * a01 + (c11 & 31) * a11) / asum;
                int a = asum >> 16;
                a = a * alpha_mul >> 8;
                drow[x] = blend565(drow[x], pack565(r, g, b), a);
            }
        }
    }
}

void KartGame::draw_effects(gfx::Canvas& c)
{
    // dym, kurz i plomienie: obrazy z alfa rzutowane przez kamere 3D, sortowane od najdalszych
    struct E { float sx, sy, depth; int kind, index; };
    E list[N_PUFFS + N_KARTS];
    int n = 0;
    for (int i = 0; i < N_PUFFS; ++i) {
        if (!puffs_[i].alive) continue;
        float sx, sy, d;
        const Puff& p = puffs_[i];
        if (r3d_.project({ p.x, surface_height(p.x, p.y) + 2.f + p.size * 0.35f, p.y }, sx, sy, d)) list[n++] = { sx, sy, d, 0, i };
    }
    for (int i = 0; i < N_KARTS; ++i) {
        const Kart& k = karts_[i];
        if (k.boost_t <= 0) continue;
        float sx, sy, d;
        const float x = k.x - cosf(k.angle) * 9.8f, z = k.y - sinf(k.angle) * 9.8f;
        if (r3d_.project({ x, surface_height(k.x, k.y) + 3.6f, z }, sx, sy, d)) list[n++] = { sx, sy, d, 1, i };
    }
    for (int i = 1; i < n; ++i) {
        E e = list[i];
        int j = i - 1;
        while (j >= 0 && list[j].depth < e.depth) { list[j + 1] = list[j]; --j; }
        list[j + 1] = e;
    }
    const float focal = (H / 2.f) / tanf(fov_vis_ * 0.5f);
    for (int i = 0; i < n; ++i) {
        const E& e = list[i];
        const float pxu = focal / e.depth;
        if (e.kind == 0) {
            const Puff& p = puffs_[e.index];
            const float k = 1.f - p.t / p.life;
            const float scale = pxu * p.size / 64.f;
            const int   a = (int)((p.kind == 0 ? 190.f : 120.f) * k);
            draw_image(c, smoke_, smoke_.w, 0, e.sx, e.sy + 32.f * scale, scale, a);
        } else {
            const int frame = ((int)(anim_ * 24.f)) & 1;
            draw_image(c, flame_, 32, frame, e.sx, e.sy + 4.f * pxu, pxu * 7.f / 64.f, 230);
        }
    }
    // iskry driftu (ekran) - zrodlo przy tylnych kolach gracza
    const Kart& p = karts_[0];
    if (p.drifting && ((int)(anim_ * 60)) % 2 == 0) {
        const uint16_t col = p.drift_t > 1.4f ? gfx::rgb565(120, 170, 255) : (p.drift_t > 0.7f ? gfx::rgb565(255, 150, 40) : gfx::rgb565(255, 230, 120));
        float sx, sy, d;
        const float rx = -sinf(p.angle), rz = cosf(p.angle);
        const float ph = surface_height(p.x, p.y) + 1.f;
        if (r3d_.project({ p.x - rx * 6.5f - cosf(p.angle) * 5.f, ph, p.y - rz * 6.5f - sinf(p.angle) * 5.f }, sx, sy, d))
            add_spark(sx, sy, -60.f - (float)(((int)(anim_ * 1000)) % 50), -90.f, 0.3f, col);
        if (r3d_.project({ p.x + rx * 6.5f - cosf(p.angle) * 5.f, ph, p.y + rz * 6.5f - sinf(p.angle) * 5.f }, sx, sy, d))
            add_spark(sx, sy, 60.f + (float)(((int)(anim_ * 777)) % 50), -90.f, 0.3f, col);
    }
    for (const Spark& s : sparks_) {
        if (s.t <= 0) continue;
        c.fill_rect((int)s.x, (int)s.y, 3, 3, s.color);
    }
}

// ============================================================================ HUD

void KartGame::draw_minimap(gfx::Canvas& c)
{
    const int size = 124, x0 = W - size - 22, y0 = H - size - 24;
    hud_panel(c, x0 - 10, y0 - 10, size + 20, size + 20, 150);
    const float k = (float)size / WORLD;
    for (int i = 0; i < N_PATH; ++i) c.fill_circle(x0 + (int)(path_x_[i] * k), y0 + (int)(path_y_[i] * k), 4, gfx::rgb565(50, 56, 76));
    for (int i = 0; i < N_PATH; ++i) c.fill_circle(x0 + (int)(path_x_[i] * k), y0 + (int)(path_y_[i] * k), 2, gfx::rgb565(200, 206, 222));
    {   // linia startu w poprzek
        float tx, tz, rx, rz;
        path_frame(0, tx, tz, rx, rz);
        const int mx = x0 + (int)(path_x_[0] * k), my = y0 + (int)(path_y_[0] * k);
        c.line(mx - (int)(rx * 6), my - (int)(rz * 6), mx + (int)(rx * 6), my + (int)(rz * 6), gfx::pal::WHITE);
    }
    for (int i = N_KARTS - 1; i >= 0; --i) {
        const int mx = x0 + (int)(karts_[i].x * k), my = y0 + (int)(karts_[i].y * k);
        c.fill_circle(mx, my, i == 0 ? 7 : 5, gfx::rgb565(10, 12, 20));
        c.fill_circle(mx, my, i == 0 ? 5 : 3, KART_COLORS[karts_[i].color & 3]);
        if (i == 0) c.fill_circle(mx, my, 2, gfx::pal::WHITE);
    }
}

void KartGame::draw_hud_title(gfx::Canvas& c)
{
    auto center = [&](int y, const char* s, uint16_t col, int px) {
        text_shadow(c, (W - gfx::text_width_px(s, px)) / 2, y, s, col, px);
    };
    hud_panel(c, W / 2 - 250, 22, 500, 150, 190);
    c.fill_rect(W / 2 - 250 + 24, 96, 452, 2, gfx::pal::YELLOW);
    center(28, "KART", gfx::pal::YELLOW, 48);
    center(106, "3 okrazenia  |  3 rywali  |  grzyb, banan, skorupa", gfx::pal::WHITE, 16);
    center(134, "drift z mini-turbo  |  pola przyspieszenia", TEXT_DIM, 16);
    hud_panel(c, W / 2 - 300, 186, 600, 34, 150);
    center(194, "A gaz    strzalki skret    B drift    X przedmiot    dol hamulec    Y autopilot", TEXT_DIM, 14);

    // wybor toru: nazwa, motyw, rekord okrazenia, strzalki
    char buf[64];
    hud_panel(c, W / 2 - 230, 236, 460, 92, 175);
    snprintf(buf, sizeof(buf), "TOR %d / %d", track_ + 1, TRACK_COUNT);
    center(244, buf, TEXT_DIM, 14);
    center(262, TRACKS[track_].name, gfx::pal::WHITE, 28);
    if (rec_lap_[track_] > 0)
        snprintf(buf, sizeof(buf), "motyw: %s   |   rekord okr\u0105\u017cenia %.2f s", THEMES[TRACKS[track_].theme].name, (double)rec_lap_[track_]);
    else snprintf(buf, sizeof(buf), "motyw: %s   |   brak rekordu", THEMES[TRACKS[track_].theme].name);
    center(298, buf, gfx::pal::YELLOW, 14);
    const int ay = 266;
    for (int s = -1; s <= 1; s += 2) {   // strzalki < >
        const int ax = W / 2 + s * 200;
        for (int i = 0; i < 12; ++i) c.vline(ax + s * i, ay + i, 24 - 2 * i, gfx::pal::YELLOW);   // grot na zewnatrz
    }
    if (fmodf(anim_, 1.f) < 0.65f) {
        fill_round_rect_alpha(c, W / 2 - 90, H - 62, 180, 44, 12, PANEL, 170);
        center(H - 56, "Wcisnij A", gfx::pal::WHITE, 28);
    }
}

void KartGame::draw_hud_race(gfx::Canvas& c)
{
    char buf[80];
    const Kart& p = karts_[0];
    auto center = [&](int y, const char* s, uint16_t col, int px) {
        text_shadow(c, (W - gfx::text_width_px(s, px)) / 2, y, s, col, px);
    };

    // --- lewy gorny: miejsce, okrazenie, czas ---
    hud_panel(c, 16, 14, 262, 86);
    static const uint16_t PLACE_COL[4] = { gfx::rgb565(255, 208, 40), gfx::rgb565(200, 206, 220), gfx::rgb565(214, 140, 70), gfx::rgb565(150, 160, 185) };
    const int pl = engine::iclamp(p.place - 1, 0, 3);
    c.fill_circle(58, 57, 32, gfx::rgb565(6, 8, 14));
    c.fill_circle(58, 57, 30, PLACE_COL[pl]);
    snprintf(buf, sizeof(buf), "%d", pl + 1);
    gfx::draw_text_px(c, 58 - gfx::text_width_px(buf, 40) / 2, 57 - gfx::text_height_px(40) / 2 + 1, buf, gfx::rgb565(20, 22, 34), 40);
    gfx::draw_text_px(c, 106, 22, "OKRAZENIE", TEXT_DIM, 12);
    snprintf(buf, sizeof(buf), "%d / %d", engine::imin(p.lap, LAPS), LAPS);
    text_shadow(c, 106, 36, buf, gfx::pal::WHITE, 28);
    const int cs = (int)(race_t_ * 100) % 100, sec = (int)race_t_ % 60, mn = (int)race_t_ / 60;
    snprintf(buf, sizeof(buf), "%d:%02d.%02d", mn, sec, cs);
    gfx::draw_text_px(c, 190, 22, "CZAS", TEXT_DIM, 12);
    gfx::draw_text_px(c, 190, 38, buf, gfx::pal::WHITE, 16);
    if (best_lap_ > 0) {
        snprintf(buf, sizeof(buf), "najlepsze %.2f", (double)best_lap_);
        gfx::draw_text_px(c, 190, 62, buf, gfx::rgb565(140, 210, 150), 14);
    }

    // --- prawy gorny: przedmiot ---
    hud_panel(c, W - 104, 14, 88, 88);
    const uint16_t frame = p.item != NONE ? gfx::pal::YELLOW : gfx::rgb565(60, 70, 95);
    fill_round_rect_alpha(c, W - 96, 22, 72, 72, 10, frame, p.item != NONE ? 200 : 90);
    fill_round_rect_alpha(c, W - 93, 25, 66, 66, 8, PANEL, 220);
    const gfx::Image* icon = p.item == MUSHROOM ? &mushroom_ : (p.item == BANANA ? &banana_ : (p.item == SHELL ? &shell_ : nullptr));
    if (icon && icon->px) draw_image(c, *icon, icon->w, 0, W - 60, 58 + 29, 1.2f);
    else if (p.item == NONE) gfx::draw_text_px(c, W - 60 - gfx::text_width_px("?", 28) / 2, 58 - gfx::text_height_px(28) / 2, "?", gfx::rgb565(50, 58, 80), 28);

    // --- prawy: kolejnosc (kolorowe kropki) ---
    hud_panel(c, W - 104, 112, 88, 4 * 26 + 14, 170);
    for (int place = 1; place <= N_KARTS; ++place)
        for (int i = 0; i < N_KARTS; ++i) {
            if (karts_[i].place != place) continue;
            const int y = 120 + (place - 1) * 26;
            snprintf(buf, sizeof(buf), "%d.", place);
            gfx::draw_text_px(c, W - 92, y + 2, buf, i == 0 ? gfx::pal::YELLOW : TEXT_DIM, 16);
            c.fill_circle(W - 44, y + 12, 9, gfx::rgb565(6, 8, 14));
            c.fill_circle(W - 44, y + 12, 7, KART_COLORS[karts_[i].color & 3]);
            if (i == 0) c.fill_circle(W - 44, y + 12, 3, gfx::pal::WHITE);
        }

    // --- lewy dolny: predkosc (pasek z gradientem + liczba) ---
    const int bar_w = 232, bar_x = 30, bar_y = H - 46;
    hud_panel(c, 16, H - 74, 336, 58);
    const float frac = engine::clampf(fabsf(p.speed) / 225.f, 0.f, 1.f);
    const int fill = (int)(bar_w * frac);
    fill_round_rect_alpha(c, bar_x, bar_y, bar_w, 14, 6, gfx::rgb565(36, 42, 60), 255);
    for (int x = 0; x < fill; ++x) {
        const int t = x * 64 / bar_w;
        uint16_t col = t < 32 ? mix565i(gfx::rgb565(70, 210, 110), gfx::rgb565(250, 220, 60), t) : mix565i(gfx::rgb565(250, 220, 60), gfx::rgb565(250, 110, 40), t - 32);
        if (p.boost_t > 0) col = mix565i(col, gfx::pal::WHITE, (int)(12 + 12 * sinf(anim_ * 30.f)));
        c.fill_rect(bar_x + x, bar_y + 2, 1, 10, col);
    }
    for (int i = 1; i < 4; ++i) c.fill_rect(bar_x + bar_w * i / 4, bar_y + 1, 1, 12, gfx::rgb565(12, 16, 28));
    gfx::draw_text_px(c, bar_x, H - 68, "PREDKOSC", TEXT_DIM, 12);
    snprintf(buf, sizeof(buf), "%d", (int)(fabsf(p.speed) * 1.2f));
    text_shadow(c, bar_x + bar_w + 16 + (48 - gfx::text_width_px(buf, 28)), H - 70, buf, gfx::pal::WHITE, 28);
    gfx::draw_text_px(c, bar_x + bar_w + 22, H - 34, "km/h", TEXT_DIM, 12);
    if (p.drifting && p.drift_t > 0.7f) {
        const uint16_t col = p.drift_t > 1.4f ? gfx::rgb565(120, 170, 255) : gfx::rgb565(255, 150, 40);
        fill_round_rect_alpha(c, bar_x, H - 74 - 26, 108, 22, 8, col, 220);
        gfx::draw_text_px(c, bar_x + 10, H - 74 - 24, p.drift_t > 1.4f ? "MINI-TURBO" : "DRIFT", gfx::rgb565(10, 12, 20), 14);
    }

    draw_minimap(c);

    // --- komunikaty na srodku ---
    if (state_ == State::Countdown) {
        const int n = (int)ceilf(countdown_ - 0.2f);
        if (n >= 1 && n <= 3) {
            snprintf(buf, sizeof(buf), "%d", n);
            const uint16_t col = n == 1 ? gfx::pal::GREEN : (n == 2 ? gfx::pal::YELLOW : gfx::pal::RED);
            const float ph = countdown_ - floorf(countdown_);
            const int r = 44 + (int)(ph * 10.f);
            c.fill_circle(W / 2, 176, r + 3, gfx::rgb565(6, 8, 14));
            c.fill_circle(W / 2, 176, r, col);
            gfx::draw_text_px(c, W / 2 - gfx::text_width_px(buf, 48) / 2, 176 - gfx::text_height_px(48) / 2 + 2, buf, gfx::rgb565(14, 16, 26), 48);
        }
    } else if (go_t_ > 0) {
        hud_panel(c, W / 2 - 140, 140, 280, 78, 170);
        center(152, "START!", gfx::pal::GREEN, 48);
    }
    if (lap_flash_ > 0 && state_ == State::Racing) {
        const int a = (int)engine::clampf(lap_flash_ * 200.f, 0.f, 170.f);
        hud_panel(c, W / 2 - 190, 130, 380, 60, a);
        if (p.lap >= LAPS) center(142, "OSTATNIE OKRAZENIE!", gfx::pal::YELLOW, 32);
        else { snprintf(buf, sizeof(buf), "OKRAZENIE %d", p.lap); center(142, buf, gfx::pal::WHITE, 32); }
    }
    if (autopilot_ && state_ == State::Racing) {
        fill_round_rect_alpha(c, W / 2 - 60, 108, 120, 22, 8, PANEL, 150);
        center(110, "AUTOPILOT", TEXT_DIM, 14);
    }
    if (p.spin_t > 0.8f) center(230, "Auu!", gfx::pal::RED, 40);
}

void KartGame::draw_hud_finish(gfx::Canvas& c)
{
    char buf[80];
    const Kart& p = karts_[0];
    auto center = [&](int y, const char* s, uint16_t col, int px) {
        text_shadow(c, (W - gfx::text_width_px(s, px)) / 2, y, s, col, px);
    };
    hud_panel(c, 150, 84, W - 300, 316, 215);
    c.fill_rect(174, 150, W - 348, 2, p.place == 1 ? gfx::pal::YELLOW : PANEL_HI);
    snprintf(buf, sizeof(buf), "META - %d. miejsce", p.place);
    center(98, buf, p.place == 1 ? gfx::pal::YELLOW : gfx::pal::WHITE, 40);
    const int fs = (int)p.finish_time % 60, fm = (int)p.finish_time / 60, fc = (int)(p.finish_time * 100) % 100;
    snprintf(buf, sizeof(buf), "czas %d:%02d.%02d      najlepsze okrazenie %.2f s", fm, fs, fc, (double)best_lap_);
    center(164, buf, TEXT_DIM, 16);
    static const uint16_t PLACE_COL[4] = { gfx::rgb565(255, 208, 40), gfx::rgb565(200, 206, 220), gfx::rgb565(214, 140, 70), gfx::rgb565(90, 96, 120) };
    for (int place = 1; place <= N_KARTS; ++place) {
        for (int i = 0; i < N_KARTS; ++i) {
            if (karts_[i].place != place) continue;
            static const char* const NAMES[] = { "Ty", "Niebieski", "Zielony", "Zolty" };
            const int y = 200 + (place - 1) * 36;
            fill_round_rect_alpha(c, 250, y - 2, 300, 30, 8, i == 0 ? gfx::rgb565(60, 60, 30) : gfx::rgb565(24, 30, 48), 160);
            c.fill_circle(272, y + 13, 11, PLACE_COL[place - 1]);
            snprintf(buf, sizeof(buf), "%d", place);
            gfx::draw_text_px(c, 272 - gfx::text_width_px(buf, 16) / 2, y + 13 - gfx::text_height_px(16) / 2, buf, gfx::rgb565(20, 22, 34), 16);
            c.fill_circle(306, y + 13, 8, KART_COLORS[karts_[i].color & 3]);
            gfx::draw_text_px(c, 326, y + 3, NAMES[i], i == 0 ? gfx::pal::YELLOW : gfx::pal::WHITE, 20);
            const int ts = (int)karts_[i].finish_time % 60, tm = (int)karts_[i].finish_time / 60, tc = (int)(karts_[i].finish_time * 100) % 100;
            if (karts_[i].finished) snprintf(buf, sizeof(buf), "%d:%02d.%02d", tm, ts, tc);
            else snprintf(buf, sizeof(buf), "w trasie");
            gfx::draw_text_px(c, 540 - gfx::text_width_px(buf, 16), y + 5, buf, TEXT_DIM, 16);
        }
    }
    if (fmodf(anim_, 1.f) < 0.65f) center(360, "A - jeszcze raz", TEXT_DIM, 16);
    draw_minimap(c);
}

void KartGame::draw_hud(gfx::Canvas& c)
{
    if (getenv("KART_CAM")) return;   // podglad modelu bez HUD
    // stan wizualny: komunikat o nowym okrazeniu
    const float dt = engine::clampf(anim_ - last_anim_, 0.f, 0.1f);
    last_anim_ = anim_;
    if (state_ == State::Title || state_ == State::Countdown) { lap_seen_ = 1; lap_flash_ = 0; }
    else if (karts_[0].lap != lap_seen_ && karts_[0].lap <= LAPS) { lap_seen_ = karts_[0].lap; lap_flash_ = 2.2f; }
    if (lap_flash_ > 0) lap_flash_ -= dt;

    if (state_ == State::Title) draw_hud_title(c);
    else if (state_ == State::Finished) draw_hud_finish(c);
    else draw_hud_race(c);
}

void KartGame::render(gfx::Canvas& c)
{
    const int64_t t0 = platform::micros();
    if (scene_ok_) draw_scene(c);
    else c.clear(fog_col_);
    draw_hud(c);
    const float ms = (float)(platform::micros() - t0) / 1000.f;
    render_ms_ = render_ms_ <= 0 ? ms : render_ms_ * 0.9f + ms * 0.1f;
    if (render_ms_ > 13.f && !engine::deterministic()) {   // w testach --frames jakosc stala (zrzuty powtarzalne)
        if (++slow_frames_ > 45 && quality_ < 2) {
            ++quality_;
            slow_frames_ = 0;
            CONSOLE_LOGW(TAG, "render %.1f ms (%d tri) - obnizam jakosc do %d", (double)render_ms_, r3d_.triangles_drawn(), quality_);
        }
    } else {
        slow_frames_ = 0;
    }
}

}  // namespace kart
