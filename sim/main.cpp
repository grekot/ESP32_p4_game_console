// Emulator konsoli Lake na Windows.
// Uruchamia DOKLADNIE ten sam kod co plytka (app/, engine/, gfx/, input/, ui/, games/) -
// rozni je tylko implementacja warstwy platform:: w sim/platform_win32.cpp.
//
// Uzycie:
//   lake_sim.exe                              - normalne okno
//   lake_sim.exe --game 0                     - wejdz od razu do gry nr 0 (bez klikania w menu)
//   lake_sim.exe --frames 120 --shot ui.bmp   - przelicz N klatek, zapisz zrzut i zakoncz
//                                               (do dokumentacji i sprawdzania regresji UI)
//   lake_sim.exe --game 0 --pause-at 20 --frames 40 --shot pauza.bmp
//                                             - wejdz w gre, w 20. klatce wcisnij START (pauza)
//   lake_sim.exe --keymap moje.cfg            - wlasne przypisanie klawiszy PC do klawiszy konsoli
//   lake_sim.exe --game 0 --frames 120 --trace 10
//                                             - co 10 klatek wypisz linie stanu gry (pozycja, predkosc,
//                                               podloze, wynik...) - do weryfikacji mechanik liczbami
//   lake_sim.exe --game 0 --hold SELECT 5 6 --hold A 30 50 --frames 80
//                                             - skryptowane wcisniecia: klawisz od klatki N do M.
//                                               Mozna podac do 8 wpisow (scenariusze testowe).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/app.h"
#include "core/log.h"
#include "platform/platform.h"
#include "keymap_win32.h"
#include "sim_extra.h"

static const char* TAG = "lake-sim";

int main(int argc, char** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);   // logi widoczne od razu, takze po przekierowaniu
    setvbuf(stderr, nullptr, _IONBF, 0);

    int         frames   = -1;
    int         game     = -1;
    int         pause_at = -1;
    const char* shot     = nullptr;
    const char* keymap   = nullptr;
    int trace_every = 0;
    struct Hold { input::Key key; int from; int to; };
    Hold hold[8] = {};
    int  hold_count = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--shot") == 0 && i + 1 < argc) {
            shot = argv[++i];
        } else if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
            game = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--pause-at") == 0 && i + 1 < argc) {
            pause_at = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--keymap") == 0 && i + 1 < argc) {
            keymap = argv[++i];
        } else if (strcmp(argv[i], "--trace") == 0 && i + 1 < argc) {
            trace_every = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--hold") == 0 && i + 3 < argc) {
            if (hold_count >= 8) {
                LAKE_LOGE(TAG, "za duzo --hold (max 8)");
                return 2;
            }
            input::Key k;
            if (!input::key_from_name(argv[i + 1], k)) {
                LAKE_LOGE(TAG, "nieznany klawisz konsoli: %s", argv[i + 1]);
                return 2;
            }
            hold[hold_count++] = { k, atoi(argv[i + 2]), atoi(argv[i + 3]) };
            i += 3;
        } else {
            LAKE_LOGW(TAG, "nieznany argument: %s", argv[i]);
        }
    }

    LAKE_LOGI(TAG, "Lake Console - emulator");
    sim::keymap_load(keymap);

    if (!platform::init()) {
        LAKE_LOGE(TAG, "nie udalo sie otworzyc okna");
        return 1;
    }
    if (!app::init()) {
        LAKE_LOGE(TAG, "inicjalizacja konsoli nie powiodla sie");
        return 1;
    }

    if (game >= 0) {
        app::start_game_by_index(game);
    }

    if (frames > 0) {
        // Tryb skryptowany: staly krok 1/60 s, zeby wyniki nie zalezaly od obciazenia PC.
        app::set_fixed_dt(1.f / 60.f);
        for (int i = 0; i < frames && platform::should_run(); ++i) {
            // Klawisze wstrzykniete: START na jedna klatke (--pause-at) i/lub przytrzymanie (--hold).
            uint16_t syn = 0;
            if (pause_at >= 0 && i == pause_at) syn |= input::key_bit(input::Key::Start);
            for (int h = 0; h < hold_count; ++h) {
                if (i >= hold[h].from && i < hold[h].to) syn |= input::key_bit(hold[h].key);
            }
            sim::set_synthetic_keys(syn);
            app::frame();

            if (trace_every > 0 && (i % trace_every) == 0) {
                char line[160];
                app::debug_line(line, sizeof(line));
                printf("TRACE %4d %s\n", i, line);
            }
        }
        if (shot) {
            const bool ok = sim::save_screenshot(shot);
            LAKE_LOGI(TAG, "zrzut %s: %s", shot, ok ? "zapisany" : "BLAD");
            if (!ok) return 2;
        }
        LAKE_LOGI(TAG, "przeliczono %d klatek", frames);
        return 0;
    }

    app::run();
    LAKE_LOGI(TAG, "koniec");
    return 0;
}
