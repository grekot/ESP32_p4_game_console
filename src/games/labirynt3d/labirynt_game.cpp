#include "games/labirynt3d/labirynt_game.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "engine/math2d.h"
#include "gfx/palette.h"
#include "gfx/png.h"
#include "gfx/text.h"

namespace labirynt {

namespace {

const char* TAG = "labirynt";

// Mapa 32 x 24 kafelki. '#' cegla, '=' kamien, 'D' drzwi (stale), 'o' moneta, 'S' start, 'E' wyjscie (portal).
const char* const MAP[24] = {
    "################################",
    "#S     #     o   =   o  #     E#",
    "#  ##  #  ####   =  ###D#  ### #",
    "#  ##  #  #  #   =    # #    # #",
    "#      D   o #   ===  # # o  # #",
    "####### ###  #        # ###### #",
    "#     # #    ####D##  #      # #",
    "#  o  # # o  #     #  ####D  # #",
    "#     # #    #  o  #     #   # #",
    "##D####  ##### #####  #  #  ## #",
    "#      #       #      #  #     #",
    "#  # #### ####  ##### #  #######",
    "#  #    #    #  #   # #        #",
    "#  # o  # # o#  # o D #  o  #  #",
    "#  #    # #  #  #   # ####  #  #",
    "#  ######D#### ###### #     #  #",
    "#            #        #  ####  #",
    "###### ####  ####D#####  #  #  #",
    "#    # #  #       #      #  #  #",
    "# o  # #o #   o   #  ##### ##  #",
    "#    D #  #########  #      #  #",
    "#  ###   ##       #  #  o   D  #",
    "#       o   ###   D  #      #  #",
    "################################",
};

constexpr float MOVE_SPEED = 2.6f;    // kafelki/s
constexpr float RUN_SPEED  = 4.2f;
constexpr float TURN_SPEED = 2.4f;    // rad/s
constexpr float RADIUS     = 0.22f;   // "grubosc" gracza
constexpr float FOG        = 0.10f;   // przyciemnianie z odlegloscia

bool solid_tile(char t) { return t == '#' || t == '=' || t == 'D'; }

int texture_index(char t)
{
    switch (t) {
        case '#': return 0;
        case '=': return 1;
        case 'D': return 2;
        default:  return 0;
    }
}

// Przyciemnienie RGB565 o wspolczynnik k (0..1) - 32 kroki, bez float w petli pikseli.
inline uint16_t darken(uint16_t c, int k32)
{
    const uint32_t r = ((c >> 11) & 0x1F) * k32 >> 5;
    const uint32_t g = ((c >> 5) & 0x3F) * k32 >> 5;
    const uint32_t b = (c & 0x1F) * k32 >> 5;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

inline int fog_k32(float dist, bool side)
{
    float k = 1.f / (1.f + dist * FOG);
    if (side) k *= 0.72f;   // sciany N/S ciemniejsze - daje wrazenie bryly
    if (k < 0.18f) k = 0.18f;
    return (int)(k * 32.f);
}

}  // namespace

// ============================================================================ cykl zycia

void LabiryntGame::init(gfx::Canvas&)
{
    if (!assets_loaded_) {
        assets_loaded_ = true;
        tex_[0] = gfx::load_png("labirynt3d/brick.png");
        tex_[1] = gfx::load_png("labirynt3d/stone.png");
        tex_[2] = gfx::load_png("labirynt3d/door.png");
        tex_[3] = gfx::load_png("labirynt3d/exit.png");
        coin_   = gfx::load_png("labirynt3d/coin.png");
        portal_ = gfx::load_png("labirynt3d/portal.png");
        CONSOLE_LOGI(TAG, "tekstury: %s", tex_[0].px ? "PNG z assets/labirynt3d" : "BRAK - kolory zastepcze");
    }
    // Gradient sufitu (ciemny -> ciemniejszy przy horyzoncie) i podlogi (jasniejsza blizej gracza).
    for (int y = 0; y < H; ++y) {
        const float t = (float)(y < H / 2 ? (H / 2 - y) : (y - H / 2)) / (float)(H / 2);   // 0 przy horyzoncie
        const int   k = (int)((0.25f + 0.75f * t) * 32.f);
        ceil_row_[y]  = darken(gfx::rgb565(70, 80, 110), k);
        floor_row_[y] = darken(gfx::rgb565(110, 95, 80), k);
    }
    new_game();
}

void LabiryntGame::new_game()
{
    load_level();
    state_ = State::Title;
    time_  = 0;
}

void LabiryntGame::load_level()
{
    item_count_ = 0;
    coins_ = coins_total_ = 0;
    doors_ = 0;
    px_ = 1.5f;
    py_ = 1.5f;
    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            char t = MAP[r][c];
            if (t == 'S') {
                px_ = c + 0.5f;
                py_ = r + 0.5f;
                t = ' ';
            } else if ((t == 'o' || t == 'E') && item_count_ < MAX_ITEMS) {
                Item& it  = items_[item_count_++];
                it.x      = c + 0.5f;
                it.y      = r + 0.5f;
                it.taken  = false;
                it.portal = (t == 'E');
                if (!it.portal) ++coins_total_;
                t = ' ';
            }
            tiles_[r][c] = t;
        }
    }
    set_angle(0.f);
}

