#include "gfx3d/renderer.h"

#include <math.h>
#include <string.h>

#include "core/log.h"
#include "platform/platform.h"

namespace gfx3d {

namespace {
const char* TAG = "gfx3d";

constexpr float NEAR_Z = 1.0f;
constexpr int   TEX_BLOCK = 16;   // korekcja perspektywy co tyle pikseli

inline int   imin(int a, int b) { return a < b ? a : b; }
inline int   imax(int a, int b) { return a > b ? a : b; }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

inline uint16_t pack565(int r, int g, int b) { return (uint16_t)((r << 11) | (g << 5) | b); }
// przyciemnienie ok. 0.69 (cien): polowa + osemka + szesnastka kazdej skladowej
inline uint16_t darken565(uint16_t c) { return (uint16_t)(((c >> 1) & 0x7BEF) + ((c >> 3) & 0x1861) + ((c >> 4) & 0x0841)); }

void* alloc_bytes(size_t n, bool fast) { return platform::alloc_pixels((n + 1) / 2, fast); }

// atrybuty wierzcholka na krawedzi/skanlinii
struct Attr { float x, iz, uz, vz, row, r, g, b; };
inline Attr lerp_attr(const Attr& a, const Attr& b, float t)
{
    return { lerpf(a.x, b.x, t), lerpf(a.iz, b.iz, t), lerpf(a.uz, b.uz, t), lerpf(a.vz, b.vz, t), lerpf(a.row, b.row, t),
             lerpf(a.r, b.r, t), lerpf(a.g, b.g, t), lerpf(a.b, b.b, t) };
}
}  // namespace

bool Renderer::init(int max_tris)
{
    tris_    = reinterpret_cast<STri*>(alloc_bytes(sizeof(STri) * (size_t)max_tris, false));
    buckets_ = reinterpret_cast<int32_t*>(alloc_bytes(sizeof(int32_t) * 2 * BUCKETS, false));
    tails_   = reinterpret_cast<int32_t*>(alloc_bytes(sizeof(int32_t) * 2 * BUCKETS, false));
    // cache wierzcholkow: najlepiej w SRAM (czytany przy kazdym trojkacie), awaryjnie PSRAM
    wp_   = reinterpret_cast<Vec3*>(alloc_bytes(sizeof(Vec3) * MAX_MESH_VERTS, true));
    vp_   = reinterpret_cast<Vec3*>(alloc_bytes(sizeof(Vec3) * MAX_MESH_VERTS, true));
    done_ = reinterpret_cast<uint8_t*>(alloc_bytes(MAX_MESH_VERTS, true));
    if (!tris_ || !buckets_ || !tails_ || !wp_ || !vp_ || !done_) {
        CONSOLE_LOGE(TAG, "brak pamieci na bufor %d trojkatow", max_tris);
        tris_ = nullptr;
        cap_  = 0;
        return false;
    }
    cap_ = max_tris;
    return true;
}

bool Renderer::set_texture(const uint8_t* atlas, int w_shift, int h, const uint16_t* palette565, uint16_t fog_color)
{
    if (!cmap_) cmap_ = reinterpret_cast<uint16_t*>(alloc_bytes(sizeof(uint16_t) * 256 * SHADES * FOGS, false));
    if (!cmap_) {
        CONSOLE_LOGE(TAG, "brak pamieci na colormape");
        return false;
    }
    atlas_ = atlas;
    atlas_shift_ = w_shift;
    atlas_w_ = 1 << w_shift;
    atlas_h_ = h;
    for (int f = 0; f < FOGS; ++f) {
        const float ft = (float)f / (FOGS - 1);
        for (int s = 0; s < SHADES; ++s) {
            const float k = 0.35f + 0.9f * (float)s / (SHADES - 1);
            uint16_t* row = cmap_ + ((f * SHADES + s) << 8);
            for (int i = 0; i < 256; ++i) row[i] = mix565(shade565(palette565[i], k), fog_color, ft);
        }
    }
    return true;
}

bool Renderer::enable_zbuffer(bool on)
{
    zbuf_on_ = on;
    if (!on) return true;
    if (!zbuf_ && w_ > 0 && h_ > 0) zbuf_ = reinterpret_cast<uint16_t*>(alloc_bytes(sizeof(uint16_t) * (size_t)w_ * h_, false));
    return true;
}

void Renderer::begin(gfx::Canvas& canvas, const Camera& cam, const Light& light, const Fog& fog)
{
    canvas_ = &canvas;
    px_     = canvas.data();
    w_      = canvas.width();
    h_      = canvas.height();
    cam_pos_ = cam.pos;
    f_ = (cam.target - cam.pos).normalized();
    r_ = cross(f_, Vec3(0, 1, 0)).normalized();
    u_ = cross(r_, f_);
    focal_ = (h_ / 2.f) / tanf(cam.fov_y / 2.f);
    light_ = light;
    fog_   = fog;
    n_ = 0;
    stat_submitted_ = stat_drawn_ = stat_pixels_ = 0;
    if (buckets_) for (int i = 0; i < 2 * BUCKETS; ++i) buckets_[i] = tails_[i] = -1;
    if (zbuf_on_) {
        if (!zbuf_) {
            zbuf_ = reinterpret_cast<uint16_t*>(alloc_bytes(sizeof(uint16_t) * (size_t)w_ * h_, false));
            if (!zbuf_) { CONSOLE_LOGW(TAG, "brak pamieci na Z-bufor - wylaczam"); zbuf_on_ = false; }
        }
        if (zbuf_) memset(zbuf_, 0, sizeof(uint16_t) * (size_t)w_ * h_);
    }
    if (!mask_) {
        mask_ = reinterpret_cast<uint8_t*>(alloc_bytes((size_t)w_ * h_, false));
        if (mask_) memset(mask_, 0, (size_t)w_ * h_);
    }
    mask_x0_ = w_; mask_y0_ = h_; mask_x1_ = -1; mask_y1_ = -1;
}

bool Renderer::project(const Vec3& p, float& sx, float& sy, float& depth) const
{
    const Vec3 d = p - cam_pos_;
    depth = dot(d, f_);
    if (depth < NEAR_Z) return false;
    sx = w_ / 2.f + dot(d, r_) * focal_ / depth;
    sy = h_ / 2.f - dot(d, u_) * focal_ / depth;
    return true;
}

float Renderer::horizon_y() const
{
    // punkt bardzo daleko w poziomym kierunku patrzenia
    Vec3 flat(f_.x, 0, f_.z);
    if (flat.length() < 1e-4f) return h_ / 2.f;
    flat = flat.normalized();
    float sx, sy, d;
    if (!project(cam_pos_ + flat * 100000.f, sx, sy, d)) return h_ / 2.f;
    return sy;
}

float Renderer::light_k(const Vec3& n, const Vec3& half, float specular) const
{
    float k = light_.ambient + light_.diffuse * fmaxf(0.f, dot(n, light_.dir));
    if (specular > 0.f) {
        const float s = fmaxf(0.f, dot(n, half));
        const float s2 = s * s, s4 = s2 * s2, s8 = s4 * s4;
        k += specular * s8 * s8;   // wykladnik 16
    }
    return k > 1.25f ? 1.25f : k;
}

uint16_t Renderer::light_color(uint16_t base, float k, float depth) const
{
    uint16_t c = shade565(base, k);
    if (depth > fog_.start) c = mix565(c, fog_.color, (depth - fog_.start) / (fog_.end - fog_.start));
    return c;
}

float Renderer::tex_row(float k, float depth) const
{
    float s = (k - 0.35f) / 0.9f * (SHADES - 1);
    if (s < 0) s = 0;
    if (s > SHADES - 1) s = SHADES - 1;
    float f = depth > fog_.start ? (depth - fog_.start) / (fog_.end - fog_.start) * (FOGS - 1) : 0.f;
    if (f > FOGS - 1) f = FOGS - 1;
    return (float)((int)(f + 0.5f)) * SHADES + s;
}

void Renderer::submit_view_tri(const CV& a, const CV& b, const CV& c, int layer, bool smooth, bool tex, bool alpha)
{
    // przycinanie do z >= NEAR_Z (Sutherland-Hodgman na jednej plaszczyznie)
    const CV in[3] = { a, b, c };
    CV out[4];
    int n = 0;
    for (int i = 0; i < 3; ++i) {
        const CV& p = in[i];
        const CV& q = in[(i + 1) % 3];
        const bool pin = p.p.z >= NEAR_Z, qin = q.p.z >= NEAR_Z;
        if (pin) out[n++] = p;
        if (pin != qin) {
            const float t = (NEAR_Z - p.p.z) / (q.p.z - p.p.z);
            out[n++] = { lerp(p.p, q.p, t), mix565(p.c, q.c, t), lerpf(p.u, q.u, t), lerpf(p.v, q.v, t), lerpf(p.row, q.row, t) };
        }
    }
    if (n < 3) return;
    const float depth = (a.p.z + b.p.z + c.p.z) / 3.f;
    submit(out, n, layer, smooth, depth, tex, alpha);
}

void Renderer::submit(const CV* poly, int n, int layer, bool smooth, float depth, bool tex, bool alpha)
{
    float sx[4], sy[4], iz[4];
    float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
    for (int i = 0; i < n; ++i) {
        iz[i] = 1.f / poly[i].p.z;
        sx[i] = w_ / 2.f + poly[i].p.x * focal_ * iz[i];
        sy[i] = h_ / 2.f - poly[i].p.y * focal_ * iz[i];
        minx = fminf(minx, sx[i]); maxx = fmaxf(maxx, sx[i]);
        miny = fminf(miny, sy[i]); maxy = fmaxf(maxy, sy[i]);
    }
    if (maxx < 0 || minx > w_ || maxy < 0 || miny > h_) return;   // caly poza ekranem
    if (maxx - minx > 20000.f || maxy - miny > 20000.f) return;   // degeneracja numeryczna
    for (int i = 1; i + 1 < n; ++i) {
        if (n_ >= cap_) return;
        STri& t = tris_[n_];
        const int idx[3] = { 0, i, i + 1 };
        for (int k = 0; k < 3; ++k) {
            const int j = idx[k];
            t.x[k] = sx[j]; t.y[k] = sy[j]; t.c[k] = poly[j].c; t.iz[k] = iz[j];
            t.uz[k] = poly[j].u * iz[j]; t.vz[k] = poly[j].v * iz[j]; t.row[k] = poly[j].row;
        }
        t.z      = depth;
        t.layer  = (uint8_t)(layer ? 1 : 0);
        t.smooth = smooth ? 1 : 0;
        t.tex    = tex ? 1 : 0;
        t.alpha  = alpha ? 1 : 0;
        int b = (int)((depth - depth_bias_) * (BUCKETS - 1) / fog_.end);
        if (b < 0) b = 0;
        if (b > BUCKETS - 1) b = BUCKETS - 1;
        const int slot = t.layer * BUCKETS + b;
        t.next = -1;
        if (tails_[slot] >= 0) tris_[tails_[slot]].next = n_;
        else buckets_[slot] = n_;
        tails_[slot] = n_;
        ++n_;
    }
}

void Renderer::draw_tri(const Vec3& a, const Vec3& b, const Vec3& c, uint16_t color, int layer, bool lit, float depth_bias)
{
    if (!tris_) return;
    ++stat_submitted_;
    const Vec3 va = a - cam_pos_, vb = b - cam_pos_, vc = c - cam_pos_;
    const Vec3 pa(dot(va, r_), dot(va, u_), dot(va, f_));
    const Vec3 pb(dot(vb, r_), dot(vb, u_), dot(vb, f_));
    const Vec3 pc(dot(vc, r_), dot(vc, u_), dot(vc, f_));
    if (pa.z < NEAR_Z && pb.z < NEAR_Z && pc.z < NEAR_Z) return;
    const float depth = (pa.z + pb.z + pc.z) / 3.f;
    if (depth > fog_.end) return;
    uint16_t col = color;
    if (lit) col = light_color(color, light_k(cross(b - a, c - a).normalized(), Vec3(0, 1, 0), 0.f), depth);
    else if (depth > fog_.start) col = mix565(color, fog_.color, (depth - fog_.start) / (fog_.end - fog_.start));
    depth_bias_ = depth_bias;
    submit_view_tri({ pa, col, 0, 0, 0 }, { pb, col, 0, 0, 0 }, { pc, col, 0, 0, 0 }, layer, false, false, false);
    depth_bias_ = 0.f;
}

void Renderer::draw_mesh(const Mesh& mesh, const Mat4& model, int layer, bool cull, float depth_bias)
{
    if (!tris_ || mesh.triangle_count() == 0) return;
    depth_bias_ = depth_bias;
    const Vertex* V = mesh.vertices();
    const Tri*    T = mesh.triangles();
    const int     nt = mesh.triangle_count();
    // wierzcholki: swiat i kamera - liczone na zadanie, z cache na wierzcholek (indeks -> slot)
    Vec3*    wp   = wp_;
    Vec3*    vp   = vp_;
    uint8_t* done = done_;
    const int nv = mesh.vertex_count() > MAX_MESH_VERTS ? MAX_MESH_VERTS : mesh.vertex_count();
    memset(done, 0, (size_t)nv);
    const bool smooth = mesh.smooth && !flat_only_;
    const bool vcol   = mesh.vertex_colors;
    const bool has_tex = atlas_ && cmap_;
    // Macierz z odbiciem (wyznacznik < 0, np. Mat4::heading - jawna baza prawo/gora/przod jest lewoskretna) odwraca
    // nawiniecie trojkatow: normalna z iloczynu wektorowego w swiecie wskazuje do srodka bryly. Bez korekty cull
    // odrzucalby BLIZSZE sciany i widac by bylo lustrzane odbicie modelu (kola "skrecone w druga strone", brak malowania).
    const float det = model.m[0] * (model.m[5] * model.m[10] - model.m[6] * model.m[9]) -
                      model.m[1] * (model.m[4] * model.m[10] - model.m[6] * model.m[8]) +
                      model.m[2] * (model.m[4] * model.m[9] - model.m[5] * model.m[8]);
    const float wind = det < 0.f ? -1.f : 1.f;
    // polwektor do odblasku: miedzy kierunkiem do swiatla a kierunkiem do kamery (od srodka modelu)
    Vec3 half(0, 1, 0);
    if (mesh.specular > 0.f) half = (light_.dir + (cam_pos_ - model.apply(Vec3(0, 0, 0))).normalized()).normalized();
    const float spec = mesh.specular;

    for (int i = 0; i < nt; ++i) {
        const Tri& t = T[i];
        if (t.a >= nv || t.b >= nv || t.c >= nv) continue;
        const int idx[3] = { t.a, t.b, t.c };
        for (int k = 0; k < 3; ++k) {
            const int vi = idx[k];
            if (done[vi]) continue;
            done[vi] = 1;
            wp[vi] = model.apply(V[vi].p);
            const Vec3 d = wp[vi] - cam_pos_;
            vp[vi] = Vec3(dot(d, r_), dot(d, u_), dot(d, f_));
        }
        ++stat_submitted_;
        const Vec3& pa = vp[t.a];
        const Vec3& pb = vp[t.b];
        const Vec3& pc = vp[t.c];
        if (pa.z < NEAR_Z && pb.z < NEAR_Z && pc.z < NEAR_Z) continue;
        const float depth = (pa.z + pb.z + pc.z) / 3.f;
        if (depth > fog_.end) continue;
        const Vec3 n = cross(wp[t.b] - wp[t.a], wp[t.c] - wp[t.a]) * wind;
        if (cull && dot(n, wp[t.a] - cam_pos_) > 0) continue;   // tylem do kamery
        const bool tex = t.tex && has_tex;
        CV cv[3] = { { pa, 0, 0, 0, 0 }, { pb, 0, 0, 0, 0 }, { pc, 0, 0, 0, 0 } };
        if (tex) {
            for (int k = 0; k < 3; ++k) { cv[k].u = (float)t.u[k]; cv[k].v = (float)t.v[k]; }
            if (mesh.unlit) {
                const float row = tex_row(1.0f, depth);
                for (int k = 0; k < 3; ++k) cv[k].row = row;
            } else if (!smooth) {
                const float row = tex_row(light_k(n.normalized(), half, spec), depth);
                for (int k = 0; k < 3; ++k) cv[k].row = row;
            } else {
                for (int k = 0; k < 3; ++k)
                    cv[k].row = tex_row(light_k(model.apply_dir(V[idx[k]].n).normalized(), half, spec), depth);
            }
            submit_view_tri(cv[0], cv[1], cv[2], layer, smooth, true, mesh.alpha_test);
        } else if (mesh.unlit) {
            uint16_t col = t.color;
            if (depth > fog_.start) col = mix565(col, fog_.color, (depth - fog_.start) / (fog_.end - fog_.start));
            for (int k = 0; k < 3; ++k) cv[k].c = col;
            submit_view_tri(cv[0], cv[1], cv[2], layer, false, false, false);
        } else if (!smooth) {
            const uint16_t base = vcol ? V[t.a].c : t.color;
            const uint16_t col  = light_color(base, light_k(n.normalized(), half, spec), depth);
            for (int k = 0; k < 3; ++k) cv[k].c = col;
            submit_view_tri(cv[0], cv[1], cv[2], layer, false, false, false);
        } else {
            for (int k = 0; k < 3; ++k) {
                const int vi = idx[k];
                cv[k].c = light_color(vcol ? V[vi].c : t.color, light_k(model.apply_dir(V[vi].n).normalized(), half, spec), depth);
            }
            submit_view_tri(cv[0], cv[1], cv[2], layer, true, false, false);
        }
    }
    depth_bias_ = 0.f;
}

// ============================================================================ cienie (maska)

void Renderer::draw_shadow(const Mesh& mesh, const Mat4& model)
{
    if (!mask_ || mesh.triangle_count() == 0) return;
    const Vertex* V = mesh.vertices();
    const Tri*    T = mesh.triangles();
    const int     nt = mesh.triangle_count();
    Vec3*    vp   = vp_;
    uint8_t* done = done_;
    const int nv = mesh.vertex_count() > MAX_MESH_VERTS ? MAX_MESH_VERTS : mesh.vertex_count();
    memset(done, 0, (size_t)nv);
    for (int i = 0; i < nt; ++i) {
        const Tri& t = T[i];
        if (t.a >= nv || t.b >= nv || t.c >= nv) continue;
        const int idx[3] = { t.a, t.b, t.c };
        for (int k = 0; k < 3; ++k) {
            const int vi = idx[k];
            if (done[vi]) continue;
            done[vi] = 1;
            const Vec3 d = model.apply(V[vi].p) - cam_pos_;
            vp[vi] = Vec3(dot(d, r_), dot(d, u_), dot(d, f_));
        }
        const CV in[3] = { { vp[t.a], 0, 0, 0, 0 }, { vp[t.b], 0, 0, 0, 0 }, { vp[t.c], 0, 0, 0, 0 } };
        if (in[0].p.z < NEAR_Z && in[1].p.z < NEAR_Z && in[2].p.z < NEAR_Z) continue;
        if ((in[0].p.z + in[1].p.z + in[2].p.z) / 3.f > fog_.start) continue;   // daleko - cien niewidoczny we mgle
        CV out[4];
        int n = 0;
        for (int k = 0; k < 3; ++k) {
            const CV& p = in[k];
            const CV& q = in[(k + 1) % 3];
            const bool pin = p.p.z >= NEAR_Z, qin = q.p.z >= NEAR_Z;
            if (pin) out[n++] = p;
            if (pin != qin) {
                const float tt = (NEAR_Z - p.p.z) / (q.p.z - p.p.z);
                out[n++] = { lerp(p.p, q.p, tt), 0, 0, 0, 0 };
            }
        }
        if (n >= 3) raster_shadow(out, n);
    }
}

void Renderer::raster_shadow(const CV* poly, int n)
{
    float sx[4], sy[4];
    for (int i = 0; i < n; ++i) {
        sx[i] = w_ / 2.f + poly[i].p.x * focal_ / poly[i].p.z;
        sy[i] = h_ / 2.f - poly[i].p.y * focal_ / poly[i].p.z;
    }
    for (int tri = 1; tri + 1 < n; ++tri) {
        int i0 = 0, i1 = tri, i2 = tri + 1;
        if (sy[i0] > sy[i1]) { const int s = i0; i0 = i1; i1 = s; }
        if (sy[i1] > sy[i2]) { const int s = i1; i1 = i2; i2 = s; }
        if (sy[i0] > sy[i1]) { const int s = i0; i0 = i1; i1 = s; }
        const float x0 = sx[i0], y0 = sy[i0], x1 = sx[i1], y1 = sy[i1], x2 = sx[i2], y2 = sy[i2];
        if (y2 - y0 < 0.001f) continue;
        const int ys = imax((int)ceilf(y0 - 0.5f), 0), ye = imin((int)floorf(y2 - 0.5f), h_ - 1);
        const float dx02 = (x2 - x0) / (y2 - y0);
        const float dx01 = (y1 - y0) > 0.0001f ? (x1 - x0) / (y1 - y0) : 0.f;
        const float dx12 = (y2 - y1) > 0.0001f ? (x2 - x1) / (y2 - y1) : 0.f;
        for (int y = ys; y <= ye; ++y) {
            const float yc = y + 0.5f;
            float xa = x0 + dx02 * (yc - y0);
            float xb = (yc < y1) ? x0 + dx01 * (yc - y0) : x1 + dx12 * (yc - y1);
            if (xa > xb) { const float s = xa; xa = xb; xb = s; }
            const int xs = imax((int)ceilf(xa - 0.5f), 0), xe = imin((int)floorf(xb - 0.5f), w_ - 1);
            if (xs > xe) continue;
            memset(mask_ + (size_t)y * w_ + xs, 1, (size_t)(xe - xs + 1));
            if (xs < mask_x0_) mask_x0_ = xs;
            if (xe > mask_x1_) mask_x1_ = xe;
            if (y < mask_y0_) mask_y0_ = y;
            if (y > mask_y1_) mask_y1_ = y;
        }
    }
}

void Renderer::apply_shadows()
{
    if (!mask_ || mask_x1_ < mask_x0_) return;
    for (int y = mask_y0_; y <= mask_y1_; ++y) {
        uint8_t*  m   = mask_ + (size_t)y * w_;
        uint16_t* row = px_ + (size_t)y * w_;
        for (int x = mask_x0_; x <= mask_x1_; ++x) {
            if (m[x]) { row[x] = darken565(row[x]); m[x] = 0; }
        }
    }
    mask_x0_ = w_; mask_y0_ = h_; mask_x1_ = -1; mask_y1_ = -1;
}

// ============================================================================ rasteryzacja

void Renderer::end()
{
    if (!tris_ || !px_) return;
    for (int layer = 0; layer < 2; ++layer) {
        for (int b = BUCKETS - 1; b >= 0; --b) {
            int32_t i = buckets_[layer * BUCKETS + b];
            while (i >= 0) {
                raster(tris_[i]);
                i = tris_[i].next;
            }
        }
        if (layer == 0) apply_shadows();
    }
}

// Jeden rasteryzator skanliniowy: krawedzie z interpolacja atrybutow (float), spany w trzech trybach
// (plaski / Gouraud / tekstura z korekcja perspektywy co TEX_BLOCK px), opcjonalny test Z (1/z liniowe w ekranie).
void Renderer::raster(const STri& t)
{
    ++stat_drawn_;
    int i0 = 0, i1 = 1, i2 = 2;
    if (t.y[i0] > t.y[i1]) { const int s = i0; i0 = i1; i1 = s; }
    if (t.y[i1] > t.y[i2]) { const int s = i1; i1 = i2; i2 = s; }
    if (t.y[i0] > t.y[i1]) { const int s = i0; i0 = i1; i1 = s; }
    const float y0 = t.y[i0], y1 = t.y[i1], y2 = t.y[i2];
    if (y2 - y0 < 0.001f) return;
    const int mode = t.tex ? 2 : ((t.smooth && (t.c[0] != t.c[1] || t.c[1] != t.c[2])) ? 1 : 0);
    Attr A[3];
    const int ii[3] = { i0, i1, i2 };
    for (int k = 0; k < 3; ++k) {
        const int j = ii[k];
        const uint16_t c = t.c[j];
        A[k] = { t.x[j], t.iz[j], t.uz[j], t.vz[j], t.row[j], (float)((c >> 11) & 31), (float)((c >> 5) & 63), (float)(c & 31) };
    }
    const bool zb = zbuf_on_ && zbuf_;
    const uint16_t flat = t.c[0];
    const int ys = imax((int)ceilf(y0 - 0.5f), 0), ye = imin((int)floorf(y2 - 0.5f), h_ - 1);
    const float inv02 = 1.f / (y2 - y0);
    const float inv01 = (y1 - y0) > 0.0001f ? 1.f / (y1 - y0) : 0.f;
    const float inv12 = (y2 - y1) > 0.0001f ? 1.f / (y2 - y1) : 0.f;
    const int    tw_mask = atlas_w_ - 1, th_mask = atlas_h_ - 1, tshift = atlas_shift_;
    const uint8_t* atlas = atlas_;
    const uint16_t* cmap = cmap_;

    for (int y = ys; y <= ye; ++y) {
        const float yc = y + 0.5f;
        Attr a = lerp_attr(A[0], A[2], (yc - y0) * inv02);
        Attr b = (yc < y1) ? lerp_attr(A[0], A[1], (yc - y0) * inv01) : lerp_attr(A[1], A[2], (yc - y1) * inv12);
        if (a.x > b.x) { const Attr s = a; a = b; b = s; }
        const int xs = imax((int)ceilf(a.x - 0.5f), 0), xe = imin((int)floorf(b.x - 0.5f), w_ - 1);
        if (xs > xe) continue;
        const float span = b.x - a.x > 0.0001f ? b.x - a.x : 1.f;
        const float inv_span = 1.f / span;
        const float t0 = (xs + 0.5f - a.x) * inv_span;   // parametr pierwszego piksela
        uint16_t* row  = px_ + (size_t)y * w_;
        uint16_t* zrow = zb ? zbuf_ + (size_t)y * w_ : nullptr;
        stat_pixels_ += xe - xs + 1;
        // 1/z w 8.16 (wartosc 0..65535 z 8 bitami ulamka -> miesci sie w int32)
        const float diz = (b.iz - a.iz) * inv_span;
        int32_t izq = (int32_t)((a.iz + diz * (xs + 0.5f - a.x)) * 65535.f * 256.f);
        const int32_t dizq = (int32_t)(diz * 65535.f * 256.f);

        if (mode == 0) {
            if (!zb) {
                for (int x = xs; x <= xe; ++x) row[x] = flat;
            } else {
                for (int x = xs; x <= xe; ++x, izq += dizq) {
                    const uint16_t zq = (uint16_t)(izq >> 8);
                    if (zq >= zrow[x]) { zrow[x] = zq; row[x] = flat; }
                }
            }
        } else if (mode == 1) {
            const float dr = (b.r - a.r) * inv_span, dg = (b.g - a.g) * inv_span, db = (b.b - a.b) * inv_span;
            int32_t cr = (int32_t)((a.r + dr * (xs + 0.5f - a.x)) * 65536.f);
            int32_t cg = (int32_t)((a.g + dg * (xs + 0.5f - a.x)) * 65536.f);
            int32_t cb = (int32_t)((a.b + db * (xs + 0.5f - a.x)) * 65536.f);
            const int32_t drq = (int32_t)(dr * 65536.f), dgq = (int32_t)(dg * 65536.f), dbq = (int32_t)(db * 65536.f);
            for (int x = xs; x <= xe; ++x, cr += drq, cg += dgq, cb += dbq, izq += dizq) {
                if (zb) {
                    const uint16_t zq = (uint16_t)(izq >> 8);
                    if (zq < zrow[x]) continue;
                    zrow[x] = zq;
                }
                const int rr = imin(imax(cr >> 16, 0), 31), gg = imin(imax(cg >> 16, 0), 63), bb = imin(imax(cb >> 16, 0), 31);
                row[x] = pack565(rr, gg, bb);
            }
        } else {
            // tekstura: u/z, v/z, 1/z liniowe w ekranie; co TEX_BLOCK px dzielenie i liniowe u, v w bloku
            const float duz = (b.uz - a.uz) * inv_span, dvz = (b.vz - a.vz) * inv_span, drow = (b.row - a.row) * inv_span;
            float uz = a.uz + duz * (xs + 0.5f - a.x), vz = a.vz + dvz * (xs + 0.5f - a.x), iz = a.iz + diz * (xs + 0.5f - a.x);
            float rowf = a.row + drow * (xs + 0.5f - a.x);
            float z = 1.f / iz;
            float u0 = uz * z, v0 = vz * z;
            const bool alpha = t.alpha != 0;
            (void)t0;
            for (int x = xs; x <= xe;) {
                const int len = imin(TEX_BLOCK, xe - x + 1);
                const float uz1 = uz + duz * len, vz1 = vz + dvz * len, iz1 = iz + diz * len;
                const float z1 = 1.f / iz1;
                const float u1 = uz1 * z1, v1 = vz1 * z1;
                const int32_t uq_step = (int32_t)((u1 - u0) / len * 65536.f), vq_step = (int32_t)((v1 - v0) / len * 65536.f);
                int32_t uq = (int32_t)(u0 * 65536.f), vq = (int32_t)(v0 * 65536.f);
                int r = (int)(rowf + drow * (len * 0.5f) + 0.5f);
                if (r < 0) r = 0;
                if (r >= SHADES * FOGS) r = SHADES * FOGS - 1;
                const uint16_t* crow = cmap + (r << 8);
                const int xend = x + len;
                if (!zb) {
                    if (!alpha) {
                        for (; x < xend; ++x, uq += uq_step, vq += vq_step)
                            row[x] = crow[atlas[(((vq >> 16) & th_mask) << tshift) | ((uq >> 16) & tw_mask)]];
                    } else {
                        for (; x < xend; ++x, uq += uq_step, vq += vq_step) {
                            const uint8_t idx = atlas[(((vq >> 16) & th_mask) << tshift) | ((uq >> 16) & tw_mask)];
                            if (idx) row[x] = crow[idx];
                        }
                    }
                    izq += dizq * len;
                } else {
                    for (; x < xend; ++x, uq += uq_step, vq += vq_step, izq += dizq) {
                        const uint16_t zq = (uint16_t)(izq >> 8);
                        if (zq < zrow[x]) continue;
                        const uint8_t idx = atlas[(((vq >> 16) & th_mask) << tshift) | ((uq >> 16) & tw_mask)];
                        if (alpha && !idx) continue;
                        zrow[x] = zq;
                        row[x] = crow[idx];
                    }
                }
                uz = uz1; vz = vz1; iz = iz1; u0 = u1; v0 = v1; rowf += drow * len;
            }
        }
    }
}

}  // namespace gfx3d
