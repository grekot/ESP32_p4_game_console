// Zestaw klawiszy konsoli - wspolny dla plytki (klawiatura mechaniczna) i emulatora (klawiatura PC).
//
// Uklad 10 klawiszy (klasyka: krzyzak + cztery akcje + dwa systemowe):
//
//   [galka]                                       [ X ]
//        [ UP ]                                [ Y ]   [ A ]
//   [LEFT] [RIGHT]                                 [ B ]
//        [DOWN]              [START]
//
// A = skok, B = bieg/akcja, X i Y sa wolne dla przyszlych gier.
// Galka analogowa dubluje krzyzak (po progowaniu) i daje grom plynne osie.
// SELECT nie jest podlaczony na plytce - zabraklo pinu; w menu cofa B (docs/HARDWARE.md).
#pragma once

#include <stdint.h>

namespace input {

enum class Key : uint8_t {
    Up = 0,
    Down,
    Left,
    Right,
    A,
    B,
    X,
    Y,
    Start,
    Select,
    // Kierunki galki analogowej. Na plytce pochodza z potencjometrow, w emulatorze z pada USB
    // albo z osobnych klawiszy PC (patrz keymap.cfg) - dlatego maja swoje nazwy w mapowaniu.
    StickUp,
    StickDown,
    StickLeft,
    StickRight,
    Count
};

constexpr int KEY_COUNT = (int)Key::Count;

inline constexpr uint16_t key_bit(Key k) { return (uint16_t)(1u << (int)k); }

// Nazwa klawisza, np. "UP", "START" - uzywana w pliku mapowania emulatora i w logach.
const char* key_name(Key k);

// Zamienia nazwe na klawisz (bez rozrozniania wielkosci liter). false = nieznana nazwa.
bool key_from_name(const char* name, Key& out);

}  // namespace input
