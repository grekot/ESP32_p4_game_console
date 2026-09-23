#include "ui/pause.h"

#include "engine/screen.h"
#include "lvgl.h"
#include "ui/lvgl_glue.h"

namespace ui::pause {

namespace {

lv_obj_t* s_root   = nullptr;   // przyciemnienie na caly ekran + panel w srodku
Result    s_result = Result::None;

void on_click(lv_event_t* e)
{
    s_result = (Result)(intptr_t)lv_event_get_user_data(e);
}

// Wyrazna obwodka na zaznaczonym elemencie - inaczej nawigacja klawiszami jest nieczytelna.
void mark_focusable(lv_obj_t* obj)
{
    lv_obj_set_style_outline_color(obj, lv_color_hex(0x60a5fa), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(obj, 3, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(obj, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_STATE_FOCUSED);
}

lv_obj_t* make_button(lv_obj_t* parent, const char* text, uint32_t color, Result result)
{
    lv_obj_t* btn = lv_button_create(parent);
    mark_focusable(btn);
    lv_obj_set_size(btn, 120, 40);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, (void*)(intptr_t)result);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
    return btn;
}

}  // namespace

void show(const char* game_name)
{
    if (s_root) return;
    s_result = Result::None;

    // Warstwa przyciemniajaca: pod spodem jest zamrozona klatka gry (ui::set_background_frame).
    s_root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_root);
    lv_obj_set_size(s_root, engine::CANVAS_W, engine::CANVAS_H);
    lv_obj_set_pos(s_root, 0, 0);
    lv_obj_set_style_bg_color(s_root, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_50, LV_PART_MAIN);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* panel = lv_obj_create(s_root);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, 280, 150);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x0b1220), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x3b82f6), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 8, LV_PART_MAIN);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(panel);
    lv_label_set_text(title, "PAUZA");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    lv_obj_t* name = lv_label_create(panel);
    lv_label_set_text(name, game_name ? game_name : "");
    lv_obj_set_style_text_font(name, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(0x93a4c4), 0);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 44);

    lv_obj_t* resume = make_button(panel, "WROC", 0x2563eb, Result::Resume);
    lv_obj_align(resume, LV_ALIGN_BOTTOM_LEFT, 12, -16);

    lv_obj_t* quit = make_button(panel, "MENU", 0x475569, Result::Exit);
    lv_obj_align(quit, LV_ALIGN_BOTTOM_RIGHT, -12, -16);

    // Obsluga klawiatura: LEWO/PRAWO przelacza przyciski, A zatwierdza.
    lv_group_add_obj(ui::nav_group(), resume);
    lv_group_add_obj(ui::nav_group(), quit);
    lv_group_focus_obj(resume);
}

void hide()
{
    if (!s_root) return;
    lv_group_remove_all_objs(ui::nav_group());
    lv_obj_delete(s_root);
    s_root   = nullptr;
    s_result = Result::None;
}

Result take_result()
{
    const Result r = s_result;
    s_result = Result::None;
    return r;
}

}  // namespace ui::pause