bool LabiryntGame::solid_at(int c, int r) const { return solid_tile(tile_at(c, r)); }

void LabiryntGame::set_angle(float a)
{
    angle_  = a;
    dir_x_  = cosf(a);
    dir_y_  = sinf(a);
    plane_x_ = -dir_y_ * 0.66f;   // szerokosc pola widzenia ~66 stopni
    plane_y_ = dir_x_ * 0.66f;
}

void LabiryntGame::try_move(float dx, float dy)
{
    // Osobno w X i Y, z marginesem RADIUS - gracz slizga sie po scianach zamiast w nie wbijac.
    const float nx = px_ + dx;
    const float sx = dx > 0 ? RADIUS : -RADIUS;
    if (!solid_at((int)floorf(nx + sx), (int)floorf(py_ - RADIUS)) &&
        !solid_at((int)floorf(nx + sx), (int)floorf(py_ + RADIUS))) {
        px_ = nx;
    }
    const float ny = py_ + dy;
    const float sy = dy > 0 ? RADIUS : -RADIUS;
    if (!solid_at((int)floorf(px_ - RADIUS), (int)floorf(ny + sy)) &&
        !solid_at((int)floorf(px_ + RADIUS), (int)floorf(ny + sy))) {
        py_ = ny;
    }
}

// ============================================================================ logika

void LabiryntGame::update(float dt, const input::PadState& pad)
{
    anim_ += dt;
    switch (state_) {
        case State::Title:
            if (pad.a_pressed || pad.b_pressed || (pad.any_pressed && !pad.start_pressed)) {
                state_ = State::Playing;
                time_  = 0;
            }
            break;

        case State::Playing: {
            time_ += dt;
            if (pad.left)  set_angle(angle_ - TURN_SPEED * dt);
            if (pad.right) set_angle(angle_ + TURN_SPEED * dt);
            const float speed = pad.b ? RUN_SPEED : MOVE_SPEED;
            float mx = 0, my = 0;
            if (pad.up)   { mx += dir_x_; my += dir_y_; }
            if (pad.down) { mx -= dir_x_; my -= dir_y_; }
            if (pad.x)    { mx += dir_y_; my -= dir_x_; }   // krok w bok (strafe)
            if (pad.y)    { mx -= dir_y_; my += dir_x_; }
            if (mx != 0 || my != 0) {
                try_move(mx * speed * dt, my * speed * dt);
                bob_ += dt * 9.f;
            }
            // drzwi: otwieraja sie (znikaja) gdy gracz podejdzie blisko - najprostsza wersja drzwi z Wolfensteina
            {
                const int cx = (int)floorf(px_), cy = (int)floorf(py_);
                for (int r = cy - 1; r <= cy + 1; ++r) {
                    for (int c = cx - 1; c <= cx + 1; ++c) {
                        if (tile_at(c, r) != 'D') continue;
                        const float dx = c + 0.5f - px_, dy = r + 0.5f - py_;
                        if (dx * dx + dy * dy < 0.9f * 0.9f) { tiles_[r][c] = ' '; ++doors_; }
                    }
                }
            }
            // przedmioty
            for (int i = 0; i < item_count_; ++i) {
                Item& it = items_[i];
                if (it.taken) continue;
                const float dx = it.x - px_, dy = it.y - py_;
                if (dx * dx + dy * dy < 0.45f * 0.45f) {
                    if (it.portal) {
                        state_ = State::Won;
                    } else {
                        it.taken = true;
                        ++coins_;
                    }
                }
            }
            break;
        }

        case State::Won:
            if (pad.a_pressed) new_game();
            break;
    }
}

