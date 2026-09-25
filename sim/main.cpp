// Emulator konsoli na Windows.
// Uruchamia DOKLADNIE ten sam kod co plytka (app/, engine/, gfx/, input/, ui/, console/, games/) -
// rozni je tylko implementacja warstwy platform:: w sim/platform_win32.cpp.
//
// Uzycie:
//   console_sim.exe                              - normalne okno
//   console_sim.exe --list                       - wypisz gry (numer, id, nazwa) i zakoncz
//   console_sim.exe --game 0                     - wejdz od razu do gry nr 0 (bez klikania w menu)
//   console_sim.exe --game pilka                 - to samo po id albo nazwie; dziala tez sciezka katalogu
//                                               lekcji (src/games/lekcje/02_pilka -> "pilka")
//   console_sim.exe --frames 120 --shot ui.bmp   - przelicz N klatek, zapisz zrzut i zakoncz
//                                               (do dokumentacji i sprawdzania regresji UI)
//   console_sim.exe --game 0 --pause-at 20 --frames 40 --shot pauza.bmp
//                                             - wejdz w gre, w 20. klatce wcisnij START (pauza)
//   console_sim.exe --keymap moje.cfg            - wlasne przypisanie klawiszy PC do klawiszy konsoli
//   console_sim.exe --game 0 --frames 120 --trace 10
//                                             - co 10 klatek wypisz linie stanu gry (pozycja, predkosc,
//                                               podloze, wynik...) - do weryfikacji mechanik liczbami
//   console_sim.exe --game 0 --hold SELECT 5 6 --hold A 30 50 --frames 80
//                                             - skryptowane wcisniecia: klawisz od klatki N do M.
//                                               Mozna podac do 12 wpisow (scenariusze testowe).
//   console_sim.exe --game labirynt3d --frames 600 --bench
//                                             - sredni i maksymalny czas app::frame() w ms (bez 10 pierwszych
//                                               klatek); porownuj gry miedzy soba, nie z plytka
// W trybie --frames krok czasu jest staly (1/60 s) i nie ma czekania na 60 FPS - ten sam
// skrypt daje zawsze identyczny wynik, a 600 klatek liczy sie w ulamku sekundy.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/app.h"
#include "core/log.h"
#include "engine/game_registry.h"
#include "platform/platform.h"
#include "keymap_win32.h"
#include "sim_extra.h"

static const char* TAG = "console-sim";

static void print_games(FILE* out)
{
    for (int i = 0; i < engine::GAME_COUNT; ++i) {
        fprintf(out, "  %2d  %-12s %s - %s\n", i, engine::GAMES[i].id, engine::GAMES[i].name,
                engine::GAMES[i].description);
    }
}

