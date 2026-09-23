#include "board/buttons.h"
#include "board/pins.h"

#include "driver/gpio.h"

namespace board::buttons {

void init()
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << pins::BTN_BOOT;
    cfg.mode         = GPIO_MODE_INPUT;
    cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&cfg);
}

bool boot_pressed()
{
    return gpio_get_level(pins::BTN_BOOT) == 0;
}

}  // namespace board::buttons
