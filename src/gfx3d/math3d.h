// Matematyka 3D dla software'owego renderera (gfx3d): wektor, macierz 4x4 (wiersze), obroty, rzut.
// Uklad: X w prawo, Y w gore, Z "w glab" (kierunek jazdy przy kacie 0 to +X; mapa 2D gry lezy w plaszczyznie XZ).
// Sam float - P4 ma FPU pojedynczej precyzji, na PC bez roznicy.
#pragma once

#include <math.h>

namespace gfx3d {

struct Vec3 {
    float x = 0, y = 0, z = 0;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator*(float k) const { return { x * k, y * k, z * k }; }
    Vec3 operator-() const { return { -x, -y, -z }; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }

    float length() const { return sqrtf(x * x + y * y + z * z); }
    Vec3  normalized() const
    {
        const float l = length();
        return l > 1e-9f ? Vec3(x / l, y / l, z / l) : Vec3(0, 1, 0);
    }
};

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3  cross(const Vec3& a, const Vec3& b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }

// Macierz 4x4, wiersze (m[r*4+c]). (A * B).apply(p) == A.apply(B.apply(p)).
struct Mat4 {
    float m[16];

    static Mat4 identity()
    {
        Mat4 r;
        for (int i = 0; i < 16; ++i) r.m[i] = (i % 5 == 0) ? 1.f : 0.f;
        return r;
    }
    static Mat4 translation(const Vec3& t)
    {
        Mat4 r = identity();
        r.m[3] = t.x; r.m[7] = t.y; r.m[11] = t.z;
        return r;
    }
    static Mat4 scale(float sx, float sy, float sz)
    {
        Mat4 r = identity();
        r.m[0] = sx; r.m[5] = sy; r.m[10] = sz;
        return r;
    }
    static Mat4 rotation_x(float a)   // wokol X (pochylenie)
    {
        Mat4 r = identity();
        const float c = cosf(a), s = sinf(a);
        r.m[5] = c; r.m[6] = -s; r.m[9] = s; r.m[10] = c;
        return r;
    }
    static Mat4 rotation_y(float a)   // wokol Y (kierunek jazdy); dodatni kat obraca +X w strone +Z? -> patrz heading()
    {
        Mat4 r = identity();
        const float c = cosf(a), s = sinf(a);
        r.m[0] = c; r.m[2] = s; r.m[8] = -s; r.m[10] = c;
        return r;
    }
    static Mat4 rotation_z(float a)   // wokol Z (przechyl)
    {
        Mat4 r = identity();
        const float c = cosf(a), s = sinf(a);
        r.m[0] = c; r.m[1] = -s; r.m[4] = s; r.m[5] = c;
        return r;
    }
    // Obrot ustawiajacy lokalna os +Z modelu ("przod") na kierunek (cos a, 0, sin a) w plaszczyznie XZ -
    // zgodnie z katem gokarta w fizyce 2D (dir = (cos angle, sin angle) na mapie x/y = 3D X/Z).
    static Mat4 heading(float angle)
    {
        // kolumny: prawo r = (-sin a, 0, cos a), gora (0,1,0), przod f = (cos a, 0, sin a); cross(r, f) = (0,1,0)
        Mat4 r = identity();
        const float c = cosf(angle), s = sinf(angle);
        r.m[0] = -s; r.m[2] = c;
        r.m[8] = c;  r.m[10] = s;
        return r;
    }

    // Rzut na plaszczyzne y = ground wzdluz kierunku swiatla `light` (KIERUNEK DO swiatla, light.y > 0):
    // p' = p - light * (p.y - ground) / light.y. Cien rzutowany: draw_shadow(mesh, shadow * model).
    static Mat4 shadow_onto_plane(const Vec3& light, float ground)
    {
        Mat4 r = identity();
        const float ly = light.y > 0.05f ? light.y : 0.05f;
        const float kx = light.x / ly, kz = light.z / ly;
        r.m[0] = 1;  r.m[1] = -kx; r.m[2] = 0;  r.m[3] = kx * ground;
        r.m[4] = 0;  r.m[5] = 0;   r.m[6] = 0;  r.m[7] = ground;
        r.m[8] = 0;  r.m[9] = -kz; r.m[10] = 1; r.m[11] = kz * ground;
        return r;
    }

    Mat4 operator*(const Mat4& b) const
    {
        Mat4 r;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                r.m[i * 4 + j] = m[i * 4 + 0] * b.m[0 * 4 + j] + m[i * 4 + 1] * b.m[1 * 4 + j] +
                                 m[i * 4 + 2] * b.m[2 * 4 + j] + m[i * 4 + 3] * b.m[3 * 4 + j];
        return r;
    }
    Vec3 apply(const Vec3& p) const
    {
        return { m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3], m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7],
                 m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11] };
    }
    Vec3 apply_dir(const Vec3& d) const   // bez przesuniecia (normalne, kierunki)
    {
        return { m[0] * d.x + m[1] * d.y + m[2] * d.z, m[4] * d.x + m[5] * d.y + m[6] * d.z,
                 m[8] * d.x + m[9] * d.y + m[10] * d.z };
    }
};

}  // namespace gfx3d
