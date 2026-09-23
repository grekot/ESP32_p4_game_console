// Mapa pinow plytki Guition JC4880P443C_I_W_Y (modul JC-ESP32P4-M3 + ESP32-C6)
// Zrodlo: schemat producenta + notatki spolecznosci (docs/HARDWARE.md)
#pragma once

#include "driver/gpio.h"

namespace board::pins {

// --- Wyswietlacz 4.3" 480x800, ST7701S, MIPI-DSI 2 linie ---
constexpr gpio_num_t LCD_RESET      = GPIO_NUM_5;
constexpr gpio_num_t LCD_BACKLIGHT  = GPIO_NUM_23;   // zwykly GPIO, aktywny stan wysoki (nie PWM)
constexpr int        MIPI_LDO_CHAN  = 3;             // wewnetrzny LDO zasilajacy DSI-PHY
constexpr int        MIPI_LDO_MV    = 2500;

// --- Dotyk pojemnosciowy GT911 (I2C, adres 0x5D; magistrala wspolna z kodekiem audio) ---
constexpr gpio_num_t TOUCH_SDA      = GPIO_NUM_7;
constexpr gpio_num_t TOUCH_SCL      = GPIO_NUM_8;
constexpr gpio_num_t TOUCH_RST      = GPIO_NUM_NC;   // fizycznie GPIO3, ale nie jest potrzebny
constexpr gpio_num_t TOUCH_INT      = GPIO_NUM_NC;

// --- Przyciski / LED ---
constexpr gpio_num_t BTN_BOOT       = GPIO_NUM_35;   // przycisk BOOT (stan niski = wcisniety)
constexpr gpio_num_t LED_BUILTIN    = GPIO_NUM_26;   // wspolny z RS485 TX

// --- Audio: kodek ES8311 (I2S) + wzmacniacz NS4150 ---  (na przyszlosc)
constexpr gpio_num_t I2S_MCLK       = GPIO_NUM_13;
constexpr gpio_num_t I2S_BCLK       = GPIO_NUM_12;
constexpr gpio_num_t I2S_WS         = GPIO_NUM_10;
constexpr gpio_num_t I2S_DOUT       = GPIO_NUM_9;    // do kodeka (glosnik)
constexpr gpio_num_t I2S_DIN        = GPIO_NUM_48;   // z kodeka (mikrofon)
constexpr gpio_num_t AUDIO_PA_EN    = GPIO_NUM_11;
constexpr uint8_t    ES8311_ADDR7   = 0x18;

// --- Karta microSD: SDMMC slot 0, 4-bit; zasilanie z wewnetrznego LDO kanal 4 (3.3 V) ---
constexpr gpio_num_t SD_CLK         = GPIO_NUM_43;
constexpr gpio_num_t SD_CMD         = GPIO_NUM_44;
constexpr gpio_num_t SD_D0          = GPIO_NUM_39;
constexpr gpio_num_t SD_D1          = GPIO_NUM_40;
constexpr gpio_num_t SD_D2          = GPIO_NUM_41;
constexpr gpio_num_t SD_D3          = GPIO_NUM_42;
constexpr int        SD_LDO_CHAN    = 4;

// --- ESP32-C6 (Wi-Fi 6 / BLE) przez SDIO slot 1, protokol ESP-Hosted ---
constexpr gpio_num_t C6_SDIO_CLK    = GPIO_NUM_18;
constexpr gpio_num_t C6_SDIO_CMD    = GPIO_NUM_19;
constexpr gpio_num_t C6_SDIO_D0     = GPIO_NUM_14;
constexpr gpio_num_t C6_SDIO_D1     = GPIO_NUM_15;
constexpr gpio_num_t C6_SDIO_D2     = GPIO_NUM_16;
constexpr gpio_num_t C6_SDIO_D3     = GPIO_NUM_17;
constexpr gpio_num_t C6_RESET       = GPIO_NUM_54;

// --- Kontroler na zlaczu rozszerzen JP1 (uklad w stylu pada Switch) ---
//
// Krzyzak mechaniczny + galka analogowa + cztery przyciski akcji + START.
// Przelaczniki: kazdy laczy swoj GPIO z masa, podciaganie wewnetrzne, stan niski = wcisniety.
// Bezposrednio, bez matrycy i bez diod, wiec dowolna kombinacja dziala.
// Galka: dwa potencjometry na wejsciach ADC. Uklad i polaczenia: docs/HARDWARE.md.
//
// UWAGA na przydzial pinow: z calego zlacza TYLKO GPIO49-52 maja przetwornik ADC, wiec osie
// galki musza siedziec na dwoch z nich. Pozostale klawisze dostaja piny czysto cyfrowe.

// Galka analogowa (ADC2). Zasilanie modulu z 3V3, NIE z 5V - wejscia analogowe znosza max 3,3 V.
constexpr gpio_num_t STICK_X    = GPIO_NUM_49;   // JP1 pin 13, ADC2 kanal 0
constexpr gpio_num_t STICK_Y    = GPIO_NUM_50;   // JP1 pin 11, ADC2 kanal 1

// Krzyzak
constexpr gpio_num_t KEY_UP     = GPIO_NUM_52;   // JP1 pin 7
constexpr gpio_num_t KEY_DOWN   = GPIO_NUM_51;   // JP1 pin 9
constexpr gpio_num_t KEY_LEFT   = GPIO_NUM_34;   // JP1 pin 17
constexpr gpio_num_t KEY_RIGHT  = GPIO_NUM_33;   // JP1 pin 8

// Przyciski akcji (romb jak w padzie)
constexpr gpio_num_t KEY_A      = GPIO_NUM_32;   // JP1 pin 19
constexpr gpio_num_t KEY_B      = GPIO_NUM_31;   // JP1 pin 10
constexpr gpio_num_t KEY_X      = GPIO_NUM_30;   // JP1 pin 12
constexpr gpio_num_t KEY_Y      = GPIO_NUM_29;   // JP1 pin 14

// Funkcyjne
constexpr gpio_num_t KEY_START  = GPIO_NUM_28;   // JP1 pin 21

// SELECT nie jest podlaczony: na JP1 zostaje 11 uzywalnych pinow, a pelny uklad potrzebowalby 12.
// W menu role "wstecz" pelni B, wiec brak SELECT nic nie blokuje. Kod go obsluguje, wiec wystarczy
// wpisac tu numer pinu, jesli zwolnisz jakis albo dolozysz ekspander I2C (docs/HARDWARE.md).
constexpr gpio_num_t KEY_SELECT = GPIO_NUM_NC;

// GPIO35 to przycisk BOOT na plytce - NIE uzywac go jako klawisza gry: przytrzymany przy
// starcie wprowadza uklad w tryb programowania. Sluzy tylko jako zapasowy START.

}  // namespace board::pins