// ============================================================================ rysowanie

void LabiryntGame::draw_walls(gfx::Canvas& c)
{
    uint16_t* px = c.data();
    const int  horizon = H / 2 + (int)(sinf(bob_) * 3.f);   // lekkie kolysanie przy chodzeniu

    // Sufit i podloga: gotowe gradienty wiersz po wierszu.
    for (int y = 0; y < H; ++y) {
        const uint16_t col = y < horizon ? ceil_row_[engine::iclamp(y + (H / 2 - horizon), 0, H - 1)]
                                         : floor_row_[engine::iclamp(y - (horizon - H / 2), 0, H - 1)];
        uint32_t* row = reinterpret_cast<uint32_t*>(px + (size_t)y * W);
        const uint32_t v = ((uint32_t)col << 16) | col;
        for (int x = 0; x < W / 2; ++x) row[x] = v;
    }

    for (int x = 0; x < W; ++x) {
        // Promien dla kolumny x (DDA po kafelkach)
        const float cam    = 2.f * x / (float)W - 1.f;
        const float rdx    = dir_x_ + plane_x_ * cam;
        const float rdy    = dir_y_ + plane_y_ * cam;
        int         map_x  = (int)floorf(px_);
        int         map_y  = (int)floorf(py_);
        const float ddx    = rdx == 0 ? 1e30f : fabsf(1.f / rdx);
        const float ddy    = rdy == 0 ? 1e30f : fabsf(1.f / rdy);
        int         step_x, step_y;
        float       side_x, side_y;
        if (rdx < 0) { step_x = -1; side_x = (px_ - map_x) * ddx; }
        else         { step_x = 1;  side_x = (map_x + 1.f - px_) * ddx; }
        if (rdy < 0) { step_y = -1; side_y = (py_ - map_y) * ddy; }
        else         { step_y = 1;  side_y = (map_y + 1.f - py_) * ddy; }

        bool side = false;
        char tile = '#';
        for (int i = 0; i < 64; ++i) {
            if (side_x < side_y) { side_x += ddx; map_x += step_x; side = false; }
            else                 { side_y += ddy; map_y += step_y; side = true; }
            if (map_x < 0 || map_y < 0 || map_x >= MAP_COLS || map_y >= MAP_ROWS) break;
            tile = tile_at(map_x, map_y);
            if (solid_tile(tile)) break;
        }
        const float dist = side ? (side_y - ddy) : (side_x - ddx);
        zbuf_[x] = dist;

        const int line_h = dist > 0.01f ? (int)(H / dist) : H * 8;
        int y0 = horizon - line_h / 2;
        int y1 = horizon + line_h / 2;
        const int y0c = engine::imax(y0, 0), y1c = engine::imin(y1, H - 1);

        const int          k32 = fog_k32(dist, side);
        const gfx::Sprite& tex = tex_[texture_index(tile)];
        if (tex.px) {
            float wall_x = side ? (px_ + dist * rdx) : (py_ + dist * rdy);
            wall_x -= floorf(wall_x);
            int tx = (int)(wall_x * tex.w);
            if ((!side && rdx > 0) || (side && rdy < 0)) tx = tex.w - tx - 1;
            const int32_t step = (int32_t)((tex.h << 16) / (line_h > 0 ? line_h : 1));
            int32_t       tpos = (int32_t)((y0c - y0) * (int64_t)step);
            for (int y = y0c; y <= y1c; ++y) {
                const int ty = (tpos >> 16) & (tex.h - 1);
                tpos += step;
                px[(size_t)y * W + x] = darken(tex.px[(size_t)ty * tex.w + tx], k32);
            }
        } else {
            static const uint16_t FLAT[3] = { gfx::rgb565(176, 66, 48), gfx::rgb565(128, 132, 140), gfx::rgb565(150, 100, 52) };
            const uint16_t col = darken(FLAT[texture_index(tile) % 3], k32);
            for (int y = y0c; y <= y1c; ++y) px[(size_t)y * W + x] = col;
        }
    }
}

