#include "gfx3d/mesh.h"

#include <string.h>

#include "core/log.h"
#include "platform/platform.h"

namespace gfx3d {

namespace {
const char* TAG = "gfx3d";

void* alloc_bytes(size_t n) { return platform::alloc_pixels((n + 1) / 2, /*fast=*/false); }

inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
}  // namespace

uint16_t shade565(uint16_t c, float k)
{
    const int r = clampi((int)(((c >> 11) & 31) * k + 0.5f), 0, 31);
    const int g = clampi((int)(((c >> 5) & 63) * k + 0.5f), 0, 63);
    const int b = clampi((int)((c & 31) * k + 0.5f), 0, 31);
    return (uint16_t)((r << 11) | (g << 5) | b);
}

uint16_t mix565(uint16_t a, uint16_t b, float t)
{
    if (t <= 0) return a;
    if (t >= 1) return b;
    const int k = (int)(t * 256.f);
    const int r = (((a >> 11) & 31) * (256 - k) + ((b >> 11) & 31) * k) >> 8;
    const int g = (((a >> 5) & 63) * (256 - k) + ((b >> 5) & 63) * k) >> 8;
    const int bl = ((a & 31) * (256 - k) + (b & 31) * k) >> 8;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

bool Mesh::init(int max_vertices, int max_triangles)
{
    v_ = reinterpret_cast<Vertex*>(alloc_bytes(sizeof(Vertex) * (size_t)max_vertices));
    t_ = reinterpret_cast<Tri*>(alloc_bytes(sizeof(Tri) * (size_t)max_triangles));
    if (!v_ || !t_) {
        CONSOLE_LOGE(TAG, "brak pamieci na mesh (%d wierzcholkow, %d trojkatow)", max_vertices, max_triangles);
        v_ = nullptr; t_ = nullptr;
        cap_v_ = cap_t_ = 0;
        return false;
    }
    cap_v_ = max_vertices;
    cap_t_ = max_triangles;
    nv_ = nt_ = 0;
    return true;
}

int Mesh::add_vertex(const Vec3& p) { return add_vertex(p, 0xFFFF); }

int Mesh::add_vertex(const Vec3& p, uint16_t color)
{
    if (nv_ >= cap_v_) return 0;
    v_[nv_].p = p;
    v_[nv_].n = Vec3(0, 1, 0);
    v_[nv_].c = color;
    return nv_++;
}

int Mesh::add_tri(int a, int b, int c, uint16_t color)
{
    if (nt_ >= cap_t_ || !t_) {
        if (t_ && nt_ == cap_t_) CONSOLE_LOGW(TAG, "mesh pelny (%d trojkatow) - reszta pominieta", cap_t_);
        return -1;
    }
    Tri& t = t_[nt_];
    t = Tri{};
    t.a = (uint16_t)a; t.b = (uint16_t)b; t.c = (uint16_t)c;
    t.color = color;
    return nt_++;
}

int Mesh::add_tri_out(int a, int b, int c, uint16_t color, const Vec3& outward)
{
    if (!v_) return -1;
    const Vec3 n = cross(v_[b].p - v_[a].p, v_[c].p - v_[a].p);
    if (dot(n, outward) < 0) return add_tri(a, c, b, color);
    return add_tri(a, b, c, color);
}

int Mesh::add_quad_out(int a, int b, int c, int d, uint16_t color, const Vec3& outward)
{
    const int first = add_tri_out(a, b, c, color, outward);
    add_tri_out(a, c, d, color, outward);
    return first;
}

void Mesh::set_tri_uv(int tri, float ua, float va, float ub, float vb, float uc, float vc)
{
    if (tri < 0 || tri >= nt_) return;
    Tri& t = t_[tri];
    t.tex = 1;
    t.u[0] = (uint16_t)(ua + 0.5f); t.v[0] = (uint16_t)(va + 0.5f);
    t.u[1] = (uint16_t)(ub + 0.5f); t.v[1] = (uint16_t)(vb + 0.5f);
    t.u[2] = (uint16_t)(uc + 0.5f); t.v[2] = (uint16_t)(vc + 0.5f);
}

int Mesh::add_tri_uv(int a, int b, int c, const Vec3& outward, const float uv[3][2], uint16_t color)
{
    if (!v_) return -1;
    const Vec3 n = cross(v_[b].p - v_[a].p, v_[c].p - v_[a].p);
    int idx;
    if (dot(n, outward) < 0) {
        idx = add_tri(a, c, b, color);
        set_tri_uv(idx, uv[0][0], uv[0][1], uv[2][0], uv[2][1], uv[1][0], uv[1][1]);
    } else {
        idx = add_tri(a, b, c, color);
        set_tri_uv(idx, uv[0][0], uv[0][1], uv[1][0], uv[1][1], uv[2][0], uv[2][1]);
    }
    return idx;
}

int Mesh::add_quad_uv(int a, int b, int c, int d, const Vec3& outward, const float uv[4][2], uint16_t color)
{
    const float t1[3][2] = { { uv[0][0], uv[0][1] }, { uv[1][0], uv[1][1] }, { uv[2][0], uv[2][1] } };
    const float t2[3][2] = { { uv[0][0], uv[0][1] }, { uv[2][0], uv[2][1] }, { uv[3][0], uv[3][1] } };
    const int first = add_tri_uv(a, b, c, outward, t1, color);
    add_tri_uv(a, c, d, outward, t2, color);
    return first;
}

void Mesh::add_box(const Vec3& c, const Vec3& s, uint16_t color) { add_box(c, s, color, color); }

void Mesh::add_box(const Vec3& c, const Vec3& s, uint16_t top, uint16_t side)
{
    const float hx = s.x / 2, hy = s.y / 2, hz = s.z / 2;
    Vec3 k[8];
    for (int i = 0; i < 8; ++i) k[i] = { c.x + ((i & 1) ? hx : -hx), c.y + ((i & 2) ? hy : -hy), c.z + ((i & 4) ? hz : -hz) };
    add_hexa(k, top, side, shade565(side, 0.7f));
}

int Mesh::add_hexa(const Vec3 k[8], uint16_t top, uint16_t side, uint16_t bottom, const float* top_uv)
{
    int v[8];
    for (int i = 0; i < 8; ++i) v[i] = add_vertex(k[i]);
    // srodek bryly - wektory "na zewnatrz" liczone od niego, wiec dziala tez dla bryl scietych
    Vec3 mid(0, 0, 0);
    for (int i = 0; i < 8; ++i) mid += k[i];
    mid = mid * (1.f / 8.f);
    auto face = [&](int a, int b, int c, int d, uint16_t col) {
        const Vec3 fc = (k[a] + k[b] + k[c] + k[d]) * 0.25f;
        return add_quad_out(v[a], v[b], v[c], v[d], col, fc - mid);
    };
    // sciany: (bity: 1 = +x, 2 = +y, 4 = +z)
    int first;
    if (top_uv) {
        // gora: 2 (-x,-z) 3 (+x,-z) 7 (+x,+z) 6 (-x,+z); v0 przy +z
        const float uv[4][2] = { { top_uv[0], top_uv[3] }, { top_uv[2], top_uv[3] }, { top_uv[2], top_uv[1] }, { top_uv[0], top_uv[1] } };
        const Vec3 fc = (k[2] + k[3] + k[7] + k[6]) * 0.25f;
        first = add_quad_uv(v[2], v[3], v[7], v[6], fc - mid, uv, top);
    } else {
        first = face(2, 3, 7, 6, top);   // gora
    }
    face(0, 1, 5, 4, bottom);   // dol
    face(1, 3, 7, 5, side);     // +x
    face(0, 2, 6, 4, side);     // -x
    face(4, 5, 7, 6, side);     // +z
    face(0, 1, 3, 2, side);     // -z
    return first;
}

int Mesh::add_loft(const Vec3& c0, float hw0, float hh0, const Vec3& c1, float hw1, float hh1, uint16_t color, const float* top_uv)
{
    Vec3 k[8];
    for (int i = 0; i < 8; ++i) {
        const Vec3& c = (i & 4) ? c1 : c0;
        const float hw = (i & 4) ? hw1 : hw0, hh = (i & 4) ? hh1 : hh0;
        k[i] = { c.x + ((i & 1) ? hw : -hw), c.y + ((i & 2) ? hh : -hh), c.z };
    }
    return add_hexa(k, shade565(color, 1.08f), color, shade565(color, 0.7f), top_uv);
}

void Mesh::add_wedge(float x0, float x1, float z0, float z1, float y0, float y_back, float y_front, uint16_t color)
{
    const int b0 = add_vertex({ x0, y0, z0 }), b1 = add_vertex({ x1, y0, z0 });
    const int b2 = add_vertex({ x1, y0, z1 }), b3 = add_vertex({ x0, y0, z1 });
    const int t0 = add_vertex({ x0, y_back, z0 }), t1 = add_vertex({ x1, y_back, z0 });
    const int t2 = add_vertex({ x1, y_front, z1 }), t3 = add_vertex({ x0, y_front, z1 });
    add_quad_out(t0, t1, t2, t3, color, { 0, 1, 0 });
    add_quad_out(b0, b1, t1, t0, color, { 0, 0, -1 });
    add_quad_out(b2, b3, t3, t2, color, { 0, 0, 1 });
    add_quad_out(b1, b2, t2, t1, color, { 1, 0, 0 });
    add_quad_out(b3, b0, t0, t3, color, { -1, 0, 0 });
    add_quad_out(b0, b1, b2, b3, shade565(color, 0.6f), { 0, -1, 0 });
}

void Mesh::add_wheel(const Vec3& c, float radius, float width, int segments, uint16_t tyre, uint16_t hub, float rim, const float* tread_uv)
{
    const float hw = width / 2;
    int ring_l[24], ring_r[24], rim_l[24], rim_r[24], cap_l[24], cap_r[24];
    if (segments > 24) segments = 24;
    if (rim > 0.98f) rim = 0.f;   // brak osobnej felgi: caly bok w kolorze hub
    const float rr = radius * rim, rc = radius * 0.2f;      // promien felgi, promien kapsla
    const float dish = rim > 0.f ? width * 0.22f : 0.f;      // wklesniecie felgi do srodka kola
    const int cl = add_vertex({ c.x - hw + dish, c.y, c.z }), cr = add_vertex({ c.x + hw - dish, c.y, c.z });
    for (int i = 0; i < segments; ++i) {
        const float a = i * 6.2831853f / segments, ca = cosf(a), sa = sinf(a);
        ring_l[i] = add_vertex({ c.x - hw, c.y + ca * radius, c.z + sa * radius });
        ring_r[i] = add_vertex({ c.x + hw, c.y + ca * radius, c.z + sa * radius });
        if (rim > 0.f) {
            rim_l[i] = add_vertex({ c.x - hw, c.y + ca * rr, c.z + sa * rr });
            rim_r[i] = add_vertex({ c.x + hw, c.y + ca * rr, c.z + sa * rr });
            cap_l[i] = add_vertex({ c.x - hw + dish, c.y + ca * rc, c.z + sa * rc });
            cap_r[i] = add_vertex({ c.x + hw - dish, c.y + ca * rc, c.z + sa * rc });
        }
    }
    const uint16_t sidewall = shade565(tyre, 1.7f), cap = shade565(hub, 1.6f);
    for (int i = 0; i < segments; ++i) {
        const int j = (i + 1) % segments;
        const float am = (i + 0.5f) * 6.2831853f / segments;
        const Vec3 out = Vec3(0, cosf(am), sinf(am));
        if (tread_uv) {
            const float uv[4][2] = { { tread_uv[0], tread_uv[1] }, { tread_uv[2], tread_uv[1] }, { tread_uv[2], tread_uv[3] }, { tread_uv[0], tread_uv[3] } };
            add_quad_uv(ring_l[i], ring_r[i], ring_r[j], ring_l[j], out, uv, tyre);
        } else {
            add_quad_out(ring_l[i], ring_r[i], ring_r[j], ring_l[j], (i & 1) ? tyre : shade565(tyre, 0.9f), out);   // bieznik
        }
        if (rim > 0.f) {
            const uint16_t spoke = (i & 1) ? hub : shade565(hub, 0.72f);
            add_quad_out(ring_l[i], ring_l[j], rim_l[j], rim_l[i], sidewall, { -1, 0, 0 });                      // sciana boczna
            add_quad_out(ring_r[i], ring_r[j], rim_r[j], rim_r[i], sidewall, { 1, 0, 0 });
            add_quad_out(rim_l[i], rim_l[j], cap_l[j], cap_l[i], spoke, { -1, 0, 0 });                           // felga (wklesla)
            add_quad_out(rim_r[i], rim_r[j], cap_r[j], cap_r[i], spoke, { 1, 0, 0 });
            add_tri_out(cl, cap_l[i], cap_l[j], cap, { -1, 0, 0 });                                              // kapsel
            add_tri_out(cr, cap_r[i], cap_r[j], cap, { 1, 0, 0 });
        } else {
            add_tri_out(cl, ring_l[i], ring_l[j], hub, { -1, 0, 0 });
            add_tri_out(cr, ring_r[i], ring_r[j], hub, { 1, 0, 0 });
        }
    }
}

void Mesh::add_cylinder_y(const Vec3& base, float radius, float height, int segments, uint16_t color)
{
    if (segments > 24) segments = 24;
    int lo[24], hi[24];
    for (int i = 0; i < segments; ++i) {
        const float a = i * 6.2831853f / segments;
        lo[i] = add_vertex({ base.x + cosf(a) * radius, base.y, base.z + sinf(a) * radius });
        hi[i] = add_vertex({ base.x + cosf(a) * radius, base.y + height, base.z + sinf(a) * radius });
    }
    const int top = add_vertex({ base.x, base.y + height, base.z });
    for (int i = 0; i < segments; ++i) {
        const int j = (i + 1) % segments;
        const float am = (i + 0.5f) * 6.2831853f / segments;
        add_quad_out(lo[i], lo[j], hi[j], hi[i], (i & 1) ? color : shade565(color, 0.88f), { cosf(am), 0, sinf(am) });
        add_tri_out(top, hi[i], hi[j], shade565(color, 1.1f), { 0, 1, 0 });
    }
}

void Mesh::add_cone_y(const Vec3& base, float radius, float height, int segments, uint16_t color)
{
    if (segments > 24) segments = 24;
    int lo[24];
    for (int i = 0; i < segments; ++i) {
        const float a = i * 6.2831853f / segments;
        lo[i] = add_vertex({ base.x + cosf(a) * radius, base.y, base.z + sinf(a) * radius });
    }
    const int top = add_vertex({ base.x, base.y + height, base.z });
    const int bot = add_vertex({ base.x, base.y, base.z });
    for (int i = 0; i < segments; ++i) {
        const int j = (i + 1) % segments;
        const float am = (i + 0.5f) * 6.2831853f / segments;
        add_tri_out(lo[i], lo[j], top, (i & 1) ? color : shade565(color, 0.9f), { cosf(am), 0.4f, sinf(am) });
        add_tri_out(bot, lo[i], lo[j], shade565(color, 0.6f), { 0, -1, 0 });
    }
}

void Mesh::add_sphere(const Vec3& c, float radius, int segments, int rings, uint16_t color, uint16_t color2)
{
    if (segments > 16) segments = 16;
    if (rings > 8) rings = 8;
    if (color2 == 0xFFFF) color2 = color;
    int idx[9][16];
    const int top = add_vertex({ c.x, c.y + radius, c.z });
    const int bot = add_vertex({ c.x, c.y - radius, c.z });
    for (int r = 1; r < rings; ++r) {
        const float phi = 3.1415926f * r / rings;   // od gory
        const float y = cosf(phi) * radius, rr = sinf(phi) * radius;
        for (int s = 0; s < segments; ++s) {
            const float a = s * 6.2831853f / segments;
            idx[r][s] = add_vertex({ c.x + cosf(a) * rr, c.y + y, c.z + sinf(a) * rr });
        }
    }
    for (int s = 0; s < segments; ++s) {
        const int s1 = (s + 1) % segments;
        add_tri_out(top, idx[1][s], idx[1][s1], color, Vec3(0, 1, 0));
        for (int r = 1; r < rings - 1; ++r) {
            const uint16_t col = (r < rings / 2) ? color : color2;
            const Vec3 out = v_[idx[r][s]].p - c;
            add_quad_out(idx[r][s], idx[r + 1][s], idx[r + 1][s1], idx[r][s1], col, out);
        }
        add_tri_out(bot, idx[rings - 1][s1], idx[rings - 1][s], color2, Vec3(0, -1, 0));
    }
}

void Mesh::add_disc_y(const Vec3& c, float rx, float rz, int segments, uint16_t color)
{
    if (segments > 24) segments = 24;
    const int mid = add_vertex(c);
    int ring[24];
    for (int i = 0; i < segments; ++i) {
        const float a = i * 6.2831853f / segments;
        ring[i] = add_vertex({ c.x + cosf(a) * rx, c.y, c.z + sinf(a) * rz });
    }
    for (int i = 0; i < segments; ++i) add_tri_out(mid, ring[i], ring[(i + 1) % segments], color, { 0, 1, 0 });
}

void Mesh::add_billboard(const Vec3& base, float w, float h, float nx, float nz, const float* uv, uint16_t color)
{
    // prawo widza patrzacego NA sciane (wzdluz -n): cross(-n, up) = (nz, 0, -nx) - tak tekst czyta sie od lewej
    const float rx = nz, rz = -nx, hw = w * 0.5f;
    const int a = add_vertex({ base.x - rx * hw, base.y, base.z - rz * hw });
    const int b = add_vertex({ base.x + rx * hw, base.y, base.z + rz * hw });
    const int c = add_vertex({ base.x + rx * hw, base.y + h, base.z + rz * hw });
    const int d = add_vertex({ base.x - rx * hw, base.y + h, base.z - rz * hw });
    const float q[4][2] = { { uv[0], uv[3] }, { uv[2], uv[3] }, { uv[2], uv[1] }, { uv[0], uv[1] } };
    add_quad_uv(a, b, c, d, { nx, 0, nz }, q, color);
}

void Mesh::compute_smooth_normals()
{
    if (!v_ || !t_) return;
    for (int i = 0; i < nv_; ++i) v_[i].n = Vec3(0, 0, 0);
    for (int i = 0; i < nt_; ++i) {
        const Tri& t = t_[i];
        const Vec3 n = cross(v_[t.b].p - v_[t.a].p, v_[t.c].p - v_[t.a].p);
        v_[t.a].n += n; v_[t.b].n += n; v_[t.c].n += n;
    }
    for (int i = 0; i < nv_; ++i) v_[i].n = v_[i].n.normalized();
}

}  // namespace gfx3d
