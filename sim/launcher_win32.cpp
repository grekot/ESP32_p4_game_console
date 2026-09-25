// KotarbaConsole.exe - program startowy wersji instalowanej (installer/console.iss).
//
// 1. Sprawdza najnowsze wydanie na GitHubie (api.github.com/repos/.../releases/latest, limit 4 s).
//    Brak internetu, blad albo wersja "dev" = cicho pomijamy i uruchamiamy gre.
// 2. Gdy wydanie jest nowsze niz CONSOLE_VERSION: pyta, pobiera instalator KotarbaConsole-*-setup.exe
//    do %TEMP%, sprawdza SHA-256 (pole "digest" z API, jesli jest), uruchamia zwykly kreator z /RELAUNCH
//    i konczy sie. Instalator po skonczeniu sam uruchamia konsole ponownie (sekcja [Run], IsRelaunch).
// 3. Sprawdza, czy grafika (assets/) jest na dysku i da sie ja czytac - jesli nie, mowi, co zrobic (antywirus).
// 4. Uruchamia console_sim.exe (w wersji instalowanej aplikacja okienkowa, CONSOLE_GUI) z --log log.txt.
//
// Argumenty: --no-update (bez sprawdzania; tak startuje instalator), reszta idzie do console_sim.exe.
// Instalacja jest per uzytkownik (bez praw administratora), wiec aktualizacja nie wywoluje UAC.

#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <shellapi.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifndef CONSOLE_VERSION
#define CONSOLE_VERSION "dev"
#endif

namespace {

const wchar_t* APP_TITLE   = L"Kotarba Game Console";
// Obie stale mozna podmienic przy kompilacji (-DCONSOLE_RELEASE_API=...), np. do testu na innym repozytorium.
#ifndef CONSOLE_RELEASE_API
#define CONSOLE_RELEASE_API "https://api.github.com/repos/grekot/ESP32_p4_game_console/releases/latest"
#endif
#ifndef CONSOLE_ASSET_TAIL
#define CONSOLE_ASSET_TAIL "-setup.exe"
#endif
const char*    RELEASE_API = CONSOLE_RELEASE_API;
const char*    ASSET_TAIL  = CONSOLE_ASSET_TAIL;

std::wstring widen(const std::string& s)
{
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}

std::wstring exe_dir()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) *slash = 0;
    return path;
}

// ---------------------------------------------------------------- okienko postepu pobierania

HWND s_progress = nullptr;

void progress_show(const wchar_t* text)
{
    if (!s_progress) {
        const int w = 420, h = 90;
        s_progress = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"STATIC", text,
                                     WS_POPUP | WS_BORDER | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                                     (GetSystemMetrics(SM_CXSCREEN) - w) / 2, (GetSystemMetrics(SM_CYSCREEN) - h) / 2,
                                     w, h, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        SendMessageW(s_progress, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    } else {
        SetWindowTextW(s_progress, text);
    }
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void progress_hide()
{
    if (s_progress) DestroyWindow(s_progress);
    s_progress = nullptr;
}

// ---------------------------------------------------------------- HTTPS (WinHTTP)

// GET pod adres https://; przekierowania (GitHub -> objects.githubusercontent.com) WinHTTP obsluguje sam.
// out == nullptr: tresc do body; inaczej do pliku, z postepem w okienku. Zwraca false przy bledzie lub kodzie != 200.
bool http_get(const char* url, std::string* body, HANDLE out, int timeout_ms)
{
    const std::wstring wurl = widen(url);
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256], path[2048];
    uc.lpszHostName = host;  uc.dwHostNameLength = 256;
    uc.lpszUrlPath  = path;  uc.dwUrlPathLength  = 2048;
    wchar_t extra[2048];
    uc.lpszExtraInfo = extra; uc.dwExtraInfoLength = 2048;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc) || uc.nScheme != INTERNET_SCHEME_HTTPS) return false;
    std::wstring full_path = std::wstring(path) + extra;

    bool ok = false;
    HINTERNET ses = WinHttpOpen(L"KotarbaConsole/" CONSOLE_VERSION, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET con = ses ? WinHttpConnect(ses, host, uc.nPort, 0) : nullptr;
    HINTERNET req = con ? WinHttpOpenRequest(con, L"GET", full_path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
    if (req) {
        WinHttpSetTimeouts(req, timeout_ms, timeout_ms, timeout_ms, timeout_ms);
        const wchar_t* hdr = body ? L"Accept: application/vnd.github+json\r\n" : L"Accept: application/octet-stream\r\n";
        DWORD status = 0, len = sizeof(status);
        if (WinHttpSendRequest(req, hdr, (DWORD)-1, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(req, nullptr) &&
            WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                                &status, &len, WINHTTP_NO_HEADER_INDEX) &&
            status == 200) {
            DWORD total = 0;
            len = sizeof(total);
            WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                                &total, &len, WINHTTP_NO_HEADER_INDEX);
            ok = true;
            uint64_t got = 0;
            int last_pct = -1;
            char buf[16384];
            for (;;) {
                DWORD n = 0;
                if (!WinHttpReadData(req, buf, sizeof(buf), &n)) { ok = false; break; }
                if (n == 0) break;
                got += n;
                if (body) {
                    body->append(buf, n);
                } else {
                    DWORD written = 0;
                    if (!WriteFile(out, buf, n, &written, nullptr) || written != n) { ok = false; break; }
                    const int pct = total ? (int)(got * 100 / total) : -1;
                    if (pct != last_pct) {
                        wchar_t msg[128];
                        if (pct >= 0) swprintf(msg, 128, L"Pobieranie aktualizacji... %d%%", pct);
                        else swprintf(msg, 128, L"Pobieranie aktualizacji... %u kB", (unsigned)(got / 1024));
                        progress_show(msg);
                        last_pct = pct;
                    }
                }
            }
            if (total && got != total) ok = false;
        }
    }
    if (req) WinHttpCloseHandle(req);
    if (con) WinHttpCloseHandle(con);
    if (ses) WinHttpCloseHandle(ses);
    return ok;
}

