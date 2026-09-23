# Sprzęt: Guition JC4880P443C_I_W_Y

Płytka kupiona na AliExpress (GUITION Official Store, model `JC4880P443C_I_W_Y`):
moduł **JC-ESP32P4-M3** = ESP32-P4 + ESP32-C6, ekran IPS 4,3" 480x800 z dotykiem pojemnościowym,
obudowa, gniazdo kamery.

| Element | Szczegóły |
|---|---|
| MCU główny | ESP32-P4, 2x RISC-V, rev <3.0 (moduł ES) - taktowanie **360 MHz** (400 MHz tylko dla rev >=3.0 albo chipów kwalifikowanych przez Espressif) |
| Radio | ESP32-C6 (Wi-Fi 6, BLE 5) jako koprocesor przez SDIO (ESP-Hosted) |
| Flash | 16 MB |
| PSRAM | 32 MB (HEX, 200 MHz) |
| Ekran | 4,3" IPS 480x800 (pion), sterownik **ST7701S**, **MIPI-DSI 2 linie @ 500 Mbps**, DPI 34 MHz (~60 Hz) |
| Dotyk | Goodix **GT911**, I2C 0x5D (zapasowo 0x14), do 5 punktów |
| Audio | kodek ES8311 (I2C 0x18) + wzmacniacz NS4150, głośnik, mikrofon analogowy |
| Karta | microSD, SDMMC 4-bit (slot 0) |
| USB | 2x USB-C: natywny USB-Serial/JTAG ESP32-P4 (flashowanie + logi) oraz USB-UART (UART0) |
| Zasilanie | 5 V USB, buck TLV62569 3V3, IP5306 (obsługa akumulatora) |
| Inne | RS485 (MAX485), złącze rozszerzeń JP1, przycisk BOOT |

## Pinout (ESP32-P4)

Wszystkie piny są zebrane w [src/board/pins.h](../src/board/pins.h).

| Funkcja | GPIO | Uwagi |
|---|---|---|
| LCD reset | 5 | impuls: 20 ms niski / 120 ms wysoki |
| LCD podświetlenie | 23 | **zwykły GPIO, stan wysoki = włączone** (nie PWM) |
| DSI-PHY zasilanie | wewnętrzny LDO **kanał 3**, 2500 mV | musi być włączony przed utworzeniem magistrali DSI |
| Dotyk SDA / SCL | 7 / 8 | magistrala wspólna z kodekiem ES8311 |
| Dotyk RST | 3 | niepotrzebny (sterownik działa z `GPIO_NUM_NC`) |
| Przycisk BOOT | 35 | stan niski = wciśnięty |
| LED | 26 | wspólny z RS485 TX |
| UART0 (konsola) | TX 37 / RX 38 | mostek na drugie USB-C |
| I2S MCLK / BCLK / WS | 13 / 12 / 10 | ES8311 |
| I2S DOUT (głośnik) / DIN (mikrofon) | 9 / 48 | |
| Wzmacniacz PA enable | 11 | |
| SD CLK / CMD | 43 / 44 | SDMMC **slot 0** (piny IOMUX, stałe) |
| SD D0..D3 | 39 / 40 / 41 / 42 | pull-upy 5,1 kΩ na płytce |
| SD zasilanie | wewnętrzny LDO **kanał 4**, 3300 mV | bez tego karta nie ma zasilania (GPIO45 jest niepodłączony) |
| C6 SDIO CLK / CMD | 18 / 19 | SDMMC **slot 1** |
| C6 SDIO D0..D3 | 14 / 15 / 16 / 17 | 4-bit @ 40 MHz |
| C6 reset | 54 | aktywny stan wysoki |
| JP1 wolne GPIO | 52, 51, 50, 49, 34, 33, 32, 31, 30, 29, 28 (+35 = BOOT) | kontroler - patrz nizej. ADC tylko na 49-52 |
| JP1 C6 UART0 | `C6_U0RXD` / `C6_U0TXD`, `C6_IO9` (BOOT), `C6_CHIP_PU` | do flashowania firmware C6 |

## Kontroler: krzyżak, gałka analogowa i przyciski

Układ wzorowany na padzie Switch: **mechaniczny krzyżak, jedna gałka analogowa i pięć przycisków**.
Wszystko na złączu rozszerzeń JP1.

### Układ

```
   ╭───────╮          ┌─────────┐                             ┌─────────┐
   │ gałka │          │   UP    │                             │    X    │
   │  ana- │       ┌──┴──┐   ┌──┴──┐                      ┌───┴─┐   ┌───┴─┐
   │ logowa│       │LEFT │   │RIGHT│                      │  Y  │   │  A  │
   ╰───────╯       └──┬──┘   └──┬──┘                      └───┬─┘   └───┬─┘
                      │  DOWN   │                             │    B    │
                      └─────────┘                             └─────────┘

                              ┌───────┐
                              │ START │
                              └───────┘
```

