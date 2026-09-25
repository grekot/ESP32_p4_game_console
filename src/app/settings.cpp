#include "app/settings.h"

#include "engine/storage.h"
#include "platform/platform.h"

namespace app::settings {

namespace {
Values s_values;
}

Values& get() { return s_values; }

void load()
{
    Values v;
    if (engine::load_data("settings", &v, sizeof(v)) && v.version == Values{}.version) s_values = v;
    if (s_values.brightness < 10) s_values.brightness = 10;
    if (s_values.screen_off >= SCREEN_OFF_COUNT) s_values.screen_off = 0;
    if (s_values.accent >= ACCENT_COUNT) s_values.accent = 0;
    if (s_values.touch_hints > HINTS_NEVER) s_values.touch_hints = HINTS_AUTO;
}

void save() { engine::save_data("settings", &s_values, sizeof(s_values)); }

void apply() { platform::set_brightness(s_values.brightness); }

void reset()
{
    const uint8_t last = s_values.last_game;
    s_values = Values{};
    s_values.last_game = last;
    save();
    apply();
}

}  // namespace app::settings
