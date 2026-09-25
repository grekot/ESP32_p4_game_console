// Implementacja warstwy platformy dla plytki Guition JC4880P443C (ESP32-P4).
// Emulator ma swoja wersje w sim/platform_win32.cpp.
#include "platform/platform.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "nvs.h"

#include <stdio.h>
#include <stdlib.h>

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
bool s_assets_mounted = false;

// Partycja `assets` (SPIFFS, ~12 MB, partitions.csv) pod /assets. Pierwsze montowanie pustej partycji
// formatuje ja (kilka sekund). Brak partycji albo blad montowania nie zatrzymuje konsoli - gry po prostu
// nie dostana plikow (load_image zwroci pusty obrazek). Zawartosc: katalog assets/ w repo, pio run -t uploadfs.
void mount_assets()
{
    esp_vfs_spiffs_conf_t conf = {};
    conf.base_path              = "/assets";
    conf.partition_label        = "assets";
    conf.max_files              = 4;
    conf.format_if_mount_failed = true;
    const esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        CONSOLE_LOGW(TAG, "partycja assets niedostepna (%s) - bez plikow PNG", esp_err_to_name(err));
        return;
    }
    size_t total = 0, used = 0;
    esp_spiffs_info("assets", &total, &used);
    CONSOLE_LOGI(TAG, "assets: %u kB uzyte z %u kB", (unsigned)(used / 1024), (unsigned)(total / 1024));
    s_assets_mounted = true;
}
}  // namespace

bool init()
{
    if (board::display::init() != ESP_OK) {
        CONSOLE_LOGE(TAG, "ekran nie wystartowal");
        return false;
    }
    mount_assets();
    if (board::touch::init() != ESP_OK) {
        CONSOLE_LOGW(TAG, "dotyk niedostepny - zostaje klawiatura");
    }
    board::buttons::init();
    if (board::keypad::init() != ESP_OK) {
        CONSOLE_LOGW(TAG, "klawiatura niedostepna - zostaje dotyk");
    }
    if (board::joystick::init() != ESP_OK) {
        CONSOLE_LOGW(TAG, "galka analogowa niedostepna - zostaje krzyzak");
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
        CONSOLE_LOGW(TAG, "brak %u B w SRAM - probuje PSRAM", (unsigned)bytes);
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

uint8_t* read_file(const char* path, size_t& size)
{
    size = 0;
    if (!path || !*path || !s_assets_mounted) return nullptr;
    char full[128];
    snprintf(full, sizeof(full), "/assets/%s", path);
    FILE* f = fopen(full, "rb");
    if (!f) {
        CONSOLE_LOGW(TAG, "brak pliku %s", full);
        return nullptr;
    }
    fseek(f, 0, SEEK_END);
    const long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) { fclose(f); return nullptr; }
    // Zawartosc pliku w PSRAM - PNG bywaja duze, a SRAM jest cenny.
    uint8_t* buf = static_cast<uint8_t*>(heap_caps_malloc((size_t)len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf) buf = static_cast<uint8_t*>(malloc((size_t)len));
    if (!buf) { fclose(f); return nullptr; }
    const size_t got = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (got != (size_t)len) { free(buf); return nullptr; }
    size = got;
    return buf;
}

void free_file(uint8_t* data)
{
    free(data);
}

bool load_blob(const char* key, void* data, size_t size)
{
    nvs_handle_t h;
    if (nvs_open("console", NVS_READONLY, &h) != ESP_OK) return false;
    size_t len = 0;
    bool ok = nvs_get_blob(h, key, nullptr, &len) == ESP_OK && len == size && nvs_get_blob(h, key, data, &len) == ESP_OK;
    nvs_close(h);
    return ok;
}

bool save_blob(const char* key, const void* data, size_t size)
{
    nvs_handle_t h;
    if (nvs_open("console", NVS_READWRITE, &h) != ESP_OK) return false;
    const bool ok = nvs_set_blob(h, key, data, size) == ESP_OK && nvs_commit(h) == ESP_OK;
    nvs_close(h);
    if (!ok) CONSOLE_LOGW(TAG, "zapis %s do NVS nie powiodl sie", key);
    return ok;
}

void erase_blob(const char* key)
{
    nvs_handle_t h;
    if (nvs_open("console", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_erase_key(h, key);
    nvs_commit(h);
    nvs_close(h);
}

void set_brightness(int percent)
{
    board::display::set_backlight_level(percent);
}

MemInfo memory_info()
{
    MemInfo m;
    m.sram_free   = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    m.sram_total  = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    m.psram_free  = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    m.psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    return m;
}

}  // namespace platform
