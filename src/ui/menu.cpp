// Menu glowne konsoli (LVGL, 800x480): pasek z logo i zakladkami + cztery ekrany:
//   Gry       - karuzela okladek (zaznaczona na srodku, sasiednie pomniejszone), tlo = rozmyta okladka
//   Lekcje    - lista lekcji i gier ucznia + podglad (okladka generowana: kolor z nazwy + numer)
//   Ustawienia - jasnosc, wygaszanie, podpowiedzi dotykowe, licznik FPS, kolor akcentu, rekordy, reset
//   O konsoli - wersja, pamiec, czas pracy, test przyciskow
//
// Sterowanie klawiszami jest wlasne (menu::update), nie przez grupe LVGL: karuzela i wiersze ustawien
// potrzebuja LEWO/PRAWO do zmiany wartosci. GORA z pierwszego wiersza (albo X/Y) przechodzi do zakladek.
// Dotyk: stuk w zakladke, w okladke (boczna = wybor, srodkowa = start), przesuniecie palcem po karuzeli,
// stuk w wiersz ustawien (lewa/prawa polowa wartosci = mniej/wiecej).
//
// Polskie litery: czcionki console_font_pl_N (ui/theme.h), w kodzie jako \uXXXX (pliki zrodlowe w ASCII).
#include "ui/menu.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app/settings.h"
#include "core/log.h"
#include "engine/game_registry.h"
#include "engine/rng.h"
#include "engine/screen.h"
#include "engine/storage.h"
#include "gfx/canvas.h"
#include "gfx/png.h"
#include "gfx/text.h"
#include "lvgl.h"
#include "platform/platform.h"
#include "ui/lvgl_glue.h"
#include "ui/theme.h"

