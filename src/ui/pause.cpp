// Ekran pauzy: przyciemniona, zamrozona klatka gry (ui::set_background_frame) i panel na srodku.
#include "ui/pause.h"

#include "engine/screen.h"
#include "lvgl.h"
#include "ui/lvgl_glue.h"
#include "ui/theme.h"

namespace ui::pause {

namespace {

lv_obj_t* s_root   = nullptr;   // przyciemnienie na caly ekran + panel w srodku
Result    s_result = Result::None;

void on_click(lv_event_t* e)
{
    s_result = (Result)(intptr_t)lv_event_get_user_data(e);
}

lv_obj_t* make_button(lv_obj_t* parent, const char* text, uint32_t color, Result result)
{
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, 170, 56);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, theme::RADIUS, LV_PART_MAIN);
    // Zaznaczenie klawiszami: jasna obwodka; wcisniecie dotykiem: jasniejsze tlo.
    lv_obj_set_style_outline_color(btn, lv_color_hex(theme::ACCENT_HI), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 3, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(btn, 3, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(btn, lv_color_hex(theme::ACCENT_HI), LV_STATE_PRESSED);
    lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, (void*)(intptr_t)result);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, theme::FONT_BUTTON, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(theme::TEXT), 0);
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
    lv_obj_set_style_bg_color(s_root, lv_color_hex(0x020617), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_60, LV_PART_MAIN);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* panel = lv_obj_create(s_root);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, 440, 250);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(theme::BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(theme::LINE), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 22, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 40, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(panel, lv_color_black(), LV_PART_MAIN);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(panel);
    lv_label_set_text(title, "Pauza");
    lv_obj_set_style_text_font(title, theme::FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(theme::TEXT), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t* name = lv_label_create(panel);
    lv_label_set_text(name, game_name ? game_name : "");
    lv_obj_set_style_text_font(name, theme::FONT_BODY, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(theme::TEXT_MUTED), 0);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 76);

    lv_obj_t* resume = make_button(panel, "Wroc do gry", theme::ACCENT, Result::Resume);
    lv_obj_align(resume, LV_ALIGN_BOTTOM_LEFT, 40, -34);

    lv_obj_t* quit = make_button(panel, "Menu", theme::CARD_FOCUS, Result::Exit);
    lv_obj_align(quit, LV_ALIGN_BOTTOM_RIGHT, -40, -34);

    // Obsluga klawiatura: LEWO/PRAWO przelacza przyciski, A zatwierdza, B cofa.
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
