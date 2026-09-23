#include "input/virtual_pad.h"

#include "gfx/text.h"

namespace input {

namespace {

struct Zone {
    int x0, y0, x1, y1;   // [x0, x1) x [y0, y1) w pikselach plotna 400x240
    bool contains(int x, int y) const { return x >= x0 && x < x1 && y >= y0 && y < y1; }
};

// Strefy sa rozmyslnie wieksze niz rysowane obrysy - palec nie musi byc precyzyjny. Wspolrzedne plotna 800x480.
constexpr Zone ZONE_LEFT  = {   0, 260, 136, 480 };
constexpr Zone ZONE_RIGHT = { 136, 260, 272, 480 };
constexpr Zone ZONE_B     = { 516, 260, 656, 480 };
constexpr Zone ZONE_A     = { 656, 260, 800, 480 };

// Obrysy (mniejsze, dyskretne)
struct Box { int x, y, w, h; const char* label; };
constexpr Box BOX_LEFT  = {  12, 352, 112, 112, "<" };
constexpr Box BOX_RIGHT = { 144, 352, 112, 112, ">" };
constexpr Box BOX_B     = { 536, 352, 112, 112, "B" };
constexpr Box BOX_A     = { 676, 352, 112, 112, "A" };
constexpr int LABEL_PX = 40;     // wygladzana czcionka (gfx/text.h)

}  // namespace

void VirtualPad::begin_frame()
{
    cur_      = PadState{};
    touching_ = false;
}

void VirtualPad::feed_touch(const TouchPoint* pts, int n)
{
    for (int i = 0; i < n; ++i) {
        const int x = pts[i].x;
        const int y = pts[i].y;
        touching_ = true;
        if (ZONE_LEFT.contains(x, y))  cur_.left  = true;
        if (ZONE_RIGHT.contains(x, y)) cur_.right = true;
        if (ZONE_A.contains(x, y))     cur_.a     = true;
        if (ZONE_B.contains(x, y))     cur_.b     = true;
    }
}

void VirtualPad::feed_keys(const PadState& k)
{
    cur_.up     |= k.up;
    cur_.down   |= k.down;
    cur_.left   |= k.left;
    cur_.right  |= k.right;
    cur_.a      |= k.a;
    cur_.b      |= k.b;
    cur_.x      |= k.x;
    cur_.y      |= k.y;
    cur_.start  |= k.start;
    cur_.select |= k.select;

    // Osie galki przechodza bez zmian (dotyk ich nie dotyczy).
    cur_.stick_x = k.stick_x;
    cur_.stick_y = k.stick_y;

    if (k.any_held()) {
        touching_  = true;
        keys_used_ = true;
    }
}

void VirtualPad::end_frame()
{
    out_                = cur_;
    out_.a_pressed      = cur_.a && !prev_.a;
    out_.b_pressed      = cur_.b && !prev_.b;
    out_.x_pressed      = cur_.x && !prev_.x;
    out_.y_pressed      = cur_.y && !prev_.y;
    out_.start_pressed  = cur_.start && !prev_.start;
    out_.select_pressed = cur_.select && !prev_.select;
    out_.any_pressed    = touching_ && !touching_prev_;
    prev_               = cur_;
    touching_prev_      = touching_;
}

void VirtualPad::draw(gfx::Canvas& c) const
{
    const uint16_t idle   = gfx::rgb565(255, 255, 255);
    const uint16_t active = gfx::rgb565(255, 230, 80);

    auto draw_box = [&](const Box& b, bool on) {
        const uint16_t col = on ? active : idle;
        c.draw_rect(b.x, b.y, b.w, b.h, col);
        c.draw_rect(b.x + 1, b.y + 1, b.w - 2, b.h - 2, col);
        if (on) c.draw_rect(b.x + 2, b.y + 2, b.w - 4, b.h - 4, col);
        const int tw = gfx::text_width_px(b.label, LABEL_PX);
        const int th = gfx::text_height_px(LABEL_PX);
        gfx::draw_text_px(c, b.x + (b.w - tw) / 2, b.y + (b.h - th) / 2, b.label, col, LABEL_PX);
    };

    draw_box(BOX_LEFT,  out_.left);
    draw_box(BOX_RIGHT, out_.right);
    draw_box(BOX_B,     out_.b);
    draw_box(BOX_A,     out_.a);
}

}  // namespace input
