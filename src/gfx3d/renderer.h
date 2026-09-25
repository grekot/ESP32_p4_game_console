// Software'owy renderer 3D (gfx3d) na plotno RGB565: trojkaty low-poly z cieniowaniem plaskim, Gouraud albo
// teksturowane (atlas 8-bitowy + colormapa: odcien x mgla -> RGB565, korekcja perspektywy co 16 px), swiatlo
// kierunkowe + ambient + odblask, mgla, odrzucanie tylnych scian, przycinanie do plaszczyzny bliskiej,
// sortowanie malarskie (kubelki po glebokosci, osobno dla warstw: 0 = teren, 1 = obiekty), opcjonalny Z-bufor
// 16-bit (poprawne przecinanie bryl; kosztuje pamiec i czas - do wlaczenia po pomiarze na plytce), cienie rzutowane
// (maska + przyciemnienie pikseli podloza po warstwie 0 - polprzezroczyste, bez podwojnego przyciemniania).
//
// Uzycie na klatke: begin(canvas, camera, light, fog) -> draw_mesh(...)/draw_tri(...)/draw_shadow(...) -> end().
// Bufory w PSRAM (init). Rysowanie jest deterministyczne (bez losowosci).
#pragma once

#include <stdint.h>

#include "gfx/canvas.h"
#include "gfx3d/mesh.h"

namespace gfx3d {

struct Camera {
    Vec3  pos;
    Vec3  target;
    float fov_y = 1.05f;   // rad (~60 stopni)
};

struct Light {
    Vec3  dir     = Vec3(-0.4f, 0.8f, -0.45f).normalized();   // KIERUNEK DO swiatla
    float ambient = 0.45f;
    float diffuse = 0.62f;
};

struct Fog {
    uint16_t color = 0xC6FB;
    float    start = 400.f;   // od tej odleglosci kolor zmierza do fog.color
    float    end   = 1300.f;  // tu jest juz caly fog.color; trojkaty dalej sa pomijane
};

class Renderer {
public:
    static constexpr int SHADES = 16;   // poziomy odcienia w colormapie (0.35 .. 1.25)
    static constexpr int FOGS   = 12;   // poziomy mgly w colormapie

    // max_tris = pojemnosc bufora trojkatow ekranowych na klatke (po przycieciu). ~100 B kazdy.
    bool init(int max_tris);

    // Atlas tekstur: indeksy 8-bit (w = 1 << w_shift), paleta 256 kolorow RGB565 (indeks 0 = przezroczysty przy
    // alpha_test). Buduje colormape SHADES x FOGS x 256 dla podanego koloru mgly (96 kB, PSRAM).
    bool set_texture(const uint8_t* atlas, int w_shift, int h, const uint16_t* palette565, uint16_t fog_color);
    // Z-bufor 16-bit (w x h x 2 B w PSRAM, alokowany przy pierwszym wlaczeniu). false = brak pamieci.
    bool enable_zbuffer(bool on);
    bool zbuffer_enabled() const { return zbuf_on_ && zbuf_; }

    void begin(gfx::Canvas& canvas, const Camera& cam, const Light& light, const Fog& fog);
    // Rysuje siatke przeksztalcona macierza modelu. layer 0 = teren (najpierw), 1 = obiekty.
    // depth_bias > 0 przesuwa trojkaty do blizszych kubelkow sortowania (rysuja sie PO tym, co lezy pod nimi) -
    // dla cieni/sladow na podlozu (warstwa 0), zeby duzy trojkat terenu o dalszym srodku ich nie zamalowal.
    void draw_mesh(const Mesh& mesh, const Mat4& model, int layer, bool cull_backfaces = true, float depth_bias = 0.f);
    // Pojedynczy trojkat w swiecie (np. dynamiczne ksztalty). lit = z oswietleniem.
    void draw_tri(const Vec3& a, const Vec3& b, const Vec3& c, uint16_t color, int layer, bool lit = true, float depth_bias = 0.f);
    // Cien rzutowany: siatka przeksztalcona macierza `model` (zawierajaca juz rzut na plaszczyzne podloza,
    // Mat4::shadow_onto_plane) trafia do maski; w end() po warstwie 0 piksele maski sa przyciemniane.
    void draw_shadow(const Mesh& mesh, const Mat4& model);
    // Sortuje i rasteryzuje wszystko zebrane od begin().
    void end();