int main(int argc, char** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);   // logi widoczne od razu, takze po przekierowaniu
    setvbuf(stderr, nullptr, _IONBF, 0);

    int         frames   = -1;
    bool        bench    = false;
    const char* game_arg = nullptr;
    int         pause_at = -1;
    const char* shot     = nullptr;
    const char* keymap   = nullptr;
    int trace_every = 0;
    struct Hold { input::Key key; int from; int to; };
    Hold hold[12] = {};
    int  hold_count = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--list") == 0) {
            printf("Gry w konsoli (numer, id, nazwa - opis):\n");
            print_games(stdout);
            return 0;
        } else if (strcmp(argv[i], "--bench") == 0) {
            bench = true;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--shot") == 0 && i + 1 < argc) {
            shot = argv[++i];
        } else if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
            game_arg = argv[++i];
        } else if (strcmp(argv[i], "--pause-at") == 0 && i + 1 < argc) {
            pause_at = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--keymap") == 0 && i + 1 < argc) {
            keymap = argv[++i];
        } else if (strcmp(argv[i], "--trace") == 0 && i + 1 < argc) {
            trace_every = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--hold") == 0 && i + 3 < argc) {
            if (hold_count >= 12) {
                CONSOLE_LOGE(TAG, "za duzo --hold (max 12)");
                return 2;
            }
            input::Key k;
            if (!input::key_from_name(argv[i + 1], k)) {
                CONSOLE_LOGE(TAG, "nieznany klawisz konsoli: %s", argv[i + 1]);
                return 2;
            }
            hold[hold_count++] = { k, atoi(argv[i + 2]), atoi(argv[i + 3]) };
            i += 3;
        } else {
            CONSOLE_LOGW(TAG, "nieznany argument: %s", argv[i]);
        }
    }

    // Gre sprawdzamy PRZED otwarciem okna - blad ma byc widoczny od razu, bez migajacego okna.
    int game = -1;
    if (game_arg) {
        game = engine::find_game(game_arg);
        if (game < 0) {
            CONSOLE_LOGE(TAG, "nie ma gry \"%s\" - dostepne (numer, id, nazwa):", game_arg);
            print_games(stderr);
            fprintf(stderr, "Lekcja nie jest na liscie? Sprawdz CONSOLE_ADD_GAME(...) na koncu gra.cpp i wpis w lekcje/lista.h.\n");
            return 2;
        }
    }

    CONSOLE_LOGI(TAG, "Console - emulator");
    sim::keymap_load(keymap);

    // Tryb skryptowany: bez okna (nie zabiera fokusu osobie pracujacej przy komputerze) i bez prawdziwej klawiatury.
    if (frames > 0) sim::set_ignore_real_input(true);

    if (!platform::init()) {
        CONSOLE_LOGE(TAG, "nie udalo sie otworzyc okna");
        return 1;
    }
    // Tryb skryptowany: staly krok 1/60 s PRZED app::init i startem gry - app::start_game() ustawia wtedy stale
    // ziarno losowosci (gry z random() daja identyczny slad), a konsola nie czyta zapisanych ustawien ani rekordow
    // (engine::deterministic) i pomija ekran startowy.
    if (frames > 0) app::set_fixed_dt(1.f / 60.f);

    if (!app::init()) {
        CONSOLE_LOGE(TAG, "inicjalizacja konsoli nie powiodla sie");
        return 1;
    }

    if (game >= 0) {
        app::start_game_by_index(game);
    }

    if (frames > 0) {
        sim::set_unthrottled(true);   // bez czekania na 60 FPS - 600 klatek liczy sie w ulamku sekundy
        int64_t bench_sum_us = 0, bench_max_us = 0;
        int     bench_n = 0;
        for (int i = 0; i < frames && platform::should_run(); ++i) {
            // Klawisze wstrzykniete: START na jedna klatke (--pause-at) i/lub przytrzymanie (--hold).
            uint16_t syn = 0;
            if (pause_at >= 0 && i == pause_at) syn |= input::key_bit(input::Key::Start);
            for (int h = 0; h < hold_count; ++h) {
                if (i >= hold[h].from && i < hold[h].to) syn |= input::key_bit(hold[h].key);
            }
            sim::set_synthetic_keys(syn);
            const int64_t t0 = platform::micros();
            app::frame();
            const int64_t us = platform::micros() - t0;
            if (i >= 10) {   // pierwsze klatki pomijamy (ladowanie assetow, zimny cache)
                bench_sum_us += us;
                if (us > bench_max_us) bench_max_us = us;
                ++bench_n;
            }

            if (trace_every > 0 && (i % trace_every) == 0) {
                char line[256];
                app::debug_line(line, sizeof(line));
                printf("TRACE %4d %s\n", i, line);
            }
        }
        if (shot) {
            const bool ok = sim::save_screenshot(shot);
            CONSOLE_LOGI(TAG, "zrzut %s: %s", shot, ok ? "zapisany" : "BLAD");
            if (!ok) return 2;
        }
        if (bench && bench_n > 0) {
            // Czas app::frame() = logika + render + kopia do okna (GDI). Na PC; P4 patrz docs/EMULATOR.md.
            printf("BENCH klatek=%d srednio=%.2f ms max=%.2f ms\n", bench_n, bench_sum_us / 1000.0 / bench_n,
                   bench_max_us / 1000.0);
        }
        CONSOLE_LOGI(TAG, "przeliczono %d klatek", frames);
        return 0;
    }

    app::run();
    CONSOLE_LOGI(TAG, "koniec");
    return 0;
}