- **Krzyżak** to podstawa sterowania w platformówce. Cztery osobne przełączniki, przy rozstawie
  19 mm (standard MX) blok zajmuje 57x57 mm.
- **Gałka** dubluje krzyżak: firmware proguje jej wychylenie na te same kierunki, więc każda gra
  działa z obu. Dodatkowo wystawia surowe osie w zakresie -1..1 dla gier, które zechcą płynnego
  ruchu (strzelanki z góry, wyścigi, kursor).
- **A** skok, **B** bieg i akcja. **X** i **Y** czekają na kolejne gry.
- **START** pauza w grze i zatwierdzanie w menu.

### Czego brakuje i dlaczego

Pełny pad w tym stylu potrzebowałby 12 wejść, a na JP1 jest ich 11. Odpadł **SELECT**: w menu rolę
„wstecz" pełni już B, więc jego brak nic nie blokuje. Przycisk pod gałką (SW) też zostaje
niepodłączony. Kod obsługuje SELECT normalnie, wystarczy wpisać numer pinu w
[src/board/pins.h](../src/board/pins.h), jeśli kiedyś zwolnisz jakiś pin.

Jeśli zechcesz pełny zestaw razem z przyciskami na ramionach: na JP1 wyprowadzona jest magistrala
I2C (piny 23 i 25). Ekspander typu PCF8574 kosztuje kilka złotych i daje 8 dodatkowych wejść
bez zajmowania choćby jednego GPIO. Adres 0x20 nie koliduje z dotykiem (0x5D) ani z kodekiem (0x18).

### Podłączenie

Przełączniki: każdy łączy swój pin z masą. Podciąganie jest wewnętrzne, stan niski to wciśnięcie.
Nie trzeba żadnych rezystorów ani diod, a podłączenie bezpośrednie znosi problem maskowania
klawiszy, więc kombinacja w prawo plus bieg plus skok działa poprawnie.

| Element | GPIO | pin JP1 | | Element | GPIO | pin JP1 |
|---|---|---|---|---|---|---|
| gałka oś X | 49 | 13 | | A | 32 | 19 |
| gałka oś Y | 50 | 11 | | B | 31 | 10 |
| UP | 52 | 7 | | X | 30 | 12 |
| DOWN | 51 | 9 | | Y | 29 | 14 |
| LEFT | 34 | 17 | | START | 28 | 21 |
| RIGHT | 33 | 8 | | | | |

Gałkę zasil z **3,3 V** (pin 1, 3 albo 18), **nie z 5 V**: wejścia analogowe znoszą maksymalnie
3,3 V i wyższe napięcie je uszkodzi. Masa: pin 5, 6 albo 16.

Przydział osi nie jest dowolny. Z całego złącza tylko **GPIO49-52** mają przetwornik analogowy
(kanały ADC2), więc gałka musi zająć dwa z nich. Pozostałe dwa zostały krzyżakowi jako zwykłe
wejścia cyfrowe. Na ESP32-P4 nie ma klasycznego konfliktu ADC2 z Wi-Fi, bo radio siedzi w osobnym
układzie.

### Pełny rozkład złącza JP1 (2x13, 26 pinów)

Odczytane ze schematu producenta, arkusz `03-expand-io`. Numeracja: nieparzyste po lewej,
parzyste po prawej.

| pin | sygnał | | pin | sygnał |
|---|---|---|---|---|
| 1 | VCC3V3 | | 2 | VCC5V |
| 3 | VCC3V3 | | 4 | VCC5V |
| 5 | GND | | 6 | GND |
| 7 | **GPIO52** UP | | 8 | **GPIO33** RIGHT |
| 9 | **GPIO51** DOWN | | 10 | **GPIO31** B |
| 11 | **GPIO50** gałka Y | | 12 | **GPIO30** X |
| 13 | **GPIO49** gałka X | | 14 | **GPIO29** Y |
| 15 | GPIO35 (BOOT) | | 16 | GND |
| 17 | **GPIO34** LEFT | | 18 | VCC3V3 |
| 19 | **GPIO32** A | | 20 | C6_U0RXD |
| 21 | **GPIO28** START | | 22 | C6_U0TXD |
| 23 | ES_I2C_SDA | | 24 | C6_IO9 |
| 25 | ES_I2C_SCL | | 26 | C6_CHIP_PU |

Piny 20-26 służą do wgrywania firmware do ESP32-C6, a 23 i 25 to magistrala I2C wspólna z dotykiem
i kodekiem audio. **Nie podłączaj do GPIO35 klawisza gry** — przytrzymany przy starcie wprowadza
układ w tryb programowania i konsola wygląda na zepsutą. Firmware używa go jako zapasowego STARTU,
żeby dało się sterować konsolą jeszcze przed zbudowaniem kontrolera.