namespace ui::menu {

namespace {

const char* TAG = "menu";

constexpr int W = engine::CANVAS_W, H = engine::CANVAS_H;
constexpr int HEADER_H = 64;
constexpr int COVER_W = 440, COVER_H = 248;   // okladka zaznaczona (natywny rozmiar PNG)
constexpr int CAR_CY  = HEADER_H + 20 + COVER_H / 2;   // srodek karuzeli w pionie
constexpr int CAR_STEP = 380;   // odstep srodkow sasiednich okladek: 220 + 0,62*220 = 356 styku, +24 px przerwy
constexpr float SIDE_SCALE = 0.62f;
constexpr int MAX_GAMES = 16, MAX_LESSONS = 40;

enum Tab { TAB_GAMES, TAB_LESSONS, TAB_SETTINGS, TAB_ABOUT, TAB_COUNT };
const char* const TAB_NAMES[TAB_COUNT] = { "Gry", "Lekcje", "Ustawienia", "O konsoli" };

// Obraz RGB565 w PSRAM opisany dla LVGL
struct Pic {
    uint16_t*      px = nullptr;
    lv_image_dsc_t dsc{};
};

// Okladka z kanalem alfa (RGB565A8: plaszczyzna kolorow, za nia plaszczyzna alfy) - zaokraglone rogi "wypalone"
// w alfie skaluja sie razem z obrazem (boczne karty), bez maskowania clip_corner przy kazdym rysowaniu.
constexpr int COVER_RADIUS = 16;

uint16_t* alloc_cover()
{
    const size_t n = (size_t)COVER_W * COVER_H;
    uint16_t* px = platform::alloc_pixels(n + n / 2 + 1, false);
    uint8_t* a = (uint8_t*)(px + n);
    const float r = (float)COVER_RADIUS;
    for (int y = 0; y < COVER_H; ++y) {
        for (int x = 0; x < COVER_W; ++x) {
            // odleglosc od srodka zaokraglenia najblizszego naroznika (0 poza narozami)
            const float cx = x < COVER_RADIUS ? r - 0.5f : (x >= COVER_W - COVER_RADIUS ? (float)(COVER_W - COVER_RADIUS) - 0.5f : (float)x);
            const float cy = y < COVER_RADIUS ? r - 0.5f : (y >= COVER_H - COVER_RADIUS ? (float)(COVER_H - COVER_RADIUS) - 0.5f : (float)y);
            const float dx = (float)x - cx, dy = (float)y - cy;
            const float d = sqrtf(dx * dx + dy * dy);
            const float v = r - d + 0.5f;
            a[y * COVER_W + x] = (uint8_t)(v >= 1.f ? 255 : (v <= 0.f ? 0 : (int)(v * 255.f)));
        }
    }
    return px;
}

void make_dsc(Pic& p, int w, int h, bool alpha = false)
{
    p.dsc = lv_image_dsc_t{};
    p.dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
    p.dsc.header.cf     = LV_COLOR_FORMAT_RGB565;
    p.dsc.header.w      = (uint32_t)w;
    p.dsc.header.h      = (uint32_t)h;
    p.dsc.header.stride = (uint32_t)(w * 2);
    p.dsc.data_size     = (uint32_t)(w * h * 2);
    p.dsc.data          = (const uint8_t*)p.px;
    if (alpha) {
        p.dsc.header.cf = LV_COLOR_FORMAT_RGB565A8;
        p.dsc.data_size = (uint32_t)(w * h * 3);
    }
}

// ---------------------------------------------------------------- stan

struct GameSlot {
    int  index = -1;     // w engine::GAMES
    Pic  cover, bg;      // bg liczone leniwie z okladki (rozmycie + przyciemnienie)
    bool bg_ready = false;
    lv_obj_t* img = nullptr;
};

lv_obj_t* s_root = nullptr;
int       s_selection = -1;   // wynik dla app (take_selection)
Tab       s_tab = TAB_GAMES;
bool      s_tabs_focused = false;
uint16_t  s_prev_mask = 0;
uint32_t  s_last_ms = 0;
bool      s_started = false;

GameSlot  s_games[MAX_GAMES];
int       s_game_n = 0;
int       s_lessons[MAX_LESSONS];
int       s_lesson_n = 0;

// karuzela
int       s_car_sel = 0;
float     s_car_pos = 0;      // animowana pozycja (float), dazy do s_car_sel
lv_obj_t* s_bg_img[2] = {};   // dwa pelnoekranowe tla - przenikanie
int       s_bg_front = 0;
float     s_bg_fade = 1.f;
lv_obj_t* s_game_title = nullptr;
lv_obj_t* s_game_desc  = nullptr;
lv_obj_t* s_dots[MAX_GAMES] = {};

// lekcje
int       s_les_sel = 0;
Pic       s_les_cover;
lv_obj_t* s_les_rows[MAX_LESSONS] = {};
lv_obj_t* s_les_list = nullptr;
lv_obj_t* s_les_img = nullptr;
lv_obj_t* s_les_title = nullptr;
lv_obj_t* s_les_desc = nullptr;

// ustawienia
enum SetRow { SET_BRIGHT, SET_SCREEN_OFF, SET_HINTS, SET_FPS, SET_ACCENT, SET_RECORDS, SET_RESET, SET_COUNT };
int       s_set_sel = 0;
int       s_confirm = -1;     // wiersz czekajacy na potwierdzenie (A drugi raz)
lv_obj_t* s_set_rows[SET_COUNT] = {};
lv_obj_t* s_set_value[SET_COUNT] = {};
lv_obj_t* s_set_bar = nullptr;
lv_obj_t* s_accent_dots[app::settings::ACCENT_COUNT] = {};
lv_obj_t* s_set_note = nullptr;

// o konsoli
lv_obj_t* s_about_uptime = nullptr;
lv_obj_t* s_about_mem = nullptr;
lv_obj_t* s_key_chips[10] = {};
uint32_t  s_about_tick = 0;

// wspolne
lv_obj_t* s_tab_btn[TAB_COUNT] = {};
lv_obj_t* s_pages[TAB_COUNT] = {};
lv_obj_t* s_hint = nullptr;
lv_obj_t* s_frame = nullptr;
lv_obj_t* s_splash = nullptr;

uint32_t accent() { return app::settings::ACCENTS[app::settings::get().accent]; }

// ---------------------------------------------------------------- obrazy

// Okladka zastepcza (gra bez PNG, lekcje): pionowy gradient w kolorze z nazwy + duzy napis.
void draw_generated_cover(uint16_t* px, int w, int h, const char* name, const char* big)
{
    uint32_t hsh = 2166136261u;
    for (const char* p = name; *p; ++p) hsh = (hsh ^ (uint8_t)*p) * 16777619u;
    static const uint32_t BASES[8] = { 0x2563eb, 0x16a34a, 0xdb2777, 0xea580c, 0x7c3aed, 0x0891b2, 0xca8a04, 0xdc2626 };
    const uint32_t base = BASES[hsh % 8];
    const int br = (base >> 16) & 255, bgc = (base >> 8) & 255, bb = base & 255;
    for (int y = 0; y < h; ++y) {
        const float k = 1.0f - 0.55f * (float)y / (float)h;
        const uint16_t c = gfx::rgb565((uint8_t)(br * k), (uint8_t)(bgc * k), (uint8_t)(bb * k));
        for (int x = 0; x < w; ++x) px[y * w + x] = c;
    }
    // ukosne pasy dla faktury
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (((x + y) / 18) % 4 == 0) {
                uint16_t& c = px[y * w + x];
                const int r = (((c >> 11) & 31) * 9) / 8, g = (((c >> 5) & 63) * 9) / 8, b = ((c & 31) * 9) / 8;
                c = (uint16_t)(((r > 31 ? 31 : r) << 11) | ((g > 63 ? 63 : g) << 5) | (b > 31 ? 31 : b));
            }
    gfx::Canvas cv(px, w, h);
    const int bw = gfx::text_width_px(big, 48);
    gfx::draw_text_px(cv, (w - bw) / 2 + 3, h / 2 - 44 + 3, big, gfx::rgb565(10, 10, 20), 48);
    gfx::draw_text_px(cv, (w - bw) / 2, h / 2 - 44, big, gfx::WHITE, 48);
    const int nw = gfx::text_width_px(name, 24);
    gfx::draw_text_px(cv, (w - nw) / 2, h / 2 + 16, name, gfx::rgb565(235, 240, 255), 24);
}

void load_cover(GameSlot& g)
{
    const engine::GameEntry& e = engine::GAMES[g.index];
    g.cover.px = alloc_cover();
    bool ok = false;
    if (e.cover) {
        const gfx::Sprite s = gfx::load_png(e.cover, g.cover.px, (size_t)COVER_W * COVER_H);
        ok = s.px && s.w == COVER_W && s.h == COVER_H;
    }
    if (!ok) {
        char big[4] = { e.name[0], 0 };
        draw_generated_cover(g.cover.px, COVER_W, COVER_H, e.name, big);
    }
    make_dsc(g.cover, COVER_W, COVER_H, true);
}

// Tlo 800x480: okladka zmniejszona do 55x31 (srednia z blokow = rozmycie), powiekszona dwuliniowo, przyciemniona.
void build_bg(GameSlot& g)
{
    if (g.bg_ready) return;
    constexpr int SW = 55, SH = 31;
    static float sm[SH][SW][3];
    const int bx = COVER_W / SW, by = COVER_H / SH;
    for (int y = 0; y < SH; ++y)
        for (int x = 0; x < SW; ++x) {
            float r = 0, gg = 0, b = 0;
            for (int yy = 0; yy < by; ++yy)
                for (int xx = 0; xx < bx; ++xx) {
                    const uint16_t c = g.cover.px[(y * by + yy) * COVER_W + x * bx + xx];
                    r += (float)((c >> 11) & 31); gg += (float)((c >> 5) & 63); b += (float)(c & 31);
                }
            const float n = (float)(bx * by);
            sm[y][x][0] = r / n; sm[y][x][1] = gg / n; sm[y][x][2] = b / n;
        }
    g.bg.px = platform::alloc_pixels((size_t)W * H, false);
    for (int y = 0; y < H; ++y) {
        const float fy = ((float)y + 0.5f) * SH / H - 0.5f;
        const int y0 = fy < 0 ? 0 : (int)fy, y1 = y0 + 1 < SH ? y0 + 1 : SH - 1;
        const float ty = fy < 0 ? 0 : fy - (float)y0;
        // przyciemnienie: mocniej u dolu (tam tekst), lzej u gory
        const float dark = 0.42f - 0.16f * (float)y / (float)H;
        for (int x = 0; x < W; ++x) {
            const float fx = ((float)x + 0.5f) * SW / W - 0.5f;
            const int x0 = fx < 0 ? 0 : (int)fx, x1 = x0 + 1 < SW ? x0 + 1 : SW - 1;
            const float tx = fx < 0 ? 0 : fx - (float)x0;
            float c[3];
            for (int k = 0; k < 3; ++k) {
                const float a = sm[y0][x0][k] + (sm[y0][x1][k] - sm[y0][x0][k]) * tx;
                const float b = sm[y1][x0][k] + (sm[y1][x1][k] - sm[y1][x0][k]) * tx;
                c[k] = (a + (b - a) * ty) * dark;
            }
            g.bg.px[y * W + x] = (uint16_t)(((int)c[0] << 11) | ((int)c[1] << 5) | (int)c[2]);
        }
    }
    make_dsc(g.bg, W, H);
    g.bg_ready = true;
}

// ---------------------------------------------------------------- pomocnicze widgety

lv_obj_t* box(lv_obj_t* parent, int x, int y, int w, int h, uint32_t color, lv_opa_t opa, int radius)
{
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

lv_obj_t* label(lv_obj_t* parent, const char* text, const lv_font_t* font, uint32_t color)
{
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

// ---------------------------------------------------------------- zakladki

void refresh_tabs()
{
    for (int i = 0; i < TAB_COUNT; ++i) {
        lv_obj_t* b = s_tab_btn[i];
        const bool on = i == s_tab;
        lv_obj_set_style_bg_color(b, lv_color_hex(on ? accent() : theme::CARD), 0);
        lv_obj_set_style_bg_opa(b, on ? LV_OPA_COVER : LV_OPA_60, 0);
        lv_obj_set_style_border_width(b, on && s_tabs_focused ? 2 : 0, 0);
        lv_obj_set_style_border_color(b, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_color(lv_obj_get_child(b, 0), lv_color_hex(on ? 0xffffff : theme::TEXT_MUTED), 0);
    }
    for (int i = 0; i < TAB_COUNT; ++i) {
        if (i == s_tab) lv_obj_remove_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (s_frame) {
        if (s_tab == TAB_GAMES) lv_obj_remove_flag(s_frame, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(s_frame, LV_OBJ_FLAG_HIDDEN);
    }
    static const char* const HINTS[TAB_COUNT] = {
        LV_SYMBOL_LEFT " " LV_SYMBOL_RIGHT "  wyb\u00f3r      A  graj      " LV_SYMBOL_UP "  zak\u0142adki",
        LV_SYMBOL_UP " " LV_SYMBOL_DOWN "  wyb\u00f3r      A  uruchom      X / Y  zak\u0142adki",
        LV_SYMBOL_UP " " LV_SYMBOL_DOWN "  wyb\u00f3r      " LV_SYMBOL_LEFT " " LV_SYMBOL_RIGHT "  zmiana      A  zatwierd\u017a",
        "Wci\u015bnij dowolny przycisk - zapali si\u0119 na li\u015bcie      X / Y  zak\u0142adki",
    };
    lv_label_set_text(s_hint, s_tabs_focused ? LV_SYMBOL_LEFT " " LV_SYMBOL_RIGHT "  zak\u0142adka      " LV_SYMBOL_DOWN " / A  wejd\u017a"
                                              : HINTS[s_tab]);
}

void set_tab(int t)
{
    s_tab = (Tab)((t + TAB_COUNT) % TAB_COUNT);
    s_confirm = -1;
    refresh_tabs();
}

void on_tab_click(lv_event_t* e)
{
    s_tabs_focused = false;
    set_tab((int)(intptr_t)lv_event_get_user_data(e));
}

// ---------------------------------------------------------------- karuzela

void show_bg(int slot, bool instant)
{
    GameSlot& g = s_games[slot];
    build_bg(g);
    const int back = 1 - s_bg_front;
    lv_image_set_src(s_bg_img[back], &g.bg.dsc);
    lv_obj_move_to_index(s_bg_img[back], 1);   // nad starym tlem, pod reszta
    s_bg_front = back;
    s_bg_fade = instant ? 1.f : 0.f;
    lv_obj_set_style_opa(s_bg_img[back], instant ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
}

void layout_carousel()
{
    for (int i = 0; i < s_game_n; ++i) {
        const float d = (float)i - s_car_pos, ad = fabsf(d);
        const float near = ad < 1.f ? ad : 1.f, far = ad > 1.f ? ad - 1.f : 0.f;
        const float cx = (float)W / 2 + (d < 0 ? -1.f : 1.f) * (near * CAR_STEP + far * 230.f);
        const float sc = 1.f - (1.f - SIDE_SCALE) * near - 0.15f * (far < 1.f ? far : 1.f);
        lv_obj_t* img = s_games[i].img;
        lv_obj_set_pos(img, (int)(cx - COVER_W / 2), CAR_CY - COVER_H / 2 - HEADER_H);
        lv_image_set_scale(img, (uint32_t)(256.f * sc));
        const float op = ad < 1.f ? 1.f - 0.35f * ad : (ad < 2.f ? 0.65f - 0.65f * (ad - 1.f) : 0.f);
        lv_obj_set_style_opa(img, (lv_opa_t)(255.f * (op < 0 ? 0 : op)), 0);
        if (ad > 2.2f) lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(img, LV_OBJ_FLAG_HIDDEN);
    }
}

void refresh_game_info()
{
    const engine::GameEntry& e = engine::GAMES[s_games[s_car_sel].index];
    lv_label_set_text(s_game_title, e.name);
    lv_label_set_text(s_game_desc, e.description);
    for (int i = 0; i < s_game_n; ++i) {
        lv_obj_set_style_bg_color(s_dots[i], lv_color_hex(i == s_car_sel ? accent() : 0x94a3b8), 0);
        lv_obj_set_width(s_dots[i], i == s_car_sel ? 22 : 8);
    }
}

void select_game(int slot, bool instant = false)
{
    if (slot < 0) slot = 0;
    if (slot >= s_game_n) slot = s_game_n - 1;
    if (slot == s_car_sel && !instant) return;
    s_car_sel = slot;
    if (instant) s_car_pos = (float)slot;
    if (s_games[slot].img) lv_obj_move_foreground(s_games[slot].img);
    show_bg(slot, instant);
    refresh_game_info();
    layout_carousel();
}

void launch(int game_index)
{
    s_selection = game_index;
    app::settings::get().last_game = (uint8_t)game_index;
    app::settings::save();
}

void on_cover_click(lv_event_t* e)
{
    const int slot = (int)(intptr_t)lv_event_get_user_data(e);
    s_tabs_focused = false;
    if (slot == s_car_sel && fabsf(s_car_pos - (float)slot) < 0.3f) launch(s_games[slot].index);
    else select_game(slot);
}

void on_carousel_gesture(lv_event_t*)
{
    const lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if (dir == LV_DIR_LEFT) select_game(s_car_sel + 1);
    else if (dir == LV_DIR_RIGHT) select_game(s_car_sel - 1);
}

void build_games_page(lv_obj_t* page)
{
    lv_obj_add_event_cb(page, on_carousel_gesture, LV_EVENT_GESTURE, nullptr);
    lv_obj_add_flag(page, LV_OBJ_FLAG_CLICKABLE);
    for (int i = 0; i < s_game_n; ++i) {
        lv_obj_t* img = lv_image_create(page);
        lv_image_set_src(img, &s_games[i].cover.dsc);
        lv_image_set_pivot(img, COVER_W / 2, COVER_H / 2);
        lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(img, LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_event_cb(img, on_cover_click, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        s_games[i].img = img;
    }
    s_game_title = label(page, "", theme::FONT_TITLE, theme::TEXT);
    lv_obj_set_width(s_game_title, W);
    lv_obj_set_style_text_align(s_game_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(s_game_title, 0, CAR_CY + COVER_H / 2 + 14 - HEADER_H);
    s_game_desc = label(page, "", theme::FONT_BODY, 0xcbd5e1);
    lv_obj_set_width(s_game_desc, W);
    lv_obj_set_style_text_align(s_game_desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(s_game_desc, 0, CAR_CY + COVER_H / 2 + 56 - HEADER_H);
    // kropki pozycji
    lv_obj_t* dots = lv_obj_create(page);
    lv_obj_remove_style_all(dots);
    lv_obj_set_size(dots, W, 10);
    lv_obj_set_pos(dots, 0, CAR_CY + COVER_H / 2 + 88 - HEADER_H);
    lv_obj_set_flex_flow(dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dots, 6, 0);
    for (int i = 0; i < s_game_n; ++i) {
        s_dots[i] = box(dots, 0, 0, 8, 8, 0x94a3b8, LV_OPA_COVER, 4);
    }
}

// ---------------------------------------------------------------- lekcje

// Numer lekcji z opisu "Lekcja 07: ..." (tak opisane sa lekcje w CONSOLE_ADD_GAME); inne pozycje - pusty tekst.
void lesson_number(const engine::GameEntry& e, char* out, size_t n)
{
    out[0] = 0;
    const char* d = e.description;
    if (d && strncmp(d, "Lekcja ", 7) == 0 && d[7] >= '0' && d[7] <= '9') {
        int k = 0;
        for (const char* p = d + 7; *p >= '0' && *p <= '9' && k + 1 < (int)n; ++p) out[k++] = *p;
        out[k] = 0;
    }
}

void refresh_lesson()
{
    if (s_lesson_n == 0) return;
    const engine::GameEntry& e = engine::GAMES[s_lessons[s_les_sel]];
    char big[8];
    lesson_number(e, big, sizeof(big));
    if (!big[0]) { big[0] = e.name[0]; big[1] = 0; }
    draw_generated_cover(s_les_cover.px, COVER_W, COVER_H, e.name, big);
    // te same piksele pod tym samym adresem - bez cache obrazow (LV_CACHE_DEF_SIZE 0) wystarczy odrysowac
    lv_image_set_src(s_les_img, &s_les_cover.dsc);
    lv_obj_invalidate(s_les_img);
    lv_label_set_text(s_les_title, e.name);
    lv_label_set_text(s_les_desc, e.description);
    for (int i = 0; i < s_lesson_n; ++i) {
        const bool on = i == s_les_sel;
        lv_obj_set_style_bg_color(s_les_rows[i], lv_color_hex(on ? accent() : theme::CARD), 0);
        lv_obj_set_style_bg_opa(s_les_rows[i], on ? LV_OPA_COVER : LV_OPA_70, 0);
    }
    lv_obj_scroll_to_view(s_les_rows[s_les_sel], LV_ANIM_ON);
}

void on_lesson_click(lv_event_t* e)
{
    const int i = (int)(intptr_t)lv_event_get_user_data(e);
    s_tabs_focused = false;
    if (i == s_les_sel) launch(s_lessons[i]);
    else { s_les_sel = i; refresh_lesson(); }
}

void on_lesson_preview_click(lv_event_t*)
{
    if (s_lesson_n) launch(s_lessons[s_les_sel]);
}

void build_lessons_page(lv_obj_t* page)
{
    s_les_list = lv_obj_create(page);
    lv_obj_remove_style_all(s_les_list);
    lv_obj_set_pos(s_les_list, 24, 16);
    lv_obj_set_size(s_les_list, 300, H - HEADER_H - 60);
    lv_obj_set_flex_flow(s_les_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_les_list, 6, 0);
    lv_obj_set_scrollbar_mode(s_les_list, LV_SCROLLBAR_MODE_OFF);
    for (int i = 0; i < s_lesson_n; ++i) {
        lv_obj_t* row = box(s_les_list, 0, 0, 292, 42, theme::CARD, LV_OPA_70, 10);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, on_lesson_click, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        char num[8];
        lesson_number(engine::GAMES[s_lessons[i]], num, sizeof(num));
        lv_obj_t* n = label(row, num[0] ? num : LV_SYMBOL_FILE, theme::FONT_BODY, 0xcbd5e1);
        lv_obj_align(n, LV_ALIGN_LEFT_MID, 12, 0);
        lv_obj_t* t = label(row, engine::GAMES[s_lessons[i]].name, theme::FONT_BUTTON, theme::TEXT);
        lv_obj_align(t, LV_ALIGN_LEFT_MID, 48, 0);
        s_les_rows[i] = row;
    }
    s_les_cover.px = alloc_cover();
    make_dsc(s_les_cover, COVER_W, COVER_H, true);
    s_les_img = lv_image_create(page);
    lv_obj_set_pos(s_les_img, 340, 16);
    lv_obj_add_flag(s_les_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_les_img, on_lesson_preview_click, LV_EVENT_CLICKED, nullptr);
    s_les_title = label(page, "", theme::FONT_CARD_TITLE, theme::TEXT);
    lv_obj_set_pos(s_les_title, 344, 16 + COVER_H + 14);
    s_les_desc = label(page, "", theme::FONT_BODY, 0xcbd5e1);
    lv_obj_set_width(s_les_desc, COVER_W);
    lv_obj_set_pos(s_les_desc, 344, 16 + COVER_H + 50);
    if (s_lesson_n == 0) lv_label_set_text(s_les_title, "Brak lekcji");
}

// ---------------------------------------------------------------- ustawienia

const char* const SET_LABELS[SET_COUNT] = {
    "Jasno\u015b\u0107 ekranu", "Wygaszanie ekranu", "Podpowiedzi dotykowe w grach", "Licznik FPS w grach",
    "Kolor akcentu", "Rekordy gier", "Ustawienia domy\u015blne",
};

void refresh_settings()
{
    using namespace app::settings;
    Values& v = get();
    char buf[48];
    for (int i = 0; i < SET_COUNT; ++i) {
        const bool on = i == s_set_sel && !s_tabs_focused;
        lv_obj_set_style_bg_color(s_set_rows[i], lv_color_hex(on ? theme::CARD_FOCUS : theme::CARD), 0);
        lv_obj_set_style_border_width(s_set_rows[i], on ? 2 : 0, 0);
        lv_obj_set_style_border_color(s_set_rows[i], lv_color_hex(accent()), 0);
    }
    snprintf(buf, sizeof(buf), "%d %%", v.brightness);
    lv_label_set_text(s_set_value[SET_BRIGHT], buf);
    lv_bar_set_value(s_set_bar, v.brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_set_bar, lv_color_hex(accent()), LV_PART_INDICATOR);
    if (SCREEN_OFF_MIN[v.screen_off] == 0) snprintf(buf, sizeof(buf), LV_SYMBOL_LEFT "  nigdy  " LV_SYMBOL_RIGHT);
    else snprintf(buf, sizeof(buf), LV_SYMBOL_LEFT "  po %d min  " LV_SYMBOL_RIGHT, SCREEN_OFF_MIN[v.screen_off]);
    lv_label_set_text(s_set_value[SET_SCREEN_OFF], buf);
    static const char* const HINTS[3] = { "automatycznie", "zawsze", "nigdy" };
    snprintf(buf, sizeof(buf), LV_SYMBOL_LEFT "  %s  " LV_SYMBOL_RIGHT, HINTS[v.touch_hints]);
    lv_label_set_text(s_set_value[SET_HINTS], buf);
    lv_label_set_text(s_set_value[SET_FPS], v.show_fps ? "w\u0142\u0105czony" : "wy\u0142\u0105czony");
    lv_obj_set_style_text_color(s_set_value[SET_FPS], lv_color_hex(v.show_fps ? accent() : theme::TEXT_MUTED), 0);
    for (int i = 0; i < ACCENT_COUNT; ++i) {
        lv_obj_set_style_border_width(s_accent_dots[i], i == v.accent ? 3 : 0, 0);
    }
    lv_label_set_text(s_set_value[SET_RECORDS], s_confirm == SET_RECORDS ? "A - na pewno?" : "wyczy\u015b\u0107");
    lv_label_set_text(s_set_value[SET_RESET], s_confirm == SET_RESET ? "A - na pewno?" : "przywr\u00f3\u0107");
    lv_obj_set_style_text_color(s_set_value[SET_RECORDS], lv_color_hex(s_confirm == SET_RECORDS ? theme::DANGER : theme::TEXT_MUTED), 0);
    lv_obj_set_style_text_color(s_set_value[SET_RESET], lv_color_hex(s_confirm == SET_RESET ? theme::DANGER : theme::TEXT_MUTED), 0);
}

void change_setting(int row, int delta, bool activate)
{
    using namespace app::settings;
    Values& v = get();
    bool changed = true;
    switch (row) {
    case SET_BRIGHT: {
        int b = v.brightness + (delta ? delta * 10 : 0);
        v.brightness = (uint8_t)(b < 10 ? 10 : (b > 100 ? 100 : b));
        apply();
        break;
    }
    case SET_SCREEN_OFF: v.screen_off = (uint8_t)((v.screen_off + SCREEN_OFF_COUNT + (delta ? delta : 1)) % SCREEN_OFF_COUNT); break;
    case SET_HINTS:      v.touch_hints = (uint8_t)((v.touch_hints + 3 + (delta ? delta : 1)) % 3); break;
    case SET_FPS:        v.show_fps ^= 1; break;
    case SET_ACCENT:     v.accent = (uint8_t)((v.accent + ACCENT_COUNT + (delta ? delta : 1)) % ACCENT_COUNT); break;
    case SET_RECORDS:
    case SET_RESET:
        changed = false;
        if (!activate) break;
        if (s_confirm != row) { s_confirm = row; break; }
        s_confirm = -1;
        if (row == SET_RECORDS) {
            for (int i = 0; i < engine::RECORD_KEY_COUNT; ++i) engine::erase_data(engine::RECORD_KEYS[i]);
            lv_label_set_text(s_set_note, "Rekordy wyczyszczone.");
        } else {
            reset();
            lv_label_set_text(s_set_note, "Przywr\u00f3cono ustawienia domy\u015blne.");
        }
        refresh_tabs();
        break;
    default: changed = false; break;
    }
    if (changed) {
        s_confirm = -1;
        save();
        if (row == SET_ACCENT) { refresh_tabs(); refresh_game_info(); }
    }
    refresh_settings();
}

void on_setting_click(lv_event_t* e)
{
    const int row = (int)(intptr_t)lv_event_get_user_data(e);
    s_tabs_focused = false;
    s_set_sel = row;
    lv_point_t p;
    lv_indev_get_point(lv_indev_active(), &p);
    const int delta = p.x < 560 ? -1 : 1;   // lewa czesc wartosci = mniej, prawa = wiecej
    change_setting(row, row == SET_FPS || row >= SET_RECORDS ? 0 : delta, true);
}

void build_settings_page(lv_obj_t* page)
{
    for (int i = 0; i < SET_COUNT; ++i) {
        lv_obj_t* row = box(page, 24, 10 + i * 50, W - 48, 44, theme::CARD, LV_OPA_80, 10);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, on_setting_click, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_t* l = label(row, SET_LABELS[i], theme::FONT_BUTTON, theme::TEXT);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 18, 0);
        s_set_rows[i] = row;
        if (i == SET_ACCENT) {
            for (int k = 0; k < app::settings::ACCENT_COUNT; ++k) {
                lv_obj_t* d = box(row, W - 48 - 40 - (app::settings::ACCENT_COUNT - 1 - k) * 36 - 12, 8, 28, 28,
                                  app::settings::ACCENTS[k], LV_OPA_COVER, 14);
                lv_obj_set_style_border_color(d, lv_color_hex(0xffffff), 0);
                lv_obj_remove_flag(d, LV_OBJ_FLAG_CLICKABLE);
                s_accent_dots[k] = d;
            }
            continue;
        }
        lv_obj_t* v = label(row, "", theme::FONT_BODY, theme::TEXT_MUTED);
        lv_obj_align(v, LV_ALIGN_RIGHT_MID, -18, 0);
        s_set_value[i] = v;
        if (i == SET_BRIGHT) {
            s_set_bar = lv_bar_create(row);
            lv_bar_set_range(s_set_bar, 0, 100);
            lv_obj_set_size(s_set_bar, 220, 10);
            lv_obj_align(s_set_bar, LV_ALIGN_RIGHT_MID, -90, 0);
            lv_obj_set_style_bg_color(s_set_bar, lv_color_hex(0x0b1220), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(s_set_bar, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_remove_flag(s_set_bar, LV_OBJ_FLAG_CLICKABLE);
        }
    }
    s_set_note = label(page, "", theme::FONT_SMALL, theme::TEXT_MUTED);
    lv_obj_set_pos(s_set_note, 28, 10 + SET_COUNT * 50 + 2);
}

// ---------------------------------------------------------------- o konsoli

const char* const KEY_NAMES[10] = { LV_SYMBOL_UP, LV_SYMBOL_DOWN, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "A", "B", "X", "Y", "START", "SELECT" };

void refresh_about(uint16_t mask)
{
    for (int i = 0; i < 10; ++i) {
        const bool on = (mask >> i) & 1;
        lv_obj_set_style_bg_color(s_key_chips[i], lv_color_hex(on ? accent() : theme::CARD), 0);
    }
    const uint32_t now = lv_tick_get();   // w testach zegar wirtualny (ui/lvgl_glue.cpp) - zrzut powtarzalny
    if (now - s_about_tick < 500 && s_about_tick) return;
    s_about_tick = now;
    char buf[96];
    const uint32_t s = now / 1000;
    snprintf(buf, sizeof(buf), "%u:%02u:%02u", (unsigned)(s / 3600), (unsigned)(s / 60 % 60), (unsigned)(s % 60));
    lv_label_set_text(s_about_uptime, buf);
    const platform::MemInfo m = platform::memory_info();
    if (m.psram_total == 0) snprintf(buf, sizeof(buf), "emulator (Windows)");
    else snprintf(buf, sizeof(buf), "SRAM %u / %u kB    PSRAM %u / %u MB", (unsigned)(m.sram_free / 1024), (unsigned)(m.sram_total / 1024),
                  (unsigned)(m.psram_free >> 20), (unsigned)(m.psram_total >> 20));
    lv_label_set_text(s_about_mem, buf);
}

void build_about_page(lv_obj_t* page)
{
    lv_obj_t* panel = box(page, 24, 10, W - 48, 250, theme::CARD, LV_OPA_80, 16);
    lv_obj_t* logo = label(panel, theme::BRAND_NAME, &console_font_pl_40, theme::TEXT);
    lv_obj_set_style_text_letter_space(logo, 4, 0);
    lv_obj_set_pos(logo, 24, 16);
    lv_obj_t* sub = label(panel, theme::BRAND_SUB, theme::FONT_BUTTON, accent());
    lv_obj_set_style_text_letter_space(sub, 3, 0);
    lv_obj_set_pos(sub, 26, 66);
    char buf[128];
    int games = 0, lessons = 0;
    for (int i = 0; i < engine::GAME_COUNT; ++i) (engine::GAMES[i].lesson ? lessons : games)++;
    struct Row { const char* k; const char* v; };
    static char v_games[32], v_build[48];
    snprintf(v_games, sizeof(v_games), "%d gier, %d lekcji", games, lessons);
    if (engine::deterministic()) snprintf(v_build, sizeof(v_build), "(test)");   // zrzut niezalezny od daty kompilacji
#ifdef CONSOLE_VERSION
    else snprintf(v_build, sizeof(v_build), "%s  (%s)", CONSOLE_VERSION, __DATE__);   // emulator: wersja wydania
#else
    else snprintf(v_build, sizeof(v_build), "%s  %s", __DATE__, __TIME__);
#endif
    const Row rows[] = { { "Uk\u0142ad", "ESP32-P4, 2 x RISC-V 360 MHz" }, { "Ekran", "4,3\"  800 x 480, dotyk GT911" },
                         { "Zawarto\u015b\u0107", v_games }, { "Kompilacja", v_build } };
    for (int i = 0; i < 4; ++i) {
        lv_obj_t* k = label(panel, rows[i].k, theme::FONT_BODY, theme::TEXT_MUTED);
        lv_obj_set_pos(k, 26, 106 + i * 26);
        lv_obj_t* v = label(panel, rows[i].v, theme::FONT_BODY, theme::TEXT);
        lv_obj_set_pos(v, 150, 106 + i * 26);
    }
    lv_obj_t* k1 = label(panel, "Czas pracy", theme::FONT_BODY, theme::TEXT_MUTED);
    lv_obj_set_pos(k1, 470, 106);
    s_about_uptime = label(panel, "", theme::FONT_BODY, theme::TEXT);
    lv_obj_set_pos(s_about_uptime, 590, 106);
    lv_obj_t* k2 = label(panel, "Wolna pami\u0119\u0107", theme::FONT_BODY, theme::TEXT_MUTED);
    lv_obj_set_pos(k2, 470, 132);
    s_about_mem = label(panel, "", theme::FONT_SMALL, theme::TEXT);
    lv_obj_set_width(s_about_mem, 250);
    lv_obj_set_pos(s_about_mem, 470, 156);
    (void)buf;

    lv_obj_t* test = box(page, 24, 272, W - 48, 100, theme::CARD, LV_OPA_80, 16);
    lv_obj_t* tl = label(test, "Test przycisk\u00f3w", theme::FONT_BODY, theme::TEXT_MUTED);
    lv_obj_set_pos(tl, 20, 12);
    int x = 20;
    for (int i = 0; i < 10; ++i) {
        const int w = i >= 8 ? 92 : 56;
        lv_obj_t* chip = box(test, x, 44, w, 40, theme::CARD, LV_OPA_COVER, 10);
        lv_obj_set_style_border_width(chip, 1, 0);
        lv_obj_set_style_border_color(chip, lv_color_hex(theme::LINE), 0);
        lv_obj_t* l = label(chip, KEY_NAMES[i], theme::FONT_BUTTON, theme::TEXT);
        lv_obj_center(l);
        s_key_chips[i] = chip;
        x += w + 8;
    }
}

// ---------------------------------------------------------------- ekran startowy

void splash_done(lv_anim_t* a)
{
    lv_obj_t* o = (lv_obj_t*)a->var;
    lv_obj_delete(o);
    s_splash = nullptr;
}

void splash_opa(void* obj, int32_t v) { lv_obj_set_style_opa((lv_obj_t*)obj, (lv_opa_t)v, 0); }

void show_splash(lv_obj_t* scr)
{
    s_splash = box(scr, 0, 0, W, H, 0x020617, LV_OPA_COVER, 0);
    lv_obj_t* l = label(s_splash, theme::BRAND_NAME, &console_font_pl_48, theme::TEXT);
    lv_obj_set_style_text_letter_space(l, 8, 0);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, -18);
    lv_obj_t* s = label(s_splash, theme::BRAND_SUB, theme::FONT_BUTTON, accent());
    lv_obj_set_style_text_letter_space(s, 6, 0);
    lv_obj_align(s, LV_ALIGN_CENTER, 0, 30);
    lv_obj_t* line = box(s_splash, W / 2 - 60, H / 2 + 62, 120, 3, accent(), LV_OPA_COVER, 2);
    (void)line;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_splash);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_duration(&a, 450);
    lv_anim_set_delay(&a, 1100);
    lv_anim_set_exec_cb(&a, splash_opa);
    lv_anim_set_completed_cb(&a, splash_done);
    lv_anim_start(&a);
}

}  // namespace

// ============================================================================ API

void show()
{
    if (s_root) return;
    s_selection = -1;
    s_tabs_focused = false;
    s_confirm = -1;

    // lista gier i lekcji (okladki gier wczytywane raz, zyja do konca programu)
    if (!s_started) {
        s_game_n = 0;
        s_lesson_n = 0;
        for (int i = 0; i < engine::GAME_COUNT; ++i) {
            if (!engine::GAMES[i].lesson && s_game_n < MAX_GAMES) {
                s_games[s_game_n].index = i;
                load_cover(s_games[s_game_n]);
                ++s_game_n;
            } else if (engine::GAMES[i].lesson && s_lesson_n < MAX_LESSONS) {
                s_lessons[s_lesson_n++] = i;
            }
        }
    }

    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x020617), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    s_root = box(scr, 0, 0, W, H, 0x020617, LV_OPA_COVER, 0);

    // tla (dwa, do przenikania) + ciemny gradient dla czytelnosci
    for (int i = 0; i < 2; ++i) {
        s_bg_img[i] = lv_image_create(s_root);
        lv_obj_set_pos(s_bg_img[i], 0, 0);
        lv_obj_remove_flag(s_bg_img[i], LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_t* shade = box(s_root, 0, 0, W, H, 0x020617, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_main_opa(shade, LV_OPA_40, 0);
    lv_obj_set_style_bg_grad_color(shade, lv_color_hex(0x020617), 0);
    lv_obj_set_style_bg_grad_opa(shade, LV_OPA_10, 0);
    lv_obj_set_style_bg_grad_dir(shade, LV_GRAD_DIR_VER, 0);
    lv_obj_remove_flag(shade, LV_OBJ_FLAG_CLICKABLE);

    // naglowek: logo + zakladki
    lv_obj_t* head = box(s_root, 0, 0, W, HEADER_H, 0x020617, LV_OPA_50, 0);
    lv_obj_t* logo = label(head, theme::BRAND_NAME, theme::FONT_CARD_TITLE, theme::TEXT);
    lv_obj_set_style_text_letter_space(logo, 3, 0);
    lv_obj_set_pos(logo, 24, 9);
    lv_obj_t* sub = label(head, theme::BRAND_SUB, &console_font_pl_12, accent());
    lv_obj_set_style_text_letter_space(sub, 3, 0);
    lv_obj_set_pos(sub, 25, 38);
    int tx = W - 20;
    for (int i = TAB_COUNT - 1; i >= 0; --i) {
        const int tw = gfx::text_width_px(TAB_NAMES[i], 16) + 28;
        tx -= tw;
        lv_obj_t* b = box(head, tx, 14, tw, 36, theme::CARD, LV_OPA_60, 18);
        lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(b, on_tab_click, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_t* l = label(b, TAB_NAMES[i], theme::FONT_BODY, theme::TEXT_MUTED);
        lv_obj_center(l);
        s_tab_btn[i] = b;
        tx -= 8;
    }

    // strony
    for (int i = 0; i < TAB_COUNT; ++i) {
        s_pages[i] = box(s_root, 0, HEADER_H, W, H - HEADER_H, 0, LV_OPA_TRANSP, 0);
    }
    build_games_page(s_pages[TAB_GAMES]);
    build_lessons_page(s_pages[TAB_LESSONS]);
    build_settings_page(s_pages[TAB_SETTINGS]);
    build_about_page(s_pages[TAB_ABOUT]);

    // ramka zaznaczenia na srodku karuzeli (stoi w miejscu, okladki przesuwaja sie pod nia)
    s_frame = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_frame);
    lv_obj_set_size(s_frame, COVER_W + 10, COVER_H + 10);
    lv_obj_set_pos(s_frame, W / 2 - COVER_W / 2 - 5, CAR_CY - COVER_H / 2 - 5);
    lv_obj_set_style_radius(s_frame, 20, 0);
    lv_obj_set_style_border_width(s_frame, 3, 0);
    lv_obj_set_style_border_color(s_frame, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_opa(s_frame, LV_OPA_80, 0);
    lv_obj_remove_flag(s_frame, LV_OBJ_FLAG_CLICKABLE);

    s_hint = label(s_root, "", theme::FONT_SMALL, 0x94a3b8);
    lv_obj_set_width(s_hint, W);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(s_hint, 0, H - 24);

    // stan poczatkowy: ostatnio uruchomiona gra (albo lekcja)
    const int last = app::settings::get().last_game;
    s_tab = TAB_GAMES;
    int start_slot = 0;
    for (int i = 0; i < s_game_n; ++i) if (s_games[i].index == last) start_slot = i;
    for (int i = 0; i < s_lesson_n; ++i) if (s_lessons[i] == last) { s_les_sel = i; s_tab = TAB_LESSONS; }
    s_car_sel = -1;
    select_game(start_slot, true);
    refresh_lesson();
    refresh_settings();
    refresh_tabs();
    refresh_about(0);

    if (!s_started && !engine::deterministic()) show_splash(scr);
    s_started = true;
    s_last_ms = platform::millis();
    s_prev_mask = 0xFFFF;   // klawisz trzymany przy wejsciu (np. A z pauzy) nie dziala od razu
    CONSOLE_LOGI(TAG, "menu: %d gier, %d lekcji", s_game_n, s_lesson_n);
}

void hide()
{
    if (!s_root) return;
    lv_group_remove_all_objs(ui::nav_group());
    lv_obj_delete(s_root);
    if (s_splash) { lv_obj_delete(s_splash); s_splash = nullptr; }
    s_root = nullptr;
    s_selection = -1;
    for (int i = 0; i < s_game_n; ++i) s_games[i].img = nullptr;
}

void debug_line(char* buf, int n)
{
    snprintf(buf, (size_t)n, "MENU tab=%d tabs_focus=%d game=%d lesson=%d setting=%d confirm=%d", (int)s_tab, s_tabs_focused ? 1 : 0,
             s_car_sel, s_les_sel, s_set_sel, s_confirm);
}

int take_selection()
{
    const int sel = s_selection;
    s_selection = -1;
    return sel;
}

void update(const input::PadState& pad)
{
    if (!s_root) return;
    uint16_t mask = 0;
    const bool held[10] = { pad.up, pad.down, pad.left, pad.right, pad.a, pad.b, pad.x, pad.y, pad.start, pad.select };
    for (int i = 0; i < 10; ++i) if (held[i]) mask |= (uint16_t)(1u << i);
    if (s_prev_mask == 0xFFFF) s_prev_mask = mask;
    const uint16_t edge = mask & ~s_prev_mask;
    s_prev_mask = mask;
    auto e = [&](int bit) { return (edge >> bit) & 1; };
    enum { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_A, K_B, K_X, K_Y, K_START, K_SELECT };

    if (s_splash && edge) {   // dowolny klawisz pomija ekran startowy
        lv_anim_delete(s_splash, splash_opa);
        lv_obj_delete(s_splash);
        s_splash = nullptr;
        return;
    }

    if (e(K_X)) { s_tabs_focused = false; set_tab(s_tab + 1); }
    if (e(K_Y)) { s_tabs_focused = false; set_tab(s_tab - 1); }

    if (s_tabs_focused) {
        if (e(K_LEFT)) set_tab(s_tab - 1);
        if (e(K_RIGHT)) set_tab(s_tab + 1);
        if (e(K_DOWN) || e(K_A) || e(K_B)) { s_tabs_focused = false; refresh_tabs(); refresh_settings(); }
    } else {
        switch (s_tab) {
        case TAB_GAMES:
            if (e(K_LEFT)) select_game(s_car_sel - 1);
            if (e(K_RIGHT)) select_game(s_car_sel + 1);
            if (e(K_A) || e(K_START)) launch(s_games[s_car_sel].index);
            if (e(K_UP)) { s_tabs_focused = true; refresh_tabs(); }
            break;
        case TAB_LESSONS:
            if (e(K_DOWN) && s_les_sel + 1 < s_lesson_n) { ++s_les_sel; refresh_lesson(); }
            if (e(K_UP)) {
                if (s_les_sel > 0) { --s_les_sel; refresh_lesson(); }
                else { s_tabs_focused = true; refresh_tabs(); }
            }
            if ((e(K_A) || e(K_START)) && s_lesson_n) launch(s_lessons[s_les_sel]);
            if (e(K_B)) set_tab(TAB_GAMES);
            break;
        case TAB_SETTINGS:
            if (e(K_DOWN) && s_set_sel + 1 < SET_COUNT) { ++s_set_sel; s_confirm = -1; refresh_settings(); }
            if (e(K_UP)) {
                if (s_set_sel > 0) { --s_set_sel; s_confirm = -1; refresh_settings(); }
                else { s_tabs_focused = true; refresh_tabs(); refresh_settings(); }
            }
            if (e(K_LEFT) && s_set_sel != SET_RECORDS && s_set_sel != SET_RESET) change_setting(s_set_sel, -1, false);
            if (e(K_RIGHT) && s_set_sel != SET_RECORDS && s_set_sel != SET_RESET) change_setting(s_set_sel, 1, false);
            if (e(K_A)) change_setting(s_set_sel, 0, true);
            if (e(K_B)) { if (s_confirm >= 0) { s_confirm = -1; refresh_settings(); } else set_tab(TAB_GAMES); }
            break;
        case TAB_ABOUT:
            if (e(K_UP) || e(K_B)) { s_tabs_focused = true; refresh_tabs(); }
            break;
        default: break;
        }
    }
    if (s_tab == TAB_ABOUT) refresh_about(mask);

    // animacja karuzeli i przenikanie tla
    const uint32_t now = platform::millis();
    float dt = (float)(now - s_last_ms) / 1000.f;
    s_last_ms = now;
    if (engine::deterministic() || dt > 0.1f) dt = 1.f / 60.f;
    const float target = (float)s_car_sel;
    if (fabsf(s_car_pos - target) > 0.001f) {
        s_car_pos += (target - s_car_pos) * (dt * 12.f > 1.f ? 1.f : dt * 12.f);
        if (fabsf(s_car_pos - target) < 0.004f) s_car_pos = target;
        layout_carousel();
    }
    if (s_bg_fade < 1.f) {
        s_bg_fade += dt * 4.f;
        if (s_bg_fade > 1.f) s_bg_fade = 1.f;
        lv_obj_set_style_opa(s_bg_img[s_bg_front], (lv_opa_t)(255.f * s_bg_fade), 0);
    }
}

}  // namespace ui::menu
