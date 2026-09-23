// Menu glowne konsoli: lista gier jako karty, w natywnej rozdzielczosci 800x480.
//
// Styl: ciemne tlo (slate), karty z zaokragleniem, zaznaczona karta jasniejsza z niebieskim paskiem
// po lewej (zamiast obwodki), opis w przygaszonym kolorze. Bez diakrytykow - wbudowane czcionki
// Montserrat LVGL maja tylko ASCII.
#include "ui/menu.h"

#include <stdio.h>

#include "engine/game_registry.h"
#include "engine/screen.h"
#include "lvgl.h"
#include "ui/lvgl_glue.h"
#include "ui/theme.h"

namespace ui::menu {

namespace {

lv_obj_t* s_root      = nullptr;
int       s_selection = -1;

void on_game_clicked(lv_event_t* e)
{
    s_selection = (int)(intptr_t)lv_event_get_user_data(e);
}

lv_obj_t* make_card(lv_obj_t* list, int index)
{
    lv_obj_t* card = lv_button_create(list);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, lv_pct(100), theme::CARD_H);
    lv_obj_set_style_bg_color(card, lv_color_hex(theme::CARD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(card, theme::RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_left(card, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_right(card, 20, LV_PART_MAIN);
    // Zaznaczenie (klawisze) i wcisniecie (dotyk): jasniejsza karta + akcentowy pasek po lewej.
    lv_obj_set_style_bg_color(card, lv_color_hex(theme::CARD_FOCUS), LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(card, lv_color_hex(theme::CARD_FOCUS), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(card, lv_color_hex(theme::ACCENT), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(card, 4, LV_STATE_FOCUSED);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_LEFT, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(card, on_game_clicked, LV_EVENT_CLICKED, (void*)(intptr_t)index);
    lv_group_add_obj(ui::nav_group(), card);

    lv_obj_t* name = lv_label_create(card);
    lv_label_set_text(name, engine::GAMES[index].name);
    lv_obj_set_style_text_font(name, theme::FONT_CARD_TITLE, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(theme::TEXT), 0);
    lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, -13);

    lv_obj_t* desc = lv_label_create(card);
    lv_label_set_text(desc, engine::GAMES[index].description);
    lv_obj_set_style_text_font(desc, theme::FONT_BODY, 0);
    lv_obj_set_style_text_color(desc, lv_color_hex(theme::TEXT_MUTED), 0);
    lv_obj_align(desc, LV_ALIGN_LEFT_MID, 0, 14);

    // Numer gry po prawej - ten sam, ktory rozumie `console_sim --game N`.
    char num[8];
    snprintf(num, sizeof(num), "%02d", index);
    lv_obj_t* badge = lv_label_create(card);
    lv_label_set_text(badge, num);
    lv_obj_set_style_text_font(badge, theme::FONT_BODY, 0);
    lv_obj_set_style_text_color(badge, lv_color_hex(theme::TEXT_FAINT), 0);
    lv_obj_align(badge, LV_ALIGN_RIGHT_MID, 0, 0);
    return card;
}

}  // namespace

void show()
{
    if (s_root) return;
    s_selection = -1;

    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    s_root = lv_obj_create(scr);
    lv_obj_remove_style_all(s_root);
    lv_obj_set_size(s_root, engine::CANVAS_W, engine::CANVAS_H);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);

    // --- naglowek: tytul + podtytul z liczba gier ---
    lv_obj_t* title = lv_label_create(s_root);
    lv_label_set_text(title, "Console");
    lv_obj_set_style_text_font(title, theme::FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(theme::TEXT), 0);
    lv_obj_set_pos(title, theme::MARGIN, 22);

    char sub[48];
    snprintf(sub, sizeof(sub), "ESP32-P4  |  %d gier", engine::GAME_COUNT);
    lv_obj_t* subtitle = lv_label_create(s_root);
    lv_label_set_text(subtitle, sub);
    lv_obj_set_style_text_font(subtitle, theme::FONT_BODY, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(theme::TEXT_MUTED), 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_RIGHT, -theme::MARGIN, 34);

    lv_obj_t* rule = lv_obj_create(s_root);
    lv_obj_remove_style_all(rule);
    lv_obj_set_size(rule, engine::CANVAS_W - 2 * theme::MARGIN, 2);
    lv_obj_set_pos(rule, theme::MARGIN, theme::HEADER_H - 2);
    lv_obj_set_style_bg_color(rule, lv_color_hex(theme::LINE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, LV_PART_MAIN);

    // --- lista kart ---
    const int list_y = theme::HEADER_H + 14;
    const int list_h = engine::CANVAS_H - list_y - theme::FOOTER_H;
    lv_obj_t* list = lv_obj_create(s_root);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, engine::CANVAS_W - 2 * theme::MARGIN, list_h);
    lv_obj_set_pos(list, theme::MARGIN, list_y);
    lv_obj_set_style_pad_row(list, theme::CARD_GAP, LV_PART_MAIN);
    lv_obj_set_style_pad_right(list, 14, LV_PART_MAIN);   // miejsce na pasek przewijania
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(list, lv_color_hex(theme::LINE), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(list, 6, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(list, 3, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_right(list, 2, LV_PART_SCROLLBAR);

    for (int i = 0; i < engine::GAME_COUNT; ++i) {
        lv_obj_t* card = make_card(list, i);
        if (i == 0) lv_group_focus_obj(card);
    }

    // --- stopka ---
    lv_obj_t* hint = lv_label_create(s_root);
    lv_label_set_text(hint, "Gora / Dol - wybor      A - start      dotknij karty, aby zagrac");
    lv_obj_set_style_text_font(hint, theme::FONT_SMALL, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(theme::TEXT_FAINT), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -12);
}

void hide()
{
    if (!s_root) return;
    lv_group_remove_all_objs(ui::nav_group());
    lv_obj_delete(s_root);
    s_root      = nullptr;
    s_selection = -1;
}

int take_selection()
{
    const int sel = s_selection;
    s_selection = -1;
    return sel;
}

}  // namespace ui::menu
