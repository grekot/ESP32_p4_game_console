// Implementacja warstwy platformy dla plytki Guition JC4880P443C (ESP32-P4).
// Emulator ma swoja wersje w sim/platform_win32.cpp.
#include "platform/platform.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

#include "board/buttons.h"
#include "board/display.h"
#include "board/joystick.h"
#include "board/keypad.h"
#include "board/touch.h"
#include "core/log.h"
#include "engine/screen.h"

namespace platform {

namespace {
const char* TAG = "platform";
}

bool init()
{
    if (board::display::init() != ESP_OK) {
        LAKE_LOGE(TAG, "ekran nie wystartowal");
        return false;
    }
    if (board::touch::init() != ESP_OK) {
        LAKE_LOGW(TAG, "dotyk niedostepny - zostaje klawiatura");
    }
    board::buttons::init();
    if (board::keypad::init() != ESP_OK) {
        LAKE_LOGW(TAG, "klawiatura niedostepna - zostaje dotyk");
    }
    if (board::joystick::init() != ESP_OK) {
        LAKE_LOGW(TAG, "galka analogowa niedostepna - zostaje krzyzak");
    }
    return true;
}

int64_t micros()
{
    return esp_timer_get_time();
}

uint32_t millis()
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms ? ms : 1));
}

uint16_t* alloc_pixels(size_t pixel_count, bool fast)
{
    const size_t bytes = pixel_count * sizeof(uint16_t);
    // Wyrownanie 128 B: wymog PPA i synchronizacji cache.
    if (fast) {
        void* p = heap_caps_aligned_alloc(128, bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (p) return static_cast<uint16_t*>(p);
        LAKE_LOGW(TAG, "brak %u B w SRAM - probuje PSRAM", (unsigned)bytes);
    }
    return static_cast<uint16_t*>(heap_caps_aligned_alloc(128, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
}

void present(const uint16_t* canvas)
{
    board::display::present(canvas, engine::CANVAS_W, engine::CANVAS_H);
}

int read_touch(input::TouchPoint* out, int max_points)
{
    board::touch::Point raw[board::touch::MAX_POINTS];
    const int n = board::touch::read(raw, board::touch::MAX_POINTS);
    int count = 0;
    for (int i = 0; i < n && count < max_points; ++i) {
        // ekran logiczny (800x480) -> plotno (400x240)
        out[count].x = (int16_t)(raw[i].x / engine::CANVAS_SCALE);
        out[count].y = (int16_t)(raw[i].y / engine::CANVAS_SCALE);
        ++count;
    }
    return count;
}

input::PadState controller()
{
    input::PadState p = input::pad_from_mask(board::keypad::held());

    // Przycisk BOOT dziala jak START - konsola jest sterowalna zanim kontroler zostanie zbudowany.
    if (board::buttons::boot_pressed()) {
        p.start = true;
    }

    // Galka: surowe osie dla gier plynnych + progowanie na kierunki, zeby dzialala tak samo
    // jak krzyzak w grach cyfrowych. Prog wyzszy niz strefa martwa, zeby nie lapac skosow
    // przy lekkim wychyleniu.
    const board::joystick::Axes ax = board::joystick::read();
    p.stick_x = ax.x;
    p.stick_y = ax.y;

    constexpr float DIR_THRESHOLD = 0.5f;
    if (ax.x <= -DIR_THRESHOLD) p.left  = true;
    if (ax.x >=  DIR_THRESHOLD) p.right = true;
    if (ax.y <= -DIR_THRESHOLD) p.up    = true;
    if (ax.y >=  DIR_THRESHOLD) p.down  = true;

    return p;
}

bool should_run()
{
    return true;
}

}  // namespace platform
