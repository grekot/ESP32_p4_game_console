// Emulator konsoli na Windows - implementacja warstwy platform:: przez czyste Win32 (GDI).
// Bez SDL i bez zadnych zewnetrznych bibliotek: wystarczy g++/MSVC + gdi32.
//
// Okno pokazuje logiczny ekran konsoli 800x480 w skali 1:1 (plotno konsoli ma te sama
// rozdzielczosc; gry pixel-art powieksza sama konsola, patrz engine/screen.h).

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

#include <stdlib.h>
#include <string.h>

#include "core/log.h"
#include "gamepad_win32.h"
#include "keymap_win32.h"
#include "engine/screen.h"
#include "platform/platform.h"

// Zadeklarowane tu, bo controller() (nizej) korzysta z zastrzyku klawiszy (--hold, --pause-at).
namespace sim {
uint16_t synthetic_keys();
bool     unthrottled();
}

namespace platform {

namespace {

const char* TAG = "sim";

constexpr int   DEFAULT_ZOOM   = 1;   // plotno 800x480 = okno 1:1
constexpr float TARGET_FPS     = 60.0f;
const wchar_t*  WINDOW_CLASS   = L"LakeConsoleSim";

HWND     s_hwnd    = nullptr;
HDC      s_memdc   = nullptr;
HBITMAP  s_dib     = nullptr;
uint32_t* s_dib_px = nullptr;      // BGRA, engine::CANVAS_W x engine::CANVAS_H
bool     s_running = true;

// --- wejscie ---
bool  s_keys[256]   = {};
// Zatrzask wcisniec: WM_KEYDOWN moze przyjsc i zniknac (WM_KEYUP) w obrebie jednej klatki.
// Bez tego bardzo krotkie tapniecie klawisza przepadaloby, bo stan czytamy raz na klatke.
bool  s_key_hit[256] = {};
bool  s_mouse_down  = false;
POINT s_mouse_pos   = {0, 0};

// --- czas ---
LARGE_INTEGER s_freq  = {};
LARGE_INTEGER s_start = {};
int64_t       s_next_frame_us = 0;

input::PadState s_pad_keys;

// Odwzorowanie klawiatury mechanicznej konsoli: kazdy z 10 klawiszy ma przypisany klawisz PC
// (domyslny albo z keymap.cfg). Zadnego odklocania - klawiatura PC nie drga.
// Klawisz uznajemy za wcisniety, jesli jest trzymany ALBO byl tapniety od poprzedniego odczytu.
// Zatrzask jest kasowany po odczytaniu, wiec takie tapniecie trwa dokladnie jedna klatke.
bool key_down(input::Key k)
{
    const int vk = sim::keymap_vk(k);
    if (vk < 0 || vk >= 256) return false;
    const bool down = s_keys[vk] || s_key_hit[vk];
    s_key_hit[vk] = false;
    return down;
}

void update_keys()
{
    uint16_t mask = 0;
    for (int i = 0; i <= (int)input::Key::Select; ++i) {
        if (key_down((input::Key)i)) mask |= (uint16_t)(1u << i);
    }
    s_pad_keys = input::pad_from_mask(mask);

    // Galka: jesli jest pad USB, bierzemy z niego prawdziwe wartosci analogowe; jesli nie,
    // osobne klawisze daja wychylenie skrajne (tak jak krzyzak, ale osobnym kanalem).
    float ax = 0.f, ay = 0.f;
    if (!sim::gamepad_axes(ax, ay)) {
        if (key_down(input::Key::StickLeft))  ax -= 1.f;
        if (key_down(input::Key::StickRight)) ax += 1.f;
        if (key_down(input::Key::StickUp))    ay -= 1.f;
        if (key_down(input::Key::StickDown))  ay += 1.f;
    }
    s_pad_keys.stick_x = ax;
    s_pad_keys.stick_y = ay;

    // Ten sam prog co na plytce - galka dubluje krzyzak.
    constexpr float DIR_THRESHOLD = 0.5f;
    if (ax <= -DIR_THRESHOLD) s_pad_keys.left  = true;
    if (ax >=  DIR_THRESHOLD) s_pad_keys.right = true;
    if (ay <= -DIR_THRESHOLD) s_pad_keys.up    = true;
    if (ay >=  DIR_THRESHOLD) s_pad_keys.down  = true;
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            s_running = false;
            return 0;

        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) { s_running = false; return 0; }
            if (wp < 256) { s_keys[wp] = true; s_key_hit[wp] = true; }
            return 0;