// ---------------------------------------------------------------- odpowiedz API (bez biblioteki JSON)

// Wartosc tekstowa pola "key" szukanego od pozycji from; pos = pozycja za wartoscia. Pusta, gdy brak.
std::string json_string(const std::string& js, const char* key, size_t from, size_t* pos = nullptr)
{
    const std::string pat = std::string("\"") + key + "\"";
    size_t p = js.find(pat, from);
    if (p == std::string::npos) return {};
    p = js.find(':', p + pat.size());
    if (p == std::string::npos) return {};
    p = js.find('"', p);
    if (p == std::string::npos) return {};
    std::string v;
    for (++p; p < js.size() && js[p] != '"'; ++p) {
        if (js[p] == '\\' && p + 1 < js.size()) ++p;   // adresy i wersje nie maja ucieczek poza \/
        v += js[p];
    }
    if (pos) *pos = p;
    return v;
}

struct Release {
    std::string tag, url, sha256;   // sha256 malymi literami hex albo pusty
};

bool find_release(Release& r)
{
    std::string js;
    if (!http_get(RELEASE_API, &js, nullptr, 4000)) return false;
    r.tag = json_string(js, "tag_name", 0);
    if (r.tag.empty()) return false;
    // Lista "assets": kazdy ma "name", "digest" (od 2025, "sha256:..."), "browser_download_url".
    size_t p = js.find("\"assets\"");
    while (p != std::string::npos) {
        size_t next = 0;
        const std::string name = json_string(js, "name", p, &next);
        if (name.empty()) break;
        const size_t url_at = js.find("\"browser_download_url\"", next);
        if (url_at == std::string::npos) break;
        size_t after = 0;
        const std::string url = json_string(js, "browser_download_url", next, &after);
        if (name.size() > strlen(ASSET_TAIL) && name.compare(name.size() - strlen(ASSET_TAIL), std::string::npos, ASSET_TAIL) == 0) {
            r.url = url;
            // digest lezy w tym samym obiekcie, przed adresem pobrania
            const size_t dig = js.find("\"digest\"", next);
            if (dig != std::string::npos && dig < url_at) {
                const std::string d = json_string(js, "digest", next);
                if (d.rfind("sha256:", 0) == 0) r.sha256 = d.substr(7);
            }
            return true;
        }
        p = after;
    }
    return false;
}

// "v1.2.10" / "1.2" -> porownanie liczbowe kolejnych czesci; reszta po '-' ignorowana.
int compare_versions(const char* a, const char* b)
{
    if (*a == 'v' || *a == 'V') ++a;
    if (*b == 'v' || *b == 'V') ++b;
    for (int i = 0; i < 4; ++i) {
        const long x = strtol(a, (char**)&a, 10), y = strtol(b, (char**)&b, 10);
        if (x != y) return x < y ? -1 : 1;
        if (*a == '.') ++a;
        if (*b == '.') ++b;
    }
    return 0;
}

std::string sha256_file(const std::wstring& path)
{
    std::string hex;
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) return hex;
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE h = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0 &&
        BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0) == 0) {
        char buf[65536];
        DWORD n = 0;
        while (ReadFile(f, buf, sizeof(buf), &n, nullptr) && n) BCryptHashData(h, (PUCHAR)buf, n, 0);
        unsigned char dig[32];
        if (BCryptFinishHash(h, dig, sizeof(dig), 0) == 0) {
            char two[3];
            for (unsigned char c : dig) { snprintf(two, sizeof(two), "%02x", c); hex += two; }
        }
    }
    if (h) BCryptDestroyHash(h);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    CloseHandle(f);
    return hex;
}

