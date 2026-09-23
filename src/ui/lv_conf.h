// Konfiguracja LVGL 9 - WSPOLNA dla plytki i emulatora Windows.
//
// LVGL definiuje wartosci domyslne dla wszystkiego, czego tu nie ma (lv_conf_internal.h),
// wiec plik jest celowo krotki - sa w nim tylko ustawienia istotne dla tej konsoli.
// Plik jest znajdowany przez LVGL dzieki katalogowi src/ui na sciezce include (-I).
//
// Straznik MUSI nazywac sie LV_CONF_H - po tej nazwie LVGL poznaje, ze konfiguracja faktycznie
// zostala wciagnieta (inaczej ostrzega "Possible failure to include lv_conf.h").
#ifndef LV_CONF_H
#define LV_CONF_H

// --- Format koloru: RGB565, tak jak bufor ramki panelu i plotno gry ---
#define LV_COLOR_DEPTH 16

// --- Pamiec: zwykly malloc/free zamiast wlasnej puli LVGL ---
// Na ESP32-P4 duze alokacje moga wtedy trafic do PSRAM (CONFIG_SPIRAM_USE_MALLOC).
#define LV_USE_STDLIB_MALLOC  LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING  LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

// --- Brak integracji z systemem operacyjnym: lv_timer_handler() wolamy sami z petli aplikacji ---
#define LV_USE_OS LV_OS_NONE

// --- Zegar: podajemy wlasne zrodlo czasu przez lv_tick_set_cb() (platform::millis) ---
#define LV_DEF_REFR_PERIOD 16   // ~60 Hz

// --- Rysowanie programowe (bez asemblera - RISC-V nie ma tu optymalizacji LVGL) ---
#define LV_USE_DRAW_SW          1
#define LV_DRAW_SW_ASM          LV_DRAW_SW_ASM_NONE
#define LV_DRAW_SW_COMPLEX      1
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_SUPPORT_ARGB8888 1

// --- Czcionki (plotno ma 400x240, wiec male rozmiary; skalowanie x2 robi sprzet) ---
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

// --- Widgety i motyw ---
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

// --- Logi LVGL (wlacz przy diagnozowaniu UI) ---
#define LV_USE_LOG 0

// --- Asercje: wlaczone na hoscie (latwiej zlapac blad w emulatorze), oszczedne na plytce ---
#if defined(LAKE_HOST_BUILD)
#define LV_USE_ASSERT_NULL      1
#define LV_USE_ASSERT_MALLOC    1
#define LV_USE_ASSERT_OBJ       1
#else
#define LV_USE_ASSERT_NULL      1
#define LV_USE_ASSERT_MALLOC    1
#define LV_USE_ASSERT_OBJ       0
#endif

// --- Niepotrzebne w tym projekcie ---
#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0

#endif  // LV_CONF_H
