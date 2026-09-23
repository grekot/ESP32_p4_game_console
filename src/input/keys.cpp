#include "input/keys.h"

#include <string.h>

namespace input {

namespace {

const char* const NAMES[KEY_COUNT] = {
    "UP", "DOWN", "LEFT", "RIGHT", "A", "B", "X", "Y", "START", "SELECT",
    "STICK_UP", "STICK_DOWN", "STICK_LEFT", "STICK_RIGHT"
};

char upper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c; }

bool equal_ignore_case(const char* a, const char* b)
{
    while (*a && *b) {
        if (upper(*a) != upper(*b)) return false;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

}  // namespace

const char* key_name(Key k)
{
    const int i = (int)k;
    return (i >= 0 && i < KEY_COUNT) ? NAMES[i] : "?";
}

bool key_from_name(const char* name, Key& out)
{
    if (!name) return false;
    for (int i = 0; i < KEY_COUNT; ++i) {
        if (equal_ignore_case(name, NAMES[i])) {
            out = (Key)i;
            return true;
        }
    }
    return false;
}

}  // namespace input