### Odkłócanie i kalibracja

Styki przełącznika mechanicznego drgają przez 1-5 ms. Firmware odpytuje piny co 1 ms w osobnym
zadaniu i uznaje zmianę dopiero po 8 jednakowych próbkach ([src/board/keypad.cpp](../src/board/keypad.cpp)).
Jedno naciśnięcie nie zamieni się w dwa skoki, niezależnie od długości klatki.

Gałki z tych tanich modułów rzadko trafiają dokładnie w środek zakresu i każdy egzemplarz jest
inny, więc firmware **mierzy położenie spoczynkowe przy starcie** i od niego liczy wychylenie
([src/board/joystick.cpp](../src/board/joystick.cpp)). Dlatego **nie dotykaj gałki podczas
włączania konsoli**. Gdyby zmierzony środek odjechał o więcej niż ćwierć zakresu, firmware uzna
pomiar za błędny, weźmie połowę zakresu i zapisze ostrzeżenie w logu. Strefa martwa to 25 procent,
a próg zamiany wychylenia na kierunek cyfrowy to 50 procent.

## Jak ten projekt używa ekranu

1. LDO kanał 3 → 2,5 V (zasilanie PHY).
2. `esp_lcd_new_dsi_bus` (2 linie, 500 Mbps) → `esp_lcd_new_panel_io_dbi` → `esp_lcd_new_panel_st7701`
   ze sterownikiem producenta w [src/board/st7701/](../src/board/st7701/) (sekwencja inicjalizacyjna
   ST7701 z pakietu SDK Guition; komponent `esp_lcd_st7701` z rejestru daje na tej płytce czarny ekran).
3. Panel DPI 480x800 RGB565 @ 34 MHz z **dwoma buforami ramki** w PSRAM (`num_fbs = 2`).
4. Gra rysuje do własnego płótna 400x240 (SRAM). `display::present()` skaluje x2 i obraca o 90° sprzętowo
   przez **PPA** (Pixel Processing Accelerator) wprost do tylnego bufora DPI, po czym
   `esp_lcd_panel_draw_bitmap` z adresem tego bufora przełącza bufory bez kopiowania. Czekamy na
   zdarzenie `on_frame_buf_complete` (koniec wysyłania ramki) - stąd naturalne 60 FPS.

Orientacja: panel jest pionowy, gra działa poziomo. `LAKE_DISPLAY_ROTATION` (90/270) w `platformio.ini`
steruje jednocześnie obrotem obrazu i mapowaniem współrzędnych dotyku.

## Do zweryfikowania na sprzęcie (płytka jeszcze nie dotarła)

- Czy zdarzenie końca ramki odpala się co klatkę (licznik `display::vsync_timeouts()` powinien stać na 0).
- Kierunek obrotu PPA vs. położenie złączy USB (jeśli obraz do góry nogami: `LAKE_DISPLAY_ROTATION=270`).
- Czy dotyk pokrywa się z obrazem (jeśli lustrzany: sprawdź `flags.mirror_x/mirror_y` w `touch.cpp`).
- Rewizja krzemu: `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` jest kluczowe; zły wybór = crash w bootloaderze.

## Wi-Fi (nieużywane w grze)

Firmware ESP-Hosted na C6 jest fabrycznie stare (2.3.0) i nie współpracuje z aktualnym hostem (2.12.x) -
Wi-Fi łączy się i zrywa. Naprawa: wgranie `network_adapter_esp32c6.bin` 2.12.x przez UART C6 na JP1
(P4 trzeba wcześniej zatrzymać w trybie download, bo inaczej resetuje C6 przez GPIO54 podczas flashowania).
Szczegóły: notatki społeczności poniżej.

## Źródła

- [ultramcu/guition-jc4880p443c-i-w](https://github.com/ultramcu/guition-jc4880p443c-i-w) - pinout, schemat, notatki (Wi-Fi, ekran, SD)
- [ageiron/jc4880p443c-template](https://github.com/ageiron/jc4880p443c-template) - szablon PlatformIO/ESP-IDF, `docs/BRINGUP.md`, sterownik ST7701
- [bigbag/JC4880P443C-examples](https://github.com/bigbag/JC4880P443C-examples) - 12 przykładów (LVGL, Wi-Fi, SD, BLE, audio)
- [CNX Software: opis płytki](https://www.cnx-software.com/2025/08/12/4-3-inch-touch-display-board-features-single-esp32-p4-esp32-c6-module-supports-camera-and-speakers/)
- SDK producenta (schematy, dema, ~385 MB): `https://pan.jczn1688.com/directlink/1/HMI%20display/JC4880P443C_I_W.zip`
- [pioarduino/platform-espressif32](https://github.com/pioarduino/platform-espressif32) - platforma PlatformIO z obsługą ESP32-P4