        case WM_KEYUP:
            if (wp < 256) s_keys[wp] = false;
            return 0;

        case WM_KILLFOCUS:                    // po utracie fokusu klawisze "zawisaja"
            memset(s_keys, 0, sizeof(s_keys));
            memset(s_key_hit, 0, sizeof(s_key_hit));
            s_mouse_down = false;
            return 0;

        case WM_LBUTTONDOWN:
            s_mouse_down = true;
            SetCapture(hwnd);
            s_mouse_pos.x = GET_X_LPARAM(lp);
            s_mouse_pos.y = GET_Y_LPARAM(lp);
            return 0;

        case WM_LBUTTONUP:
            s_mouse_down = false;
            ReleaseCapture();
            return 0;

        case WM_MOUSEMOVE:
            s_mouse_pos.x = GET_X_LPARAM(lp);
            s_mouse_pos.y = GET_Y_LPARAM(lp);
            return 0;

        case WM_ERASEBKGND:
            return 1;                          // nie migaj tlem - i tak malujemy caly obszar

        default:
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

// Sama obsluga komunikatow okna. Migawke klawiszy robi osobno update_keys(), bo zatrzask
// wcisniec wolno skonsumowac dokladnie raz na klatke - w controller().
void pump_messages()
{
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void client_size(int& w, int& h)
{
    RECT rc = {};
    GetClientRect(s_hwnd, &rc);
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
    if (w <= 0) w = engine::SCREEN_W;
    if (h <= 0) h = engine::SCREEN_H;
}

}  // namespace

bool init()
{
    QueryPerformanceFrequency(&s_freq);
    QueryPerformanceCounter(&s_start);

    HINSTANCE inst = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = inst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = WINDOW_CLASS;
    if (!RegisterClassExW(&wc)) {
        LAKE_LOGE(TAG, "RegisterClassEx nie powiodlo sie (%lu)", (unsigned long)GetLastError());
        return false;
    }

    RECT rc = { 0, 0, engine::CANVAS_W * DEFAULT_ZOOM, engine::CANVAS_H * DEFAULT_ZOOM };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    s_hwnd = CreateWindowExW(0, WINDOW_CLASS, L"Lake Console - emulator (ESP32-P4)",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             rc.right - rc.left, rc.bottom - rc.top,
                             nullptr, nullptr, inst, nullptr);
    if (!s_hwnd) {
        LAKE_LOGE(TAG, "CreateWindowEx nie powiodlo sie (%lu)", (unsigned long)GetLastError());
        return false;
    }

    // Bitmapa posrednia w rozmiarze plotna; powiekszenie robi StretchDIBits przy rysowaniu.
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = engine::CANVAS_W;
    bmi.bmiHeader.biHeight      = -engine::CANVAS_H;   // minus = pierwszy wiersz na gorze
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(s_hwnd);
    s_memdc = CreateCompatibleDC(screen);
    s_dib   = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, (void**)&s_dib_px, nullptr, 0);
    ReleaseDC(s_hwnd, screen);

    if (!s_dib || !s_dib_px) {
        LAKE_LOGE(TAG, "CreateDIBSection nie powiodlo sie");
        return false;
    }
    SelectObject(s_memdc, s_dib);

    ShowWindow(s_hwnd, SW_SHOW);
    UpdateWindow(s_hwnd);