// true = instalator aktualizacji uruchomiony, launcher ma sie zakonczyc bez startu gry.
bool check_for_update()
{
    if (strcmp(CONSOLE_VERSION, "dev") == 0) return false;   // build lokalny - nie ma czego porownywac
    Release r;
    if (!find_release(r) || r.url.empty()) return false;
    if (compare_versions(r.tag.c_str(), CONSOLE_VERSION) <= 0) return false;

    wchar_t q[512];
    swprintf(q, 512, L"Jest nowa wersja konsoli: %ls (masz %ls).\n\nZainstalowa\u0107 teraz? To potrwa kilkana\u015bcie sekund.",
             widen(r.tag).c_str(), widen(CONSOLE_VERSION).c_str());
#ifdef LAUNCHER_SELFTEST
    printf("wydanie %s > %s: %s sha256=%s\n", r.tag.c_str(), CONSOLE_VERSION, r.url.c_str(), r.sha256.c_str());
    if (getenv("SELFTEST_NO_DOWNLOAD")) return true;
#else
    if (MessageBoxW(nullptr, q, APP_TITLE, MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND) != IDYES) return false;
#endif

    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    const std::wstring file = std::wstring(tmp) + L"KotarbaConsole-update-setup.exe";
    HANDLE out = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    bool ok = out != INVALID_HANDLE_VALUE;
    progress_show(L"Pobieranie aktualizacji...");
    if (ok) {
        ok = http_get(r.url.c_str(), nullptr, out, 30000);
        CloseHandle(out);
    }
    if (ok && !r.sha256.empty()) {
        progress_show(L"Sprawdzanie pliku...");
        ok = sha256_file(file) == r.sha256;
    }
    progress_hide();
    if (!ok) {
        DeleteFileW(file.c_str());
        MessageBoxW(nullptr, L"Nie uda\u0142o si\u0119 pobra\u0107 aktualizacji. Uruchamiam obecn\u0105 wersj\u0119.",
                    APP_TITLE, MB_OK | MB_ICONWARNING);
        return false;
    }
#ifdef LAUNCHER_SELFTEST
    printf("pobrano %ls, sha256 %s\n", file.c_str(), r.sha256.empty() ? "brak w API" : "zgodny");
    return true;
#endif
    // Zwykly kreator (bez /SILENT): cicha instalacja pobranego exe to wzorzec, na ktory antywirusy (Avast) reaguja
    // najostrzej. /RELAUNCH: instalator uruchomi konsole po zakonczeniu.
    const INT_PTR rc = (INT_PTR)ShellExecuteW(nullptr, L"open", file.c_str(), L"/RELAUNCH", nullptr, SW_SHOWNORMAL);
    return rc > 32;
}

const wchar_t* AV_HINT =
    L"Najcz\u0119\u015bciej to antywirus (np. Avast) blokuje nieznany program. Dodaj katalog konsoli do wyj\u0105tk\u00f3w "
    L"antywirusa (Avast: Menu > Ustawienia > Og\u00f3lne > Wyj\u0105tki > Dodaj wyj\u0105tek) i zainstaluj konsol\u0119 ponownie.";

// Grafika gier lezy w assets/. Gdy jej brak albo nie da sie jej czytac, gra uruchomi sie bez obrazkow -
// lepiej od razu powiedziec dlaczego (zgloszenie 25.09: Avast, menu bez okladek).
void check_assets(const std::wstring& dir)
{
    const std::wstring probe = dir + L"\\assets\\covers\\mario.png";
    HANDLE f = CreateFileW(probe.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f != INVALID_HANDLE_VALUE) { CloseHandle(f); return; }
    const DWORD err = GetLastError();
    wchar_t msg[1024];
    swprintf(msg, 1024, L"%ls (b\u0142\u0105d %lu)\n\n%ls\n\nKatalog konsoli:\n%ls",
             err == ERROR_ACCESS_DENIED ? L"Brak dost\u0119pu do plik\u00f3w z grafik\u0105 gier."
                                        : L"Brakuje plik\u00f3w z grafik\u0105 gier (katalog assets).",
             (unsigned long)err, AV_HINT, dir.c_str());
    MessageBoxW(nullptr, msg, APP_TITLE, MB_OK | MB_ICONWARNING);
}

// Reszta wiersza polecen za nazwa programu, bez --no-update.
std::wstring pass_through_args(bool& no_update)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring args;
    for (int i = 1; argv && i < argc; ++i) {
        if (wcscmp(argv[i], L"--no-update") == 0) { no_update = true; continue; }
        args += L" \"";
        args += argv[i];
        args += L"\"";
    }
    if (argv) LocalFree(argv);
    return args;
}

}  // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    bool no_update = false;
    const std::wstring args = pass_through_args(no_update);
    if (!no_update && check_for_update()) return 0;

    const std::wstring dir = exe_dir();
    check_assets(dir);
    // Log gry obok exe (console_sim.exe --log; gdy tam nie wolno pisac - %TEMP%\KotarbaConsole-log.txt).
    std::wstring cmd = L"\"" + dir + L"\\console_sim.exe\" --log \"" + dir + L"\\log.txt\"" + args;
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, 0, nullptr, dir.c_str(), &si, &pi)) {
        wchar_t msg[512];
        swprintf(msg, 512, L"Nie mog\u0119 uruchomi\u0107 console_sim.exe (b\u0142\u0105d %lu).\n\n%ls",
                 (unsigned long)GetLastError(), GetLastError() == ERROR_ACCESS_DENIED ? AV_HINT : L"Zainstaluj konsol\u0119 ponownie.");
        MessageBoxW(nullptr, msg, APP_TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
