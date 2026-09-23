// Console - proste API do pisania gier na konsole (lekcje w src/games/lekcje/).
//
// Gra to dwie funkcje: setup() wykonuje sie raz na starcie, frame() 60 razy na sekunde.
// Wszystko rysuje sie na plotnie 800x480 pikseli; (0,0) to lewy gorny rog, x rosnie w prawo,
// y rosnie W DOL. Plotno nie czysci sie samo - frame() zwykle zaczyna od clear(kolor).
//
// Ten naglowek jest wlaczany przez console/console.h (ktory dodaje "using namespace console" i makro CONSOLE_ADD_GAME).
// Nazwy po angielsku, jak w kazdym kursie C++; komentarze po polsku.
#pragma once

#include <stdint.h>

#include "engine/screen.h"
#include "gfx/canvas.h"
#include "gfx/palette.h"
#include "input/keys.h"

namespace console {

// ------------------------------------------------------------------ kolory
// Silny typ: rect(x, y, RED, 5, 5) to blad kompilacji, a nie dziwny obrazek.
struct Color {
    uint16_t raw;
};

constexpr Color rgb(int r, int g, int b)
{
    return Color{ gfx::rgb565((uint8_t)r, (uint8_t)g, (uint8_t)b) };
}

// Te same wartosci, co w palecie konsoli (gfx/palette.h) - obrazki z liter i figury maja identyczne kolory.
constexpr Color BLACK       = Color{ gfx::pal::BLACK };
constexpr Color WHITE       = Color{ gfx::pal::WHITE };
constexpr Color GRAY        = Color{ gfx::pal::GRAY };
constexpr Color DARK_GRAY   = Color{ gfx::pal::DARK_GRAY };
constexpr Color RED         = Color{ gfx::pal::RED };
constexpr Color DARK_RED    = Color{ gfx::pal::DARK_RED };
constexpr Color ORANGE      = Color{ gfx::pal::ORANGE };
constexpr Color YELLOW      = Color{ gfx::pal::YELLOW };
constexpr Color DARK_YELLOW = Color{ gfx::pal::DARK_YELLOW };
constexpr Color GREEN       = Color{ gfx::pal::GREEN };
constexpr Color DARK_GREEN  = Color{ gfx::pal::DARK_GREEN };
constexpr Color BROWN       = Color{ gfx::pal::BROWN };
constexpr Color DARK_BROWN  = Color{ gfx::pal::DARK_BROWN };
constexpr Color BEIGE       = Color{ gfx::pal::BEIGE };
constexpr Color SKIN        = Color{ gfx::pal::SKIN };
constexpr Color BLUE        = Color{ gfx::pal::BLUE };
constexpr Color DARK_BLUE   = Color{ gfx::pal::DARK_BLUE };
constexpr Color PURPLE      = Color{ gfx::pal::PURPLE };
constexpr Color DARK_PURPLE = Color{ gfx::pal::DARK_PURPLE };
constexpr Color SKY         = Color{ gfx::pal::SKY };

// ------------------------------------------------------------------ ekran
constexpr int screen_width()  { return engine::CANVAS_W; }   // 800
constexpr int screen_height() { return engine::CANVAS_H; }   // 480

void clear(Color c);                                              // cale plotno jednym kolorem
void pixel(int x, int y, Color c);
void rect(int x, int y, int w, int h, Color c);                   // prostokat wypelniony
void rect_outline(int x, int y, int w, int h, Color c);           // sam obrys (1 px)
void circle(int cx, int cy, int r, Color c);                      // kolo wypelnione
void circle_outline(int cx, int cy, int r, Color c);              // sam okrag (1 px)
void line(int x0, int y0, int x1, int y1, Color c);

// Napis wygladzana czcionka (ta sama, co w menu konsoli). scale = rozmiar: 1 maly (16 px), 2 sredni (24 px),
// 3 duzy (32 px), 4 wielki (48 px). (x, y) to lewy gorny rog napisu. Litery, cyfry, znaki - bez polskich liter.
void text(int x, int y, const char* s, Color c, int scale = 1);
void text(int x, int y, int number, Color c, int scale = 1);      // liczba, np. wynik
int  text_width(const char* s, int scale = 1);                    // szerokosc napisu - do wysrodkowania
int  text_height(int scale = 1);                                  // wysokosc linii napisu

// ------------------------------------------------------------------ sprite'y (obrazki)
// Obrazek rysuje sie znakami: kazdy wiersz to jeden rzad pikseli, litera = kolor z palety
// (k czern, w biel, e szary, E ciemnoszary, r czerwien, R ciemna czerwien, o pomarancz,
//  y zolty, Y ciemny zolty, g zielen, G ciemna zielen, b braz, B ciemny braz, t bez,
//  s skora, u niebieski, U ciemny niebieski, p fiolet, P ciemny fiolet), '.' = przezroczyste.
// Szerokosc = najdluzszy wiersz. Wolaj w setup(), nie w frame().
using gfx::Sprite;

namespace detail {
Sprite build_sprite(const char* const* rows, int h);
}

template <int H>
Sprite load_sprite(const char* const (&rows)[H])
{
    return detail::build_sprite(rows, H);
}

// Obrazek z pliku PNG. Pliki gry leza w katalogu assets/<id>/ (id z CONSOLE_ADD_GAME), np. gra `bohater`
// woła load_image("hero.png") i dostaje assets/bohater/hero.png. Nazwa ze znakiem '/' liczy sie od katalogu
// assets/ ("wspolne/moneta.png"). Przezroczystosc z kanalu alfa PNG (twarde krawedzie, bez polprzezroczystosci).
// Na plytce pliki sa na partycji assets (wgrywanie: pio run -t uploadfs). Wolaj w setup(); brak pliku = pusty
// obrazek (nic sie nie rysuje) i komunikat w terminalu.
Sprite load_image(const char* filename);

// flip_x = odbicie lustrzane; scale = powiekszenie (2 albo 3 - na ekranie 800x480 obrazek 16x16 jest maly).
// Rozmiar na ekranie: s.w * scale, s.h * scale.
void sprite(const Sprite& s, int x, int y, bool flip_x = false, int scale = 1);

// ------------------------------------------------------------------ wejscie
// Klawisze konsoli. START i SELECT naleza do konsoli (pauza) - gra ich nie widzi.
using input::Key;
constexpr Key UP    = Key::Up;
constexpr Key DOWN  = Key::Down;
constexpr Key LEFT  = Key::Left;
constexpr Key RIGHT = Key::Right;
constexpr Key A     = Key::A;
constexpr Key B     = Key::B;
constexpr Key X     = Key::X;
constexpr Key Y     = Key::Y;

bool held(Key k);       // trzymany w tej klatce
bool pressed(Key k);    // wcisniety WLASNIE w tej klatce (raz na nacisniecie)
bool released(Key k);   // puszczony wlasnie w tej klatce

// ------------------------------------------------------------------ czas i losowosc
int   frame_count();    // numer klatki od setup() (0, 1, 2, ...)
float seconds();        // sekundy od setup()
float dt();             // czas jednej klatki w sekundach (~0.0167)

int   random(int a, int b);        // losowa liczba od a do b (wlacznie)
bool  chance(int percent);         // true z prawdopodobienstwem percent %
void  random_seed(uint32_t seed);  // to samo ziarno = ten sam ciag liczb

// ------------------------------------------------------------------ kolizje i matematyka
struct Rect {
    int x, y, w, h;
};

bool overlaps(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh);   // czy prostokaty nachodza
bool overlaps(Rect a, Rect b);
int  clamp(int v, int lo, int hi);   // przycina v do zakresu lo..hi
int  abs(int v);
int  min(int a, int b);
int  max(int a, int b);

// ------------------------------------------------------------------ mapa kafelkow (lekcja 12)
// Poziom rysuje sie literami jak sprite: kazdy wiersz to rzad kafelkow 32x32 px (15 wierszy = caly ekran
// w pionie, 25 kolumn = ekran w poziomie), kazdy znak to jeden kafelek. Ktore znaki sa "stale" (sciana,
// podloga), mowi napis solid_chars, np. "#=B". Reszta to tlo, monety, dekoracje.
constexpr int TILE = 32;

namespace detail {
void load_map_rows(const char* const* rows, int h, const char* solid_chars);
}

template <int H>
void load_map(const char* const (&rows)[H], const char* solid_chars)
{
    detail::load_map_rows(rows, H, solid_chars);
}

int  map_cols();                             // szerokosc mapy w kafelkach
int  map_rows();                             // wysokosc mapy w kafelkach
int  map_width();                            // szerokosc w pikselach (map_cols() * TILE)
char map_tile(int col, int row);             // znak kafelka; ' ' poza mapa
void map_set(int col, int row, char tile);   // zmiana kafelka (np. zebrana moneta -> ' ')
bool map_solid(int col, int row);            // czy kafelek jest staly (kolumny poza mapa = tak)
bool map_solid_at(int px, int py);           // to samo dla punktu w pikselach
int  map_col(int px);                        // numer kolumny dla piksela
int  map_row(int py);                        // numer wiersza dla piksela

// Rysuje sprite na kazdym kafelku o podanym znaku (cam_x = przesuniecie kamery w pikselach).
// Sprite mniejszy niz kafelek (np. 16x16) jest powiekszany do rozmiaru kafelka.
void draw_tiles(char tile, const Sprite& s, int cam_x);
// To samo prostokatem w kolorze - wystarcza na start, bez rysowania sprite'ow.
void draw_tiles(char tile, Color c, int cam_x);

// Ruch prostokata (x, y, w, h) o (vx, vy) pikseli z kolizjami z mapa: zatrzymuje na scianach, stawia na
// podlozu, zatrzymuje pod sufitem. Funkcja ZMIENIA x, y, vx, vy (dlatego &): po scianie vx = 0, po ladowaniu vy = 0.
struct MoveResult {
    bool on_ground;   // stoi na stalym kafelku
    bool hit_wall;    // uderzyl w sciane z boku
    bool hit_head;    // uderzyl glowa w kafelek (head_col, head_row) - np. blok "?" do rozbicia
    int  head_col;
    int  head_row;
};
MoveResult move_box(int& x, int& y, int w, int h, int& vx, int& vy);

// ------------------------------------------------------------------ sterowanie i diagnostyka
void restart();                                 // od nastepnej klatki gra zaczyna od setup()

// Pokazuje wartosc w rogu ekranu i w sladzie emulatora (--trace). Max 8 na klatke.
void watch(const char* name, int value);
void watch(const char* name, float value);
void watch(const char* name, double value);
void watch(const char* name, bool value);
void watch(const char* name, const char* value);

// Pisze do terminala (emulator) albo logu (plytka). Uwaga: w frame() to 60 linii na sekunde.
void print(const char* s);
void print(const char* label, int value);
void print(const char* label, float value);
void print(const char* label, const char* value);

}  // namespace console
