#include "board/joystick.h"

#include <stdlib.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"

#include "board/pins.h"
#include "core/log.h"

namespace board::joystick {

namespace {

const char* TAG = "joystick";

// Na ESP32-P4 wejscia ADC ma tylko GPIO49-54; z tego na JP1 wychodza 49-52 (ADC2 kanaly 0-3).
constexpr adc_unit_t    UNIT      = ADC_UNIT_2;
constexpr adc_channel_t CHAN_X    = ADC_CHANNEL_0;   // GPIO49
constexpr adc_channel_t CHAN_Y    = ADC_CHANNEL_1;   // GPIO50

constexpr int   ADC_MAX      = 4095;                 // 12 bitow
constexpr int   ADC_MID      = ADC_MAX / 2;
constexpr float DEAD_ZONE    = 0.25f;                // tanie galki mocno plywaja wokol srodka
constexpr int   CALIB_SAMPLES = 32;
// Jesli zmierzony srodek odjedzie dalej niz to od polowy zakresu, znaczy ze galka byla
// trzymana przy starcie albo nic nie jest podlaczone - wtedy bierzemy polowe zakresu.
constexpr int   CALIB_MAX_OFFSET = ADC_MAX / 4;

adc_oneshot_unit_handle_t s_adc = nullptr;
int  s_center_x  = ADC_MID;
int  s_center_y  = ADC_MID;
bool s_available = false;

bool read_raw(adc_channel_t ch, int& out)
{
    return s_adc && adc_oneshot_read(s_adc, ch, &out) == ESP_OK;
}

int average(adc_channel_t ch, int samples)
{
    long sum = 0;
    int  got = 0;
    for (int i = 0; i < samples; ++i) {
        int v = 0;
        if (read_raw(ch, v)) { sum += v; ++got; }
    }
    return got ? (int)(sum / got) : ADC_MID;
}

// surowy odczyt -> -1..1 wzgledem zmierzonego srodka, ze strefa martwa
float normalize(int raw, int center)
{
    const int span = (raw >= center) ? (ADC_MAX - center) : center;
    if (span <= 0) return 0.f;

    float v = (float)(raw - center) / (float)span;      // -1..1
    if (v > 1.f)  v = 1.f;
    if (v < -1.f) v = -1.f;

    const float mag = v < 0 ? -v : v;
    if (mag < DEAD_ZONE) return 0.f;
    // Poza strefa martwa rozciagamy z powrotem na pelny zakres, zeby dalo sie osiagnac 1.0.
    const float scaled = (mag - DEAD_ZONE) / (1.f - DEAD_ZONE);
    return v < 0 ? -scaled : scaled;
}

}  // namespace

esp_err_t init()
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id  = UNIT;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &s_adc), TAG, "adc_oneshot_new_unit");

    adc_oneshot_chan_cfg_t chan_cfg = {};
    chan_cfg.atten    = ADC_ATTEN_DB_12;   // pelny zakres do ~3,3 V (modul zasilany z 3V3)
    chan_cfg.bitwidth = ADC_BITWIDTH_12;
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc, CHAN_X, &chan_cfg), TAG, "kanal X");
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc, CHAN_Y, &chan_cfg), TAG, "kanal Y");

    // Pomiar polozenia spoczynkowego. Galki nie wolno teraz dotykac.
    const int cx = average(CHAN_X, CALIB_SAMPLES);
    const int cy = average(CHAN_Y, CALIB_SAMPLES);

    s_center_x = (abs(cx - ADC_MID) <= CALIB_MAX_OFFSET) ? cx : ADC_MID;
    s_center_y = (abs(cy - ADC_MID) <= CALIB_MAX_OFFSET) ? cy : ADC_MID;

    if (s_center_x != cx || s_center_y != cy) {
        CONSOLE_LOGW(TAG, "srodek poza zakresem (X=%d Y=%d) - galka trzymana przy starcie albo "
                       "niepodlaczona; biore polowe zakresu", cx, cy);
    }

    s_available = true;
    CONSOLE_LOGI(TAG, "galka na ADC2: srodek X=%d Y=%d, strefa martwa %d%%",
              s_center_x, s_center_y, (int)(DEAD_ZONE * 100));
    return ESP_OK;
}

Axes read()
{
    Axes a;
    if (!s_available) return a;

    int rx = 0, ry = 0;
    if (read_raw(CHAN_X, rx)) a.x = normalize(rx, s_center_x);
    if (read_raw(CHAN_Y, ry)) a.y = normalize(ry, s_center_y);
    return a;
}

bool available()
{
    return s_available;
}

}  // namespace board::joystick
