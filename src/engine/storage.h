// Trwaly zapis malych danych gier i konsoli (rekordy, ustawienia) - nad platform::load_blob/save_blob.
//
// W trybie powtarzalnym (testy --frames, engine::deterministic()) zapis i odczyt sa wylaczone: kazdy test startuje
// z wartosci domyslnych, niezaleznie od tego, co zostalo po poprzedniej grze na tym komputerze.
// Klucz: do 15 znakow [a-z0-9_], np. "snake_top". Struktura zapisu powinna miec pole wersji - po zmianie ukladu
// stary zapis ma inny rozmiar i jest ignorowany (load zwraca false).
#pragma once

#include <stddef.h>

namespace engine {

bool load_data(const char* key, void* data, size_t size);
bool save_data(const char* key, const void* data, size_t size);
void erase_data(const char* key);

// Klucze rekordow gier (ekran Ustawienia -> "Wyczysc rekordy"). Nowa gra z rekordami dopisuje tu swoj klucz.
extern const char* const RECORD_KEYS[];
extern const int         RECORD_KEY_COUNT;

}  // namespace engine