    LAKE_LOGI(TAG, "okno %dx%d (plotno %dx%d, powiekszenie x%d)",
              engine::CANVAS_W * DEFAULT_ZOOM, engine::CANVAS_H * DEFAULT_ZOOM,
              engine::CANVAS_W, engine::CANVAS_H, DEFAULT_ZOOM);
    LAKE_LOGI(TAG, "klawisze konsoli wg mapowania powyzej; lewy przycisk myszy = dotyk ekranu, "
                   "Esc = wyjscie");
    return true;
}

int64_t micros()
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (int64_t)((now.QuadPart - s_start.QuadPart) * 1000000LL / s_freq.QuadPart);
}

uint32_t millis()
{
    return (uint32_t)(micros() / 1000);
}

void sleep_ms(uint32_t ms)
{
    Sleep(ms);
}

uint16_t* alloc_pixels(size_t pixel_count, bool /*fast*/)
{
    return (uint16_t*)calloc(pixel_count, sizeof(uint16_t));
}

void present(const uint16_t* canvas)
{
    pump_messages();
    if (!s_running || !canvas || !s_dib_px) return;

    // RGB565 -> BGRA (format DIB-u)
    const size_t n = (size_t)engine::CANVAS_W * engine::CANVAS_H;
    for (size_t i = 0; i < n; ++i) {
        const uint16_t c = canvas[i];
        const uint32_t r = (uint32_t)((c >> 11) & 0x1F);
        const uint32_t g = (uint32_t)((c >> 5) & 0x3F);
        const uint32_t b = (uint32_t)(c & 0x1F);
        // rozciagniecie 5/6 bitow na 8 z powieleniem najstarszych bitow
        const uint32_t r8 = (r << 3) | (r >> 2);
        const uint32_t g8 = (g << 2) | (g >> 4);
        const uint32_t b8 = (b << 3) | (b >> 2);
        s_dib_px[i] = (r8 << 16) | (g8 << 8) | b8;
    }

    int cw, ch;
    client_size(cw, ch);

    HDC dc = GetDC(s_hwnd);
    SetStretchBltMode(dc, COLORONCOLOR);   // najblizszy sasiad - ostre piksele, jak na plytce
    StretchBlt(dc, 0, 0, cw, ch, s_memdc, 0, 0, engine::CANVAS_W, engine::CANVAS_H, SRCCOPY);
    ReleaseDC(s_hwnd, dc);

    // Tryb skryptowany (--frames): nie czekamy na 60 FPS - wyniki sa i tak liczone ze stalym dt.
    if (sim::unthrottled()) return;

    // Rownanie do ~60 FPS (na plytce te role pelni synchronizacja pionowa panelu).
    const int64_t frame_us = (int64_t)(1000000.0f / TARGET_FPS);
    const int64_t now      = micros();
    if (s_next_frame_us == 0) s_next_frame_us = now;
    s_next_frame_us += frame_us;
    const int64_t wait = s_next_frame_us - now;
    if (wait > 1000) {
        Sleep((DWORD)(wait / 1000));
    } else if (wait < -frame_us * 4) {
        s_next_frame_us = now;   // mocno spoznieni - nie nadganiaj serii klatek
    }
}

int read_touch(input::TouchPoint* out, int max_points)
{
    if (!s_mouse_down || max_points < 1) return 0;

    int cw, ch;
    client_size(cw, ch);
    const int x = (int)((int64_t)s_mouse_pos.x * engine::CANVAS_W / (cw > 0 ? cw : 1));
    const int y = (int)((int64_t)s_mouse_pos.y * engine::CANVAS_H / (ch > 0 ? ch : 1));
    if (x < 0 || y < 0 || x >= engine::CANVAS_W || y >= engine::CANVAS_H) return 0;

    out[0].x = (int16_t)x;
    out[0].y = (int16_t)y;
    return 1;
}

