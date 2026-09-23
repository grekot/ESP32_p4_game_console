# Glowny komponent aplikacji (PlatformIO rejestruje katalog src/ jako komponent ESP-IDF).
# Wszystkie pliki .c/.cpp w src/ (rekurencyjnie) sa kompilowane - nowa gra = nowy katalog w src/games/.
# Po dodaniu NOWEGO pliku zrob `pio run --target clean` (albo dotknij ten plik), zeby CMake odswiezyl liste.
# (CONFIGURE_DEPENDS jest niedozwolone - ESP-IDF wykonuje ten plik takze w trybie skryptu.)

file(GLOB_RECURSE LAKE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

idf_component_register(
    SRCS ${LAKE_SOURCES}
    INCLUDE_DIRS "." "ui"
    REQUIRES
        esp_lcd          # MIPI-DSI / DPI panel, panel IO
        esp_driver_gpio
        esp_driver_i2c   # magistrala I2C dla dotyku GT911
        esp_adc          # galka analogowa (ADC2)
        esp_driver_ppa   # PPA - sprzetowe skalowanie/obrot bufora ramki
        esp_mm           # esp_cache_msync
        esp_psram
        esp_timer
        nvs_flash
        lvgl__lvgl       # LVGL (menu konsoli, nakladki UI w grach)
)
