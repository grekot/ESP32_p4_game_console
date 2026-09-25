#include "ui/lvgl_glue.h"

#include <string.h>

#include "core/log.h"
#include "engine/rng.h"
#include "engine/screen.h"
#include "lvgl.h"
#include "platform/platform.h"

namespace ui {

namespace {

const char* TAG = "ui";

// Bufor roboczy LVGL: 80 wierszy z 480 (800x80x2 B = 128 kB). Wiecej nie daje zysku, bo i tak kopiujemy fragmenty.
constexpr int LVGL_BUF_ROWS = 80;

lv_display_t* s_disp       = nullptr;
lv_indev_t*   s_indev      = nullptr;   // dotyk
lv_indev_t*   s_keydev     = nullptr;   // klawiatura konsoli
lv_group_t*   s_group      = nullptr;
input::PadState s_keys{};               // stan podany przez app::frame()
uint16_t*     s_canvas     = nullptr;   // plotno konsoli (cel kopiowania)
uint16_t*     s_lvgl_buf   = nullptr;   // bufor roboczy LVGL
lv_obj_t*     s_background = nullptr;   // widget z nieruchoma klatka gry pod UI

// LVGL narysowalo fragment do swojego bufora - przepisujemy go na plotno.
void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    if (s_canvas && area && px_map) {
        const int x1 = area->x1 < 0 ? 0 : area->x1;
        const int y1 = area->y1 < 0 ? 0 : area->y1;
        const int x2 = area->x2 >= engine::CANVAS_W ? engine::CANVAS_W - 1 : area->x2;
        const int y2 = area->y2 >= engine::CANVAS_H ? engine::CANVAS_H - 1 : area->y2;

        const int src_stride = area->x2 - area->x1 + 1;   // szerokosc fragmentu wg LVGL
        const uint16_t* src = reinterpret_cast<const uint16_t*>(px_map);

        for (int y = y1; y <= y2; ++y) {
            const uint16_t* s = src + (size_t)(y - area->y1) * src_stride + (x1 - area->x1);
            uint16_t*       d = s_canvas + (size_t)y * engine::CANVAS_W + x1;
            memcpy(d, s, (size_t)(x2 - x1 + 1) * sizeof(uint16_t));
        }
    }
    lv_display_flush_ready(disp);
}

void touch_read_cb(lv_indev_t*, lv_indev_data_t* data)
{
    input::TouchPoint pts[input::MAX_TOUCH_POINTS];
    const int n = platform::read_touch(pts, input::MAX_TOUCH_POINTS);
    if (n > 0) {
        data->point.x = pts[0].x;
        data->point.y = pts[0].y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// Klawisze konsoli -> polecenia nawigacyjne LVGL.
void key_read_cb(lv_indev_t*, lv_indev_data_t* data)
{
    uint32_t key   = 0;
    bool     press = true;

    if (s_keys.down)          key = LV_KEY_NEXT;    // nastepny widget w grupie
    else if (s_keys.up)       key = LV_KEY_PREV;
    else if (s_keys.right)    key = LV_KEY_NEXT;
    else if (s_keys.left)     key = LV_KEY_PREV;
    else if (s_keys.a)        key = LV_KEY_ENTER;   // zatwierdz
    else if (s_keys.start)    key = LV_KEY_ENTER;
    else if (s_keys.b)        key = LV_KEY_ESC;     // cofnij
    else if (s_keys.select)   key = LV_KEY_ESC;
    else                      press = false;

    data->key   = key;
    data->state = press ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// Zegar LVGL. W trybie powtarzalnym (testy --frames, klatki liczone szybciej niz w czasie rzeczywistym) zegar jest
// wirtualny: 1/60 s na kazde ui::tick(). Bez tego LVGL (odswiezanie co 16 ms, animacje) widzialo ulamek sekundy
// na 30 klatek i zrzut pokazywal stary obraz.
uint32_t s_virtual_ms = 0;
int      s_virtual_frac = 0;

uint32_t tick_cb()
{
    return engine::deterministic() ? s_virtual_ms : platform::millis();
}

}  // namespace

bool init(uint16_t* canvas_buf)
{
    if (!canvas_buf) return false;
    s_canvas = canvas_buf;

    const size_t buf_pixels = (size_t)engine::CANVAS_W * LVGL_BUF_ROWS;
    s_lvgl_buf = platform::alloc_pixels(buf_pixels, /*fast=*/false);
    if (!s_lvgl_buf) {
        CONSOLE_LOGE(TAG, "brak pamieci na bufor roboczy LVGL");
        return false;
    }

    lv_init();
    lv_tick_set_cb(tick_cb);

    s_disp = lv_display_create(engine::CANVAS_W, engine::CANVAS_H);
    if (!s_disp) {
        CONSOLE_LOGE(TAG, "lv_display_create nie powiodlo sie");
        return false;
    }
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_disp, flush_cb);
    lv_display_set_buffers(s_disp, s_lvgl_buf, nullptr,
                           (uint32_t)(buf_pixels * sizeof(uint16_t)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, touch_read_cb);
    lv_indev_set_display(s_indev, s_disp);

    // Klawiatura konsoli: nawigacja po UI bez dotykania ekranu.
    s_group  = lv_group_create();
    s_keydev = lv_indev_create();
    lv_indev_set_type(s_keydev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keydev, key_read_cb);
    lv_indev_set_display(s_keydev, s_disp);
    lv_indev_set_group(s_keydev, s_group);

    CONSOLE_LOGI(TAG, "LVGL %d.%d.%d, ekran %dx%d RGB565, bufor %d wierszy",
              LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
              engine::CANVAS_W, engine::CANVAS_H, LVGL_BUF_ROWS);
    return true;
}

void set_background_frame(const uint16_t* frame)
{
    if (s_background) {
        lv_obj_delete(s_background);
        s_background = nullptr;
    }
    if (!frame) return;

    // Widget canvas na samym spodzie: daje LVGL nieprzezroczyste tlo do kompozycji.
    s_background = lv_canvas_create(lv_screen_active());
    lv_canvas_set_buffer(s_background, const_cast<uint16_t*>(frame),
                         engine::CANVAS_W, engine::CANVAS_H, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(s_background, 0, 0);
    lv_obj_remove_flag(s_background, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_background(s_background);
}

void feed_keys(const input::PadState& keys)
{
    s_keys = keys;
}

lv_group_t* nav_group()
{
    return s_group;
}

void tick()
{
    if (!s_disp) return;
    if (engine::deterministic()) {
        s_virtual_frac += 1000;   // 1000/60 ms na klatke, bez dryfu
        s_virtual_ms += (uint32_t)(s_virtual_frac / 60);
        s_virtual_frac %= 60;
    }
    lv_timer_handler();
}

}  // namespace ui
