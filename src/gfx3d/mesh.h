// Siatka trojkatow (gfx3d): wierzcholki z normalnymi i opcjonalnym kolorem, trojkaty z kolorem RGB565 albo
// z teksturą (UV w tekselach atlasu). Budowana proceduralnie przy starcie gry (bufory w PSRAM), rysowana przez
// gfx3d::Renderer. Styl low-poly: kolor albo tekstura na trojkat, cieniowanie plaskie albo gladkie (Gouraud po
// normalnych wierzcholkow, gdy mesh.smooth), opcjonalny odblask (specular) na siatkach gladkich.
#pragma once

#include <stdint.h>

#include "gfx3d/math3d.h"

namespace gfx3d {

struct Vertex {
    Vec3     p;
    Vec3     n;   // normalna (dla smooth); liczona przez compute_smooth_normals()
    uint16_t c;   // kolor wierzcholka (uzywany, gdy mesh.vertex_colors) - gradienty terenu/asfaltu
};

struct Tri {
    uint16_t a, b, c;
    uint16_t color;      // kolor plaski (gdy tex == 0)
    uint16_t u[3], v[3]; // teksele atlasu (gdy tex): kolejnosc jak a, b, c
    uint8_t  tex;        // 1 = teksturowany
    uint8_t  pad;
};

class Mesh {
public:
    // Rezerwuje miejsce (PSRAM). false = brak pamieci (mesh pozostaje pusty, rysowanie nic nie robi).
    bool init(int max_vertices, int max_triangles);
    void clear() { nv_ = nt_ = 0; }

    int  vertex_count() const { return nv_; }
    int  triangle_count() const { return nt_; }
    const Vertex* vertices() const { return v_; }
    const Tri*    triangles() const { return t_; }
    bool  smooth        = false;   // Gouraud po normalnych wierzcholkow
    bool  unlit         = false;   // bez oswietlenia (tylko mgla) - np. swiecace skrzynki, HUD w scenie
    bool  vertex_colors = false;   // kolor bazowy z wierzcholkow (Vertex::c) zamiast z trojkata - tylko ze smooth
    bool  alpha_test    = false;   // teksel o indeksie 0 = przezroczysty (drzewa-billboardy)
    float specular      = 0.f;     // sila odblasku (0 = brak); tylko smooth - lakier, kask

    int  add_vertex(const Vec3& p);
    int  add_vertex(const Vec3& p, uint16_t color);
    // Trojkat o zadanym nawinieciu (a, b, c). Normalna = cross(b - a, c - a). Zwraca indeks trojkata (-1 = brak miejsca).
    int  add_tri(int a, int b, int c, uint16_t color);
    // Trojkat z normalna zwrocona w strone `outward` (nawiniecie poprawiane automatycznie).
    int  add_tri_out(int a, int b, int c, uint16_t color, const Vec3& outward);
    // Czworokat a-b-c-d (dwa trojkaty), normalna w strone `outward`. Zwraca indeks pierwszego trojkata.
    int  add_quad_out(int a, int b, int c, int d, uint16_t color, const Vec3& outward);
    // Czworokat teksturowany: uv[4][2] = teksele dla a, b, c, d (kolejnosc wierzcholkow zachowana przy poprawianiu
    // nawiniecia).
    // `color` = kolor zastepczy, gdy renderer nie ma atlasu.
    int  add_quad_uv(int a, int b, int c, int d, const Vec3& outward, const float uv[4][2], uint16_t color = 0x8410);
    // Trojkat teksturowany.
    int  add_tri_uv(int a, int b, int c, const Vec3& outward, const float uv[3][2], uint16_t color = 0x8410);
    // Nadaje teksture istniejacemu trojkatowi (u/v w tekselach dla jego a, b, c).
    void set_tri_uv(int tri, float ua, float va, float ub, float vb, float uc, float vc);

