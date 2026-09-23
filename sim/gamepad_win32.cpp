// Emulator: obsluga pada USB podlaczonego do PC (opcjonalna).
//
// Po co: konsola ma galke analogowa, a klawiatura daje tylko wychylenie skrajne. Jesli do PC
// jest podlaczony pad, jego lewa galka steruje galka konsoli z pelna rozdzielczoscia - dzieki
// temu w emulatorze da sie sprawdzic gry korzystajace z plynnego ruchu.
//
// Uzywamy starego API joystickowego z winmm: jest w kazdym Windowsie, nie wymaga zadnych
// bibliotek do pobrania i widzi zarowno pady XInput, jak i zwykle DirectInput.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>

#include "core/log.h"
#include "gamepad_win32.h"

namespace sim {

namespace {

const char* TAG = "gamepad";

constexpr float DEAD_ZONE = 0.15f;   // pady tez plywaja wokol srodka

bool s_checked   = false;
bool s_present   = false;
UINT s_id        = 0;
JOYCAPS s_caps   = {};

// surowa wartosc osi -> -1..1 wzgledem srodka zakresu, ze strefa martwa
float normalize(DWORD value, DWORD vmin, DWORD vmax)
{
    if (vmax <= vmin) return 0.f;
    const float span = (float)(vmax - vmin);
    float v = ((float)(value - vmin) / span) * 2.f - 1.f;   // -1..1
    if (v > 1.f)  v = 1.f;
    if (v < -1.f) v = -1.f;

    const float mag = v < 0 ? -v : v;
    if (mag < DEAD_ZONE) return 0.f;
    const float scaled = (mag - DEAD_ZONE) / (1.f - DEAD_ZONE);
    return v < 0 ? -scaled : scaled;
}

void detect_once()
{
    if (s_checked) return;
    s_checked = true;

    const UINT count = joyGetNumDevs();
    for (UINT id = 0; id < count && id < 4; ++id) {
        JOYINFOEX info = {};
        info.dwSize  = sizeof(info);
        info.dwFlags = JOY_RETURNALL;
        if (joyGetPosEx(id, &info) != JOYERR_NOERROR) continue;
        if (joyGetDevCaps(id, &s_caps, sizeof(s_caps)) != JOYERR_NOERROR) continue;

        s_id      = id;
        s_present = true;
        CONSOLE_LOGI(TAG, "pad USB %u: %s - lewa galka steruje galka konsoli",
                  id, s_caps.szPname);
        return;
    }
    CONSOLE_LOGI(TAG, "brak pada USB - galka konsoli sterowana klawiszami");
}

}  // namespace

bool gamepad_axes(float& x, float& y)
{
    detect_once();
    if (!s_present) return false;

    JOYINFOEX info = {};
    info.dwSize  = sizeof(info);
    info.dwFlags = JOY_RETURNX | JOY_RETURNY;
    if (joyGetPosEx(s_id, &info) != JOYERR_NOERROR) {
        s_present = false;                 // pad odlaczony w trakcie
        CONSOLE_LOGW(TAG, "pad USB zniknal");
        return false;
    }

    x = normalize(info.dwXpos, s_caps.wXmin, s_caps.wXmax);
    y = normalize(info.dwYpos, s_caps.wYmin, s_caps.wYmax);
    return true;
}

bool gamepad_present()
{
    detect_once();
    return s_present;
}

}  // namespace sim
