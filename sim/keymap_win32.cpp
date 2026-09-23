// Emulator: mapowanie klawiszy konsoli na klawisze klawiatury PC.
//
// Domyslne mapowanie jest wbudowane, a plik keymap.cfg (jesli istnieje) je nadpisuje.
// Dzieki temu emulator odwzorowuje klawiature mechaniczna konsoli 1:1, a kazdy moze
// ustawic sobie wygodne klawisze.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "keymap_win32.h"

namespace sim {

namespace {

const char* TAG = "keymap";

struct NamedVk {
    const char* name;
    int         vk;
};

// Nazwy klawiszy PC rozpoznawane w keymap.cfg (poza literami i cyframi, obslugiwanymi osobno).
const NamedVk NAMED[] = {
    {"Up", VK_UP}, {"Down", VK_DOWN}, {"Left", VK_LEFT}, {"Right", VK_RIGHT},
    {"Enter", VK_RETURN}, {"Return", VK_RETURN}, {"Space", VK_SPACE}, {"Tab", VK_TAB},
    {"Esc", VK_ESCAPE}, {"Escape", VK_ESCAPE}, {"Backspace", VK_BACK},
    {"Shift", VK_SHIFT}, {"Ctrl", VK_CONTROL}, {"Control", VK_CONTROL}, {"Alt", VK_MENU},
    {"Comma", VK_OEM_COMMA}, {"Period", VK_OEM_PERIOD}, {"Slash", VK_OEM_2},
    {"Semicolon", VK_OEM_1}, {"Minus", VK_OEM_MINUS}, {"Plus", VK_OEM_PLUS},
    {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4}, {"F5", VK_F5}, {"F6", VK_F6},
    {"F7", VK_F7}, {"F8", VK_F8}, {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},
};

// Domyslne mapowanie - kolejnosc wg input::Key.
const int DEFAULT_VK[input::KEY_COUNT] = {
    VK_UP,      // UP
    VK_DOWN,    // DOWN
    VK_LEFT,    // LEFT
    VK_RIGHT,   // RIGHT
    'Z',        // A  - skok
    'X',        // B  - bieg
    'S',        // X
    'A',        // Y
    VK_RETURN,  // START
    VK_BACK,    // SELECT
    'I',        // STICK_UP     - galka analogowa osobno od krzyzaka
    'K',        // STICK_DOWN
    'J',        // STICK_LEFT
    'L',        // STICK_RIGHT
};
static_assert(sizeof(DEFAULT_VK) / sizeof(DEFAULT_VK[0]) == (size_t)input::KEY_COUNT,
              "domyslne mapowanie musi obejmowac wszystkie klawisze z input::Key");

char upper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c; }

bool same_name(const char* a, const char* b)
{
    while (*a && *b) {
        if (upper(*a) != upper(*b)) return false;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

// Zamienia nazwe klawisza PC na kod wirtualny Windows. -1 = nieznana nazwa.
int vk_from_name(const char* name)
{
    if (!name || !*name) return -1;
    if (name[1] == '\0') {                       // pojedyncza litera albo cyfra
        const char c = upper(name[0]);
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return (int)c;
    }
    for (const NamedVk& n : NAMED) {
        if (same_name(name, n.name)) return n.vk;
    }
    return -1;
}

const char* name_from_vk(int vk)
{
    static char buf[8];
    for (const NamedVk& n : NAMED) {
        if (n.vk == vk) return n.name;
    }
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        buf[0] = (char)vk;
        buf[1] = '\0';
        return buf;
    }
    snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
}

char* trim(char* s)
{
    while (*s == ' ' || *s == '\t') ++s;
    char* end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
        *--end = '\0';
    }
    return s;
}

int  s_vk[input::KEY_COUNT];
bool s_loaded = false;

bool load_file(const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) return false;

    char line[256];
    int  applied = 0;
    int  lineno  = 0;
    while (fgets(line, sizeof(line), f)) {
        ++lineno;
        char* p = trim(line);
        if (*p == '\0' || *p == '#' || *p == ';') continue;

        char* eq = strchr(p, '=');
        if (!eq) {
            LAKE_LOGW(TAG, "%s:%d: brak znaku '=' - pomijam", path, lineno);
            continue;
        }
        *eq = '\0';
        char* left  = trim(p);
        char* right = trim(eq + 1);

        input::Key key;
        if (!input::key_from_name(left, key)) {
            LAKE_LOGW(TAG, "%s:%d: nieznany klawisz konsoli '%s'", path, lineno, left);
            continue;
        }
        const int vk = vk_from_name(right);
        if (vk < 0) {
            LAKE_LOGW(TAG, "%s:%d: nieznany klawisz PC '%s'", path, lineno, right);
            continue;
        }
        s_vk[(int)key] = vk;
        ++applied;
    }
    fclose(f);
    LAKE_LOGI(TAG, "wczytano %s (%d wpisow)", path, applied);
    return true;
}

}  // namespace

void keymap_load(const char* explicit_path)
{
    for (int i = 0; i < input::KEY_COUNT; ++i) s_vk[i] = DEFAULT_VK[i];
    s_loaded = true;

    bool ok = false;
    if (explicit_path && *explicit_path) {
        ok = load_file(explicit_path);
        if (!ok) LAKE_LOGE(TAG, "nie moge otworzyc %s - zostaje mapowanie domyslne", explicit_path);
    } else {
        // Obok pliku wykonywalnego, potem typowe miejsca przy uruchamianiu z katalogu projektu.
        char exe[MAX_PATH] = {};
        if (GetModuleFileNameA(nullptr, exe, MAX_PATH)) {
            char* slash = strrchr(exe, '\\');
            if (slash) {
                *(slash + 1) = '\0';
                char candidate[MAX_PATH + 16];
                snprintf(candidate, sizeof(candidate), "%skeymap.cfg", exe);
                ok = load_file(candidate);
            }
        }
        if (!ok) ok = load_file("sim/keymap.cfg");
        if (!ok) ok = load_file("keymap.cfg");
        if (!ok) LAKE_LOGI(TAG, "brak keymap.cfg - mapowanie domyslne");
    }

    for (int i = 0; i < input::KEY_COUNT; ++i) {
        LAKE_LOGI(TAG, "  %-11s <- %s", input::key_name((input::Key)i), name_from_vk(s_vk[i]));
    }
}

int keymap_vk(input::Key key)
{
    if (!s_loaded) keymap_load(nullptr);
    const int i = (int)key;
    return (i >= 0 && i < input::KEY_COUNT) ? s_vk[i] : -1;
}

}  // namespace sim
