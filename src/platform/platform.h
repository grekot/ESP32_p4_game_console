// Cienka warstwa platformy - jedyna rzecz, ktora rozni plytke od emulatora na Windows.
// Wszystko powyzej (engine, gfx, ui, games) jest wspolne dla obu.
//
// Implementacje:
//   plytka    -> src/platform/platform_esp.cpp   (board::display / board::touch / board::buttons)
//   emulator  -> sim/platform_win32.cpp          (okno Win32, klawiatura, mysz)
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "input/pad.h"

namespace platform {

// Inicjalizacja ekranu i wejscia. Zwraca false, gdy sprzet/okno nie wystartowalo.
bool init();

// Czas od startu w mikrosekundach (monotoniczny).
int64_t micros();

// Milisekundy - dla LVGL (lv_tick_set_cb).
uint32_t millis();

void sleep_ms(uint32_t ms);

// Bufor pikseli RGB565 wyrownany pod DMA/cache. fast = preferuj szybka pamiec wewnetrzna.
uint16_t* alloc_pixels(size_t pixel_count, bool fast);

// Wyswietla gotowa klatke plotna (engine::CANVAS_W x CANVAS_H). Blokuje do synchronizacji pionowej.
void present(const uint16_t* canvas);

// Punkty dotyku we wspolrzednych PLOTNA (0..CANVAS_W-1, 0..CANVAS_H-1). Zwraca ich liczbe.
int read_touch(input::TouchPoint* out, int max_points);

// Stan kontrolera: klawisze (input::Key) + osie galki analogowej.
// Plytka: przelaczniki i galka na JP1 + przycisk BOOT jako zapasowy START.
// Emulator: klawisze PC wg keymap.cfg, osie z pada USB albo z klawiszy.
// Wychylenie galki jest juz przelozone na kierunki, wiec gry moga czytac same up/down/left/right.
input::PadState controller();

// Emulator: false gdy uzytkownik zamknal okno. Na plytce zawsze true.
bool should_run();

// Pliki z zasobami (PNG, w przyszlosci dzwieki, poziomy). Sciezka wzgledna, np. "bohater/hero.png":
//   plytka   -> partycja `assets` (SPIFFS) zamontowana pod /assets; wgrywanie: pio run -t uploadfs
//   emulator -> katalog assets/ w repozytorium (szukany od katalogu roboczego i od pliku exe)
// Zwraca bufor z cala zawartoscia (zwolnic przez free_file) i rozmiar; nullptr gdy pliku nie ma.
uint8_t* read_file(const char* path, size_t& size);
void     free_file(uint8_t* data);

// Trwaly zapis malych danych (ustawienia, rekordy): plytka -> NVS (przestrzen "console"), emulator -> plik
// save_<key>.bin obok exe. key: do 15 znakow [a-z0-9_]. load zwraca false, gdy zapisu nie ma albo ma inny rozmiar
// (wtedy data zostaje nietknieta - wartosci domyslne). Gry uzywaja engine::load_data/save_data (wylaczone w testach).
bool load_blob(const char* key, void* data, size_t size);
bool save_blob(const char* key, const void* data, size_t size);
void erase_blob(const char* key);

// Jasnosc podswietlenia 0..100 % (plytka: PWM LEDC na pinie podswietlenia, emulator: przyciemnienie obrazu).
void set_brightness(int percent);

// Wolna / calkowita pamiec (bajty). Emulator zwraca zera - ekran "O konsoli" pisze wtedy "emulator".
struct MemInfo {
    size_t sram_free = 0, sram_total = 0, psram_free = 0, psram_total = 0;
};
MemInfo memory_info();

}  // namespace platform
