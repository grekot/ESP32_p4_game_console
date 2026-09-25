#include "app/app.h"
#include "app/settings.h"

#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "engine/game.h"
#include "engine/game_registry.h"
#include "engine/rng.h"
#include "engine/screen.h"
#include "engine/stats.h"
#include "gfx/canvas.h"
#include "gfx/text.h"
#include "input/virtual_pad.h"
#include "platform/platform.h"
#include "ui/lvgl_glue.h"
#include "ui/menu.h"
#include "ui/pause.h"

namespace app {

namespace {

const char* TAG = "app";

enum class State { Menu, Playing, Paused };

uint16_t*         s_pixels = nullptr;   // plotno konsoli 800x480 (to jest wyswietlane)
uint16_t*         s_frozen = nullptr;   // kopia klatki gry pokazywana pod UI pauzy
gfx::Canvas*      s_canvas = nullptr;
// Pod-plotno 400x240 dla gier pixel-art (Game::canvas_scale() == 2): gra rysuje tu, konsola powieksza x2.
uint16_t*         s_small_pixels = nullptr;
gfx::Canvas*      s_small_canvas = nullptr;
int               s_scale        = 1;
input::VirtualPad s_pad;

State          s_state     = State::Menu;
engine::Game*  s_game      = nullptr;
const char*    s_game_name = "";
int64_t        s_last_us   = 0;
float          s_fixed_dt  = 0.f;   // >0: staly krok (testy skryptowane w emulatorze)
int64_t        s_idle_since_us = 0;  // ostatnie wejscie (klawisz/dotyk) - wygaszanie ekranu
bool           s_screen_off    = false;

// Wygaszanie ekranu w menu i pauzie: po N minutach bez wejscia podswietlenie gasnie; pierwsze wejscie tylko
// je zapala (nie dziala jako klikniecie). Zwraca true, gdy wejscie trzeba w tej klatce pominac.
bool screen_sleep(int64_t now, bool input)
{
    if (input) s_idle_since_us = now;
    if (s_screen_off) {
        if (!input) return true;
        s_screen_off = false;
        app::settings::apply();
        return true;
    }
    const int minutes = app::settings::SCREEN_OFF_MIN[app::settings::get().screen_off];
    if (minutes > 0 && now - s_idle_since_us > (int64_t)minutes * 60 * 1000000) {
        s_screen_off = true;
        platform::set_brightness(0);
        return true;
    }
    return false;
}

bool any_input(const input::PadState& k)
{
    input::TouchPoint pts[1];
    return k.any_held() || platform::read_touch(pts, 1) > 0;
}

// Licznik FPS (ustawienie "Licznik FPS w grach"): maly panel w prawym dolnym rogu (u gory gry maja HUD).
void draw_fps(gfx::Canvas& c)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d FPS", (int)(engine::current_fps() + 0.5f));
    const int w = gfx::text_width_px(buf, 14) + 12;
    c.fill_rect(engine::CANVAS_W - w - 4, engine::CANVAS_H - 24, w, 20, gfx::rgb565(0, 0, 0));
    gfx::draw_text_px(c, engine::CANVAS_W - w + 2, engine::CANVAS_H - 22, buf, gfx::rgb565(120, 255, 140), 14);
}

void start_game(int index)
{
    if (index < 0 || index >= engine::GAME_COUNT) return;

    ui::menu::hide();
    // Losowosc: w trybie skryptowanym (staly dt) zawsze to samo ziarno, zeby testy byly powtarzalne;
    // w normalnej grze ziarno z zegara, zeby kazda rozgrywka byla inna.
    engine::seed_rng(s_fixed_dt > 0.f ? 0x4C414B45u : (uint32_t)platform::micros());
    s_game      = engine::GAMES[index].create();
    s_game_name = engine::GAMES[index].name;
    s_scale     = s_game->canvas_scale() == 2 ? 2 : 1;
    if (s_scale == 2 && !s_small_canvas) {
        // 192 kB - probujemy szybkiej pamieci wewnetrznej, gra pixel-art rysuje tu co klatke.
        s_small_pixels = platform::alloc_pixels((size_t)engine::PIXEL_CANVAS_W * engine::PIXEL_CANVAS_H, /*fast=*/true);
        if (!s_small_pixels) {
            CONSOLE_LOGE(TAG, "brak pamieci na plotno pixel-art - gra dostaje pelne plotno");
            s_scale = 1;
        } else {
            static gfx::Canvas small(s_small_pixels, engine::PIXEL_CANVAS_W, engine::PIXEL_CANVAS_H);
            s_small_canvas = &small;
            s_small_canvas->clear(gfx::BLACK);
        }
    }
    s_game->init(s_scale == 2 ? *s_small_canvas : *s_canvas);
    s_state = State::Playing;
    CONSOLE_LOGI(TAG, "start gry: %s (plotno %s)", s_game_name, s_scale == 2 ? "400x240 x2" : "800x480");
}

void back_to_menu()
{
    ui::pause::hide();
    ui::set_background_frame(nullptr);
    ui::menu::show();
    s_game  = nullptr;
    s_state = State::Menu;
    s_idle_since_us = platform::micros();
    CONSOLE_LOGI(TAG, "powrot do menu");
}

}  // namespace