    // --- bryly (srodek, rozmiar) ---
    void add_box(const Vec3& center, const Vec3& size, uint16_t color);
    void add_box(const Vec3& center, const Vec3& size, uint16_t top, uint16_t side);
    // Szescioscian o dowolnych 8 narozach (bity indeksu: 1 = +x, 2 = +y, 4 = +z, jak w add_box). top_uv (opcjonalnie)
    // = { u0, v0, u1, v1 } prostokat atlasu na gorna sciane: v0 przy +z (przod), u0 przy -x (lewo).
    // Zwraca indeks pierwszego trojkata (gorna sciana).
    int  add_hexa(const Vec3 c[8], uint16_t top, uint16_t side, uint16_t bottom, const float* top_uv = nullptr);
    // Bryla scieta ("loft"): prostokat o srodku c0 (polszerokosc hw0, polwysokosc hh0, w plaszczyznie XY przy z = c0.z)
    // polaczony z prostokatem c1/hw1/hh1 - nos gokarta, pontony, ramiona. Kolor jeden (gora lekko jasniejsza).
    int  add_loft(const Vec3& c0, float hw0, float hh0, const Vec3& c1, float hw1, float hh1, uint16_t color,
                  const float* top_uv = nullptr);
    // Klin: pudlo, ktorego gorna sciana opada od y_back (przy z0) do y_front (przy z1) - maska gokarta.
    void add_wedge(float x0, float x1, float z0, float z1, float y0, float y_back, float y_front, uint16_t color);
    // Kolo o osi X: srodek, promien, szerokosc, liczba bokow. rim = promien felgi / promien kola (1 = caly bok w kolorze
    // hub, bez felgi). Z felga: bok = sciana boczna opony (jasniejsza czern) + wklesla felga (kolor hub, fasety na przemian
    // ciemniejsze = "szprychy") + maly jasny kapsel na osi. tread_uv (opcjonalnie) = kafelek atlasu na bieznik
    // (kazdy segment dostaje caly kafelek: u wzdluz szerokosci, v wzdluz obwodu).
    void add_wheel(const Vec3& center, float radius, float width, int segments, uint16_t tyre, uint16_t hub, float rim = 1.f,
                   const float* tread_uv = nullptr);
    // Walec/graniastoslup o osi Y (pien, slupek).
    void add_cylinder_y(const Vec3& base, float radius, float height, int segments, uint16_t color);
    // Stozek o osi Y (choinka), podstawa w `base`.
    void add_cone_y(const Vec3& base, float radius, float height, int segments, uint16_t color);
    // Kula low-poly (kask, korona drzewa, skorupa): segments x rings. color2 = dolna polowa (0xFFFF = ten sam).
    void add_sphere(const Vec3& center, float radius, int segments, int rings, uint16_t color, uint16_t color2 = 0xFFFF);
    // Dysk w plaszczyznie XZ (cien), normalna w gore.
    void add_disc_y(const Vec3& center, float rx, float rz, int segments, uint16_t color);
    // Pionowy prostokat teksturowany o srodku podstawy `base`, szerokosci w i wysokosci h, w plaszczyznie o normalnej
    // (nx, 0, nz); dwustronny (rysowany bez odrzucania tylnych scian). uv = { u0, v0, u1, v1 } (v0 = gora).
    void add_billboard(const Vec3& base, float w, float h, float nx, float nz, const float* uv, uint16_t color = 0x8410);

    // Normalne wierzcholkow = srednia normalnych trojkatow (do smooth). Wolac po zbudowaniu.
    void compute_smooth_normals();

private:
    Vertex* v_ = nullptr;
    Tri*    t_ = nullptr;
    int     nv_ = 0, nt_ = 0, cap_v_ = 0, cap_t_ = 0;
};

// Przyciemnienie/rozjasnienie koloru RGB565 (k = 1.0 bez zmian).
uint16_t shade565(uint16_t c, float k);
// Mieszanie dwoch kolorow RGB565 (t = 0 -> a, 1 -> b).
uint16_t mix565(uint16_t a, uint16_t b, float t);

}  // namespace gfx3d