    // Tryb oszczedny: siatki smooth rysowane plasko (kolor z normalnej sciany) - tansza rasteryzacja na plytce.
    void set_flat_only(bool on) { flat_only_ = on; }
    // Rzut punktu swiata na ekran (do sprite'ow 2D w scenie). false = za kamera.
    bool project(const Vec3& p, float& sx, float& sy, float& depth) const;
    // Wiersz ekranu, na ktorym lezy horyzont (do pasow nieba/gor).
    float horizon_y() const;
    // Statystyki ostatniej klatki
    int triangles_submitted() const { return stat_submitted_; }
    int triangles_drawn() const { return stat_drawn_; }
    int pixels_filled() const { return stat_pixels_; }

private:
    struct STri {
        float    x[3], y[3];
        float    iz[3];      // 1/z (Z-bufor, korekcja perspektywy)
        float    uz[3], vz[3];   // u/z, v/z w tekselach (tex)
        float    row[3];     // wiersz colormapy (tex): fog * SHADES + odcien
        float    z;          // glebokosc srodka (do sortowania)
        uint16_t c[3];       // kolory wierzcholkow (plaski = trzy takie same)
        uint8_t  layer;
        uint8_t  smooth;
        uint8_t  tex;
        uint8_t  alpha;      // alpha_test (tex)
        int32_t  next;       // lista w kubelku
    };
    static constexpr int BUCKETS = 1024;

    // wierzcholek w przestrzeni kamery z atrybutami (do przycinania)
    struct CV { Vec3 p; uint16_t c; float u, v, row; };

    void  submit(const CV* poly, int n, int layer, bool smooth, float depth, bool tex, bool alpha);
    void  submit_view_tri(const CV& a, const CV& b, const CV& c, int layer, bool smooth, bool tex, bool alpha);
    void  raster(const STri& t);
    void  raster_shadow(const CV* poly, int n);
    void  apply_shadows();
    float light_k(const Vec3& n_world, const Vec3& half, float specular) const;
    uint16_t light_color(uint16_t base, float k, float depth) const;
    float tex_row(float k, float depth) const;

    gfx::Canvas* canvas_ = nullptr;
    uint16_t*    px_     = nullptr;
    int          w_ = 0, h_ = 0;
    Vec3  cam_pos_, f_, r_, u_;
    float focal_ = 1.f;
    Light light_;
    Fog   fog_;
    bool  flat_only_ = false;
    float depth_bias_ = 0.f;

    static constexpr int MAX_MESH_VERTS = 4096;   // cache wierzcholkow na jedno draw_mesh
    Vec3*    wp_   = nullptr;      // wierzcholki w swiecie
    Vec3*    vp_   = nullptr;      // wierzcholki w kamerze
    uint8_t* done_ = nullptr;
    STri*    tris_ = nullptr;
    int      n_ = 0, cap_ = 0;
    int32_t* buckets_ = nullptr;   // [2][BUCKETS] glowy list
    int32_t* tails_   = nullptr;   // [2][BUCKETS] ogony (kolejnosc zglaszania zachowana)

    // tekstury
    const uint8_t* atlas_ = nullptr;
    int            atlas_shift_ = 0, atlas_w_ = 0, atlas_h_ = 0;
    uint16_t*      cmap_ = nullptr;   // [FOGS * SHADES][256]

    // Z-bufor i maska cieni
    uint16_t* zbuf_ = nullptr;
    bool      zbuf_on_ = false;
    uint8_t*  mask_ = nullptr;
    int       mask_x0_ = 0, mask_y0_ = 0, mask_x1_ = -1, mask_y1_ = -1;

    int stat_submitted_ = 0, stat_drawn_ = 0, stat_pixels_ = 0;
};

}  // namespace gfx3d
