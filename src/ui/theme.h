// Wspolny wyglad UI konsoli (menu, pauza, przyszle ekrany): kolory, czcionki, wymiary.
// Paleta "slate" - ciemne, nowoczesne tlo, jeden niebieski akcent. Ekran 800x480.
#pragma once

#include "lvgl.h"

namespace ui::theme {

// kolory (0xRRGGBB)
constexpr uint32_t BG         = 0x0f172a;   // tlo ekranu
constexpr uint32_t CARD       = 0x1e293b;   // karta / panel
constexpr uint32_t CARD_FOCUS = 0x334155;   // karta zaznaczona
constexpr uint32_t LINE       = 0x334155;   // separatory, pasek przewijania
constexpr uint32_t ACCENT     = 0x3b82f6;   // niebieski akcent (pasek, przycisk glowny)
constexpr uint32_t ACCENT_HI  = 0x60a5fa;
constexpr uint32_t TEXT       = 0xf8fafc;
constexpr uint32_t TEXT_MUTED = 0x94a3b8;
constexpr uint32_t TEXT_FAINT = 0x64748b;
constexpr uint32_t DANGER     = 0xef4444;

// czcionki (Montserrat wbudowane w LVGL; tylko ASCII - bez polskich liter w UI)
#define CONSOLE_UI_FONT_TITLE      (&lv_font_montserrat_32)
#define CONSOLE_UI_FONT_CARD_TITLE (&lv_font_montserrat_24)
#define CONSOLE_UI_FONT_BODY       (&lv_font_montserrat_16)
#define CONSOLE_UI_FONT_SMALL      (&lv_font_montserrat_14)
#define CONSOLE_UI_FONT_BUTTON     (&lv_font_montserrat_20)
inline const lv_font_t* const FONT_TITLE      = CONSOLE_UI_FONT_TITLE;
inline const lv_font_t* const FONT_CARD_TITLE = CONSOLE_UI_FONT_CARD_TITLE;
inline const lv_font_t* const FONT_BODY       = CONSOLE_UI_FONT_BODY;
inline const lv_font_t* const FONT_SMALL      = CONSOLE_UI_FONT_SMALL;
inline const lv_font_t* const FONT_BUTTON     = CONSOLE_UI_FONT_BUTTON;

// wymiary
constexpr int MARGIN   = 32;    // margines boczny ekranu
constexpr int HEADER_H = 84;
constexpr int FOOTER_H = 44;
constexpr int CARD_H   = 64;
constexpr int CARD_GAP = 10;
constexpr int RADIUS   = 14;

}  // namespace ui::theme
