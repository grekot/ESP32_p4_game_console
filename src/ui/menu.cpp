#include "ui/menu.h"

#include "engine/game_registry.h"
#include "engine/screen.h"
#include "lvgl.h"
#include "ui/lvgl_glue.h"

namespace ui::menu {

namespace {

lv_obj_t* s_root      = nullptr;
int       s_selection = -1;

// Wyrazna obwodka na zaznaczonym elemencie - inaczej nawigacja klawiszami jest nieczytelna.
void mark_focusable(lv_obj_t* obj)
{
    lv_obj_set_style_outline_color(obj, lv_color_hex(0x60a5fa), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(obj, 3, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(obj, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_STATE_FOCUSED);
}

void on_game_clicked(lv_event_t* e)
{
    s_selection = (int)(intptr_t)lv_event_get_user_data(e);
}

}  // namespace

void show()
{
    if (s_root) return;
    s_selection = -1;

    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101828), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    s_root = lv_obj_create(scr);
    lv_obj_remove_style_all(s_root);
    lv_obj_set_size(s_root, engine::CANVAS_W, engine::CANVAS_H);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(0x101828), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_root, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);

    // --- pasek tytulowy ---
    lv_obj_t* header = lv_obj_create(s_root);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, engine::CANVAS_W, 44);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1d4ed8), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "LAKE CONSOLE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 12, -5);

    lv_obj_t* sub = lv_label_create(header);
    lv_label_set_text(sub, "ESP32-P4");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0xbfdbfe), 0);
    lv_obj_align(sub, LV_ALIGN_LEFT_MID, 13, 12);

    // --- lista gier ---
    lv_obj_t* list = lv_obj_create(s_root);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, engine::CANVAS_W - 24, engine::CANVAS_H - 44 - 30);
    lv_obj_set_pos(list, 12, 52);
    lv_obj_set_style_pad_row(list, 8, LV_PART_MAIN);
    // Margines wewnetrzny, zeby obwodka zaznaczenia miescila sie w calosci.
    lv_obj_set_style_pad_all(list, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    for (int i = 0; i < engine::GAME_COUNT; ++i) {
        lv_obj_t* btn = lv_button_create(list);
        lv_obj_set_size(btn, lv_pct(100), 52);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1f2a44), LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2563eb), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, on_game_clicked, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        // Dzieki temu gre da sie wybrac takze klawiatura (GORA/DOL + A).
        mark_focusable(btn);
        lv_group_add_obj(ui::nav_group(), btn);
        if (i == 0) lv_group_focus_obj(btn);

        lv_obj_t* name = lv_label_create(btn);
        lv_label_set_text(name, engine::GAMES[i].name);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(name, lv_color_white(), 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 4, -8);

        lv_obj_t* desc = lv_label_create(btn);
        lv_label_set_text(desc, engine::GAMES[i].description);
        lv_obj_set_style_text_font(desc, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(desc, lv_color_hex(0x93a4c4), 0);
        lv_obj_align(desc, LV_ALIGN_LEFT_MID, 4, 11);
    }

    // --- stopka ---
    lv_obj_t* hint = lv_label_create(s_root);
    lv_label_set_text(hint, "Dotknij gry albo wybierz klawiszami: GORA/DOL + A");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x64748b), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -8);
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
