// Ustawienia konsoli (ekran "Ustawienia" w menu) - zapisywane trwale przez engine::save_data ("settings").
#pragma once

#include <stdint.h>

namespace app::settings {

enum TouchHints : uint8_t { HINTS_AUTO = 0, HINTS_ALWAYS = 1, HINTS_NEVER = 2 };

struct Values {
    uint8_t version     = 1;
    uint8_t brightness  = 100;          // 10..100 %
    uint8_t screen_off  = 0;            // indeks w SCREEN_OFF_MIN (0 = nigdy)
    uint8_t touch_hints = HINTS_AUTO;   // podpowiedzi stref dotykowych w grach
    uint8_t show_fps    = 0;            // licznik FPS w rogu ekranu w trakcie gry
    uint8_t accent      = 0;            // kolor akcentu UI (indeks w ACCENTS)
    uint8_t last_game   = 0xFF;         // ostatnio uruchomiona gra (indeks w engine::GAMES), 0xFF = brak
    uint8_t reserved    = 0;
};

constexpr int SCREEN_OFF_COUNT = 5;
constexpr int SCREEN_OFF_MIN[SCREEN_OFF_COUNT] = { 0, 1, 3, 5, 10 };
constexpr int ACCENT_COUNT = 4;
constexpr uint32_t ACCENTS[ACCENT_COUNT] = { 0x3b82f6, 0x22c55e, 0xf59e0b, 0xa855f7 };   // niebieski, zielony, pomaranczowy, fioletowy

Values& get();
void    load();      // przy starcie konsoli (w testach zostaja wartosci domyslne)
void    save();
void    apply();     // jasnosc ekranu
void    reset();     // wartosci domyslne + zapis + apply

}  // namespace app::settings