bool init()
{
    // 768 kB - w PSRAM (SRAM ma ~300 kB); dla gier pixel-art jest osobne male plotno w SRAM (start_game).
    s_pixels = platform::alloc_pixels((size_t)engine::CANVAS_W * engine::CANVAS_H, /*fast=*/false);
    if (!s_pixels) {
        CONSOLE_LOGE(TAG, "brak pamieci na plotno %dx%d", engine::CANVAS_W, engine::CANVAS_H);
        return false;
    }
    static gfx::Canvas canvas(s_pixels, engine::CANVAS_W, engine::CANVAS_H);
    s_canvas = &canvas;
    s_canvas->clear(gfx::BLACK);

    // Bufor na zamrozona klatke gry - tlo dla ekranu pauzy rysowanego w LVGL.
    s_frozen = platform::alloc_pixels((size_t)engine::CANVAS_W * engine::CANVAS_H, /*fast=*/false);
    if (!s_frozen) {
        CONSOLE_LOGE(TAG, "brak pamieci na bufor pauzy");
        return false;
    }

    if (!ui::init(s_pixels)) {
        CONSOLE_LOGE(TAG, "LVGL nie wystartowal");
        return false;
    }
    app::settings::load();   // w testach --frames zostaja domyslne (engine::deterministic)
    app::settings::apply();
    s_idle_since_us = platform::micros();
    ui::menu::show();
    s_state   = State::Menu;
    s_last_us = platform::micros();
    return true;
}

void start_game_by_index(int index)
{
    start_game(index);
}

void set_fixed_dt(float seconds)
{
    s_fixed_dt = seconds > 0.f ? seconds : 0.f;
    engine::set_deterministic(s_fixed_dt > 0.f);
}

void debug_line(char* buf, size_t n)
{
    if (!buf || n == 0) return;
    switch (s_state) {
        case State::Menu:   ui::menu::debug_line(buf, (int)n); break;
        case State::Paused: snprintf(buf, n, "PAUSED (%s)", s_game_name); break;
        case State::Playing:
            if (s_game) s_game->debug_line(buf, n);
            else        snprintf(buf, n, "PLAYING (brak gry)");
            break;
    }
}

void frame()
{
    const int64_t now = platform::micros();
    float dt = (float)(now - s_last_us) * 1e-6f;
    s_last_us = now;
    if (dt > 1.f / 30.f)  dt = 1.f / 30.f;    // po dlugiej przerwie nie "teleportuj" fizyki
    if (dt < 1.f / 240.f) dt = 1.f / 240.f;
    if (s_fixed_dt > 0.f) dt = s_fixed_dt;    // tryb testowy: powtarzalna fizyka

    switch (s_state) {
        case State::Menu: {
            // Dotyk obsluguje LVGL przez wlasne urzadzenie wejscia; klawisze interpretuje menu (karuzela, ustawienia),
            // grupa nawigacyjna LVGL (uzywana przez pauze) dostaje pusty stan.
            const input::PadState keys = platform::controller();
            if (screen_sleep(now, any_input(keys))) break;
            ui::feed_keys(input::PadState{});
            ui::menu::update(keys);
            ui::tick();
            const int picked = ui::menu::take_selection();
            if (picked >= 0) start_game(picked);
            break;
        }

        case State::Playing: {
            input::TouchPoint pts[input::MAX_TOUCH_POINTS];
            const int n = platform::read_touch(pts, input::MAX_TOUCH_POINTS);
            s_pad.begin_frame();
            s_pad.feed_touch(pts, n);
            s_pad.feed_keys(platform::controller());
            s_pad.end_frame();

            s_game->update(dt, s_pad.state());
            if (s_scale == 2) {
                s_game->render(*s_small_canvas);
                s_canvas->blit_upscale2x(s_small_pixels, engine::PIXEL_CANVAS_W, engine::PIXEL_CANVAS_H);
            } else {
                s_game->render(*s_canvas);
            }
            const int hints = app::settings::get().touch_hints;
            if (hints == app::settings::HINTS_ALWAYS || (hints == app::settings::HINTS_AUTO && !s_pad.keys_used())) {
                s_pad.draw(*s_canvas);   // automatycznie: podpowiedzi dotykowe tylko dopoki nie ma klawiatury
            }
            if (app::settings::get().show_fps) draw_fps(*s_canvas);
            // LVGL nie jest tu wolane: gra wlada calym plotnem, a kazdy przebieg
            // lv_timer_handler() kosztowalby czas procesora bez powodu.

            if (s_pad.state().start_pressed) {
                // Zamrazamy biezaca klatke i podajemy ja LVGL jako tlo - dzieki temu
                // panel pauzy sklada sie poprawnie na obrazie gry.
                memcpy(s_frozen, s_pixels,
                       (size_t)engine::CANVAS_W * engine::CANVAS_H * sizeof(uint16_t));
                ui::set_background_frame(s_frozen);
                ui::pause::show(s_game_name);
                s_state = State::Paused;
            }
            break;
        }

        case State::Paused: {
            // Gra stoi; LVGL rysuje zamrozona klatke (tlo) i panel pauzy na wierzchu.
            const input::PadState keys = platform::controller();
            if (screen_sleep(now, any_input(keys))) break;
            ui::feed_keys(keys);
            ui::tick();

            switch (ui::pause::take_result()) {
                case ui::pause::Result::Resume:
                    ui::pause::hide();
                    ui::set_background_frame(nullptr);
                    s_pad.begin_frame();
                    s_pad.end_frame();   // skasuj zbocze START, zeby nie wpasc znow w pauze
                    s_state = State::Playing;
                    break;
                case ui::pause::Result::Exit:
                    back_to_menu();
                    break;
                case ui::pause::Result::None:
                    break;
            }
            break;
        }
    }

    platform::present(s_canvas->data());
    engine::stats_tick(now);
}

void run()
{
    while (platform::should_run()) {
        frame();
    }
}

}  // namespace app