void LabiryntGame::draw_sprites(gfx::Canvas& c)
{
    uint16_t* px = c.data();
    // Kolejnosc: od najdalszych (prosty wybor po odleglosci, kilkadziesiat przedmiotow)
    int   order[MAX_ITEMS];
    float dist2[MAX_ITEMS];
    int   n = 0;
    for (int i = 0; i < item_count_; ++i) {
        if (items_[i].taken) continue;
        const float dx = items_[i].x - px_, dy = items_[i].y - py_;
        order[n] = i;
        dist2[n] = dx * dx + dy * dy;
        ++n;
    }
    for (int i = 1; i < n; ++i) {   // insertion sort malejaco
        int   oi = order[i];
        float di = dist2[i];
        int   j  = i - 1;
        while (j >= 0 && dist2[j] < di) { order[j + 1] = order[j]; dist2[j + 1] = dist2[j]; --j; }
        order[j + 1] = oi;
        dist2[j + 1] = di;
    }

    const float inv_det = 1.f / (plane_x_ * dir_y_ - dir_x_ * plane_y_);
    const int   horizon = H / 2 + (int)(sinf(bob_) * 3.f);
    for (int k = 0; k < n; ++k) {
        const Item&        it  = items_[order[k]];
        const gfx::Sprite& spr = it.portal ? portal_ : coin_;
        const float sx = it.x - px_, sy = it.y - py_;
        const float tx = inv_det * (dir_y_ * sx - dir_x_ * sy);
        const float ty = inv_det * (-plane_y_ * sx + plane_x_ * sy);   // glebia przed kamera
        if (ty <= 0.05f) continue;
        const int screen_x = (int)((W / 2) * (1.f + tx / ty));
        const float bob_y  = it.portal ? 0.f : sinf(anim_ * 3.f + it.x) * 0.06f;   // monety lekko podskakuja
        const int size     = (int)(H / ty * (it.portal ? 0.8f : 0.45f));
        if (size <= 0) continue;
        const int base_y   = horizon + (int)((0.25f + bob_y) * H / ty);   // stoja na podlodze
        const int y0 = base_y - size, y1 = base_y;
        const int x0 = screen_x - size / 2, x1 = x0 + size;
        const int k32 = fog_k32(ty, false);

        for (int x = engine::imax(x0, 0); x < engine::imin(x1, W); ++x) {
            if (ty >= zbuf_[x]) continue;   // za sciana
            if (!spr.px) {
                for (int y = engine::imax(y0, 0); y < engine::imin(y1, H); ++y)
                    px[(size_t)y * W + x] = darken(it.portal ? gfx::pal::GREEN : gfx::pal::YELLOW, k32);
                continue;
            }
            const int tcol = (x - x0) * spr.w / size;
            for (int y = engine::imax(y0, 0); y < engine::imin(y1, H); ++y) {
                const int      trow = (y - y0) * spr.h / size;
                const uint16_t col  = spr.px[(size_t)trow * spr.w + tcol];
                if (col != gfx::TRANSPARENT) px[(size_t)y * W + x] = darken(col, k32);
            }
        }
    }
}

