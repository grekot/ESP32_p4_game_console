// Petla konsoli: menu (LVGL) -> gra -> pauza (LVGL na wierzchu gry) -> menu.
// Kod wspolny dla plytki i emulatora - rozni je tylko warstwa platform::.
#pragma once

#include <stddef.h>

namespace app {

// Alokuje plotno, uruchamia LVGL i pokazuje menu. platform::init() musi byc wczesniej.
bool init();

// Jedna klatka. Emulator moze wolac to sam, jesli potrzebuje wlasnej petli.
void frame();

// Linia stanu biezacej gry (albo stanu konsoli), do logow i testow emulatora (--trace).
void debug_line(char* buf, size_t n);

// Staly krok czasu w sekundach (0 = mierz czas rzeczywisty, tak jak na plytce).
// Emulator ustawia 1/60 w trybie skryptowanym, zeby ten sam scenariusz dawal zawsze ten sam wynik.
void set_fixed_dt(float seconds);

// Wejscie prosto do gry o podanym indeksie (engine::GAMES), z pominieciem menu.
// Emulator uzywa tego przy --game N, zeby przy pracy nad gra nie klikac za kazdym razem.
void start_game_by_index(int index);

// Petla do zamkniecia okna (emulator) albo w nieskonczonosc (plytka).
void run();

}  // namespace app
