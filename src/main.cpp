// LakeMarioGame - mini konsola do gier na ESP32-P4 (Guition JC4880P443C)
//
// Wejscie dla plytki. Cala logika konsoli (menu LVGL, gra, pauza) siedzi w app::,
// wspoldzielona z emulatorem na Windows (sim/).

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"

#include "app/app.h"
#include "core/log.h"
#include "platform/platform.h"

static const char* TAG = "console";

namespace {

void console_task(void*)
{
    if (!app::init()) {
        CONSOLE_LOGE(TAG, "inicjalizacja konsoli nie powiodla sie");
        vTaskDelete(nullptr);
        return;
    }
    app::run();
    vTaskDelete(nullptr);
}

}  // namespace

extern "C" void app_main(void)
{
    CONSOLE_LOGI(TAG, "Console start");
    CONSOLE_LOGI(TAG, "Heap: wewnetrzny %u B, PSRAM %u B",
              (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
              (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    // NVS (na przyszlosc: highscore, ustawienia)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    if (!platform::init()) {
        CONSOLE_LOGE(TAG, "sprzet nie wystartowal - stop");
        return;
    }

    // Petla konsoli na rdzeniu 1 (rdzen 0 zostaje dla systemu i przyszlego Wi-Fi/audio).
    // 16 kB stosu: LVGL potrafi glebiej schodzic przy skladaniu widgetow.
    xTaskCreatePinnedToCore(console_task, "console", 16384, nullptr, 5, nullptr, 1);
    CONSOLE_LOGI(TAG, "Konsola uruchomiona");
}
