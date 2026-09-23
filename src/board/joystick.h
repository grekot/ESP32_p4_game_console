// Galka analogowa (popularny modul dwuosiowy z potencjometrami) na wejsciach ADC2.
//
// Zwraca wychylenie w zakresie -1..1 dla kazdej osi, po odjeciu srodka i strefie martwej.
// Srodek jest mierzony przy starcie, bo te modulki maja spory rozrzut i rzadko trafiaja
// dokladnie w polowe zakresu.
#pragma once

#include "esp_err.h"

namespace board::joystick {

struct Axes {
    float x = 0.f;   // -1 = maksymalnie w lewo, +1 = w prawo
    float y = 0.f;   // -1 = maksymalnie w gore,  +1 = w dol
};

// Konfiguruje ADC i mierzy polozenie spoczynkowe galki. Galki nie wolno wtedy dotykac.
esp_err_t init();

// Aktualne wychylenie. Gdy init() sie nie powiodl albo galka nie jest podlaczona: zera.
Axes read();

bool available();

}  // namespace board::joystick