void LabiryntGame::draw_minimap(gfx::Canvas& c) const
{
    constexpr int S = 2;   // px na kafelek
    const int x0 = W - MAP_COLS * S - 6, y0 = 6;
    c.fill_rect(x0 - 2, y0 - 2, MAP_COLS * S + 4, MAP_ROWS * S + 4, gfx::rgb565(10, 12, 20));
    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int col = 0; col < MAP_COLS; ++col) {
            const char t = tile_at(col, r);
            if (solid_tile(t)) c.fill_rect(x0 + col * S, y0 + r * S, S, S, t == 'D' ? gfx::pal::BROWN : gfx::pal::GRAY);
        }
    }
    for (int i = 0; i < item_count_; ++i) {
        if (items_[i].taken) continue;
        c.fill_rect(x0 + (int)(items_[i].x * S) - 1, y0 + (int)(items_[i].y * S) - 1, 2, 2,
                    items_[i].portal ? gfx::pal::GREEN : gfx::pal::YELLOW);
    }
    const int pxm = x0 + (int)(px_ * S), pym = y0 + (int)(py_ * S);
    c.fill_rect(pxm - 1, pym - 1, 3, 3, gfx::pal::RED);
    c.line(pxm, pym, pxm + (int)(dir_x_ * 5), pym + (int)(dir_y_ * 5), gfx::pal::WHITE);
}

void LabiryntGame::draw_hud(gfx::Canvas& c)
{
    char buf[48];
    snprintf(buf, sizeof(buf), "Monety %d / %d", coins_, coins_total_);
    gfx::draw_text_px(c, 6, 4, buf, gfx::pal::WHITE, 14);
    snprintf(buf, sizeof(buf), "%d s", (int)time_);
    gfx::draw_text_px(c, 6, 20, buf, gfx::pal::GRAY, 12);

    if (state_ == State::Title) {
        c.fill_rect(0, 78, W, 84, gfx::rgb565(10, 12, 20));
        gfx::draw_text_px(c, (W - gfx::text_width_px("Labirynt 3D", 32)) / 2, 84, "Labirynt 3D", gfx::pal::WHITE, 32);
        gfx::draw_text_px(c, (W - gfx::text_width_px("Zbierz monety i znajdz zielony portal. Drzwi otwieraja sie same.", 12)) / 2, 122,
                          "Zbierz monety i znajdz zielony portal. Drzwi otwieraja sie same.", gfx::pal::GRAY, 12);
        if (fmodf(anim_, 1.f) < 0.6f)
            gfx::draw_text_px(c, (W - gfx::text_width_px("A - start   strzalki - ruch   B - bieg   X/Y - krok w bok", 12)) / 2, 140,
                              "A - start   strzalki - ruch   B - bieg   X/Y - krok w bok", gfx::pal::WHITE, 12);
    } else if (state_ == State::Won) {
        c.fill_rect(0, 78, W, 84, gfx::rgb565(10, 40, 30));
        gfx::draw_text_px(c, (W - gfx::text_width_px("Wyjscie!", 32)) / 2, 84, "Wyjscie!", gfx::pal::GREEN, 32);
        snprintf(buf, sizeof(buf), "Czas %d s, monety %d / %d", (int)time_, coins_, coins_total_);
        gfx::draw_text_px(c, (W - gfx::text_width_px(buf, 14)) / 2, 124, buf, gfx::pal::WHITE, 14);
        gfx::draw_text_px(c, (W - gfx::text_width_px("A - jeszcze raz", 12)) / 2, 144, "A - jeszcze raz", gfx::pal::GRAY, 12);
    }
}

void LabiryntGame::render(gfx::Canvas& c)
{
    draw_walls(c);
    draw_sprites(c);
    draw_minimap(c);
    draw_hud(c);
}

void LabiryntGame::debug_line(char* buf, size_t n) const
{
    static const char* const NAMES[] = { "TITLE", "PLAY", "WON" };
    snprintf(buf, n, "%-5s x=%.2f y=%.2f ang=%.2f coins=%d/%d doors=%d time=%.1f", NAMES[(int)state_], (double)px_,
             (double)py_, (double)angle_, coins_, coins_total_, doors_, (double)time_);
}

}  // namespace labirynt