input::PadState controller()
{
    // Komunikaty pobieramy tuz przed odczytem, a nie dopiero przy rysowaniu klatki.
    // Wczesniej stan klawiszy byl o cala klatke (do 17 ms) spozniony i sterowanie mulilo.
    pump_messages();
    update_keys();

    input::PadState p = s_pad_keys;

    // Klawisze wstrzykniete z linii polecen (--hold, --pause-at) dokladamy do prawdziwych.
    if (const uint16_t syn = sim::synthetic_keys()) {
        const input::PadState sp = input::pad_from_mask(syn);
        p.up |= sp.up; p.down |= sp.down; p.left |= sp.left; p.right |= sp.right;
        p.a |= sp.a;   p.b |= sp.b;       p.x |= sp.x;       p.y |= sp.y;
        p.start |= sp.start;              p.select |= sp.select;
    }
    return p;
}

bool should_run()
{
    return s_running;
}


namespace detail {
// Dostep do bufora ostatniej klatki (w formacie DIB) dla funkcji pomocniczych emulatora.
const uint32_t* dib_pixels() { return s_dib_px; }
}  // namespace detail

}  // namespace platform

// ============================================================================
// Dodatki emulatora (nie istnieja na plytce)
// ============================================================================

#include "sim_extra.h"

#include <stdio.h>

namespace sim {

namespace {
uint16_t s_synthetic_keys = 0;
bool     s_unthrottled     = false;
}

void     set_synthetic_keys(uint16_t mask) { s_synthetic_keys = mask; }
uint16_t synthetic_keys()                  { return s_synthetic_keys; }
void     set_unthrottled(bool on)          { s_unthrottled = on; }
bool     unthrottled()                     { return s_unthrottled; }

bool save_screenshot(const char* path)
{
    if (!path || !platform::detail::dib_pixels()) return false;

    const int W = engine::CANVAS_W;
    const int H = engine::CANVAS_H;
    const int row_bytes = ((W * 3) + 3) & ~3;          // wiersze BMP wyrownane do 4 bajtow
    const int pixel_bytes = row_bytes * H;

    FILE* f = fopen(path, "wb");
    if (!f) return false;

    const uint32_t file_size = 14 + 40 + (uint32_t)pixel_bytes;
    uint8_t header[14 + 40] = {};
    header[0] = 'B'; header[1] = 'M';
    memcpy(header + 2, &file_size, 4);
    const uint32_t data_offset = 14 + 40;
    memcpy(header + 10, &data_offset, 4);
    const uint32_t dib_size = 40;
    memcpy(header + 14, &dib_size, 4);
    const int32_t w32 = W, h32 = H;                     // dodatnia wysokosc = wiersze od dolu
    memcpy(header + 18, &w32, 4);
    memcpy(header + 22, &h32, 4);
    const uint16_t planes = 1, bpp = 24;
    memcpy(header + 26, &planes, 2);
    memcpy(header + 28, &bpp, 2);
    const uint32_t img_size = (uint32_t)pixel_bytes;
    memcpy(header + 34, &img_size, 4);
    fwrite(header, 1, sizeof(header), f);

    uint8_t* row = (uint8_t*)calloc(1, (size_t)row_bytes);
    if (!row) { fclose(f); return false; }
    const uint32_t* px = platform::detail::dib_pixels();
    for (int y = H - 1; y >= 0; --y) {                  // BMP trzyma wiersze od dolu
        for (int x = 0; x < W; ++x) {
            const uint32_t c = px[(size_t)y * W + x];
            row[x * 3 + 0] = (uint8_t)(c & 0xFF);         // B
            row[x * 3 + 1] = (uint8_t)((c >> 8) & 0xFF);  // G
            row[x * 3 + 2] = (uint8_t)((c >> 16) & 0xFF); // R
        }
        fwrite(row, 1, (size_t)row_bytes, f);
    }
    free(row);
    fclose(f);
    return true;
}

}  // namespace sim
