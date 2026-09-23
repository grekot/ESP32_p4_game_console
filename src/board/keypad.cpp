#include "board/keypad.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"

#include "board/pins.h"
#include "core/log.h"
#include "input/keys.h"

namespace board::keypad {

namespace {

const char* TAG = "keypad";

// Kolejnosc MUSI odpowiadac input::Key.
const gpio_num_t PINS[input::KEY_COUNT] = {
    pins::KEY_UP,
    pins::KEY_DOWN,
    pins::KEY_LEFT,
    pins::KEY_RIGHT,
    pins::KEY_A,
    pins::KEY_B,
    pins::KEY_X,
    pins::KEY_Y,
    pins::KEY_START,
    pins::KEY_SELECT,
    // Kierunki galki analogowej nie sa przelacznikami - obsluguje je board::joystick.
    GPIO_NUM_NC,   // StickUp
    GPIO_NUM_NC,   // StickDown
    GPIO_NUM_NC,   // StickLeft
    GPIO_NUM_NC,   // StickRight
};
static_assert(sizeof(PINS) / sizeof(PINS[0]) == (size_t)input::KEY_COUNT,
              "lista pinow musi miec tyle pozycji, ile jest klawiszy w input::Key");

// Rejestr przesuwny na klawisz: 8 kolejnych probek co 1 ms = 8 ms odklocania.
uint8_t  s_history[input::KEY_COUNT] = {};
volatile uint16_t s_held      = 0;
bool              s_available = false;

void scan_task(void*)
{
    // Stan poczatkowy: wszystkie klawisze puszczone (stan wysoki z podciagniecia).
    for (int i = 0; i < input::KEY_COUNT; ++i) s_history[i] = 0xFF;

    for (;;) {
        uint16_t mask = s_held;
        for (int i = 0; i < input::KEY_COUNT; ++i) {
            if (PINS[i] == GPIO_NUM_NC) continue;                      // klawisz niepodlaczony
            const uint8_t level = gpio_get_level(PINS[i]) ? 1u : 0u;   // 0 = wcisniety
            s_history[i] = (uint8_t)((s_history[i] << 1) | level);

            if (s_history[i] == 0x00) {
                mask |= (uint16_t)(1u << i);        // 8 probek z rzedu nisko -> wcisniety
            } else if (s_history[i] == 0xFF) {
                mask &= (uint16_t)~(1u << i);       // 8 probek z rzedu wysoko -> puszczony
            }
            // stan posredni (drgania stykow) - zostawiamy poprzednia decyzje
        }
        s_held = mask;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

}  // namespace

esp_err_t init()
{
    uint64_t pin_mask = 0;
    int      wired    = 0;
    for (int i = 0; i < input::KEY_COUNT; ++i) {
        if (PINS[i] == GPIO_NUM_NC) continue;   // np. SELECT, dla ktorego zabraklo pinu
        pin_mask |= 1ULL << (int)PINS[i];
        ++wired;
    }

    gpio_config_t cfg = {};
    cfg.pin_bit_mask = pin_mask;
    cfg.mode         = GPIO_MODE_INPUT;
    cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type    = GPIO_INTR_DISABLE;
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "gpio_config klawiatury");

    // Priorytet wyzszy niz petla gry (5), zeby odpytywanie bylo rownomierne.
    if (xTaskCreatePinnedToCore(scan_task, "keypad", 2560, nullptr, 6, nullptr, 0) != pdPASS) {
        CONSOLE_LOGE(TAG, "nie udalo sie utworzyc zadania odpytujacego");
        return ESP_ERR_NO_MEM;
    }

    s_available = true;
    CONSOLE_LOGI(TAG, "klawiatura: %d z %d klawiszy podlaczonych, odklocanie 8 ms",
              wired, input::KEY_COUNT);
    return ESP_OK;
}

uint16_t held()
{
    return s_held;
}

bool available()
{
    return s_available;
}

}  // namespace board::keypad
