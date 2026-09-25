# Emulator konsoli na Windows.
#
# Buduje TEN SAM kod co firmware (src/app, engine, gfx, input, ui, console, games) i dokleja
# implementacje platform:: opartą o Win32 GDI. Nie wymaga SDL ani innych bibliotek.
#
#   cmake -S sim -B sim/build -G Ninja
#   cmake --build sim/build
#   sim/build/console_sim.exe

cmake_minimum_required(VERSION 3.20)
project(console_sim LANGUAGES C CXX RC)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CONSOLE_SRC ${CMAKE_CURRENT_LIST_DIR}/../src)

# Wersja wydania (instalator, aktualizacje z GitHuba, ekran "O konsoli"). Build lokalny = "dev" - program startowy
# nie sprawdza wtedy aktualizacji. Wydanie: -DCONSOLE_VERSION=1.2.0 (robi to .github/workflows/release.yml z taga v1.2.0).
set(CONSOLE_VERSION "dev" CACHE STRING "Wersja wydania, np. 1.2.0")
if(CONSOLE_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(CONSOLE_VERSION_NUM "${CMAKE_MATCH_1},${CMAKE_MATCH_2},${CMAKE_MATCH_3},0")
else()
    set(CONSOLE_VERSION_NUM "0,0,0,0")
endif()
set(CONSOLE_RC ${CMAKE_CURRENT_LIST_DIR}/app.rc)
set_source_files_properties(${CONSOLE_RC} PROPERTIES COMPILE_DEFINITIONS
    "CONSOLE_VERSION_RAW=${CONSOLE_VERSION};CONSOLE_VERSION_NUM=${CONSOLE_VERSION_NUM}")

# ---------------------------------------------------------------- LVGL
# Ta sama wersja co na plytce. Jesli firmware zostal juz zbudowany, uzywamy kopii sciagnietej
# przez menedzera komponentow ESP-IDF (zero pobierania, gwarancja identycznej wersji);
# w przeciwnym razie pobieramy ten sam tag z GitHuba.
set(LVGL_TAG v9.5.0)
# Mozna wskazac wlasna kopie LVGL: -DCONSOLE_LVGL_DIR=sciezka (pusta/nieistniejaca = pobierz z GitHuba).
set(CONSOLE_LVGL_DIR "${CMAKE_CURRENT_LIST_DIR}/../managed_components/lvgl__lvgl"
    CACHE PATH "Katalog ze zrodlami LVGL")
set(LVGL_FROM_IDF ${CONSOLE_LVGL_DIR})

# Konsola nie uzywa przykladow, dem ani ThorVG (grafika wektorowa) - skracamy kompilacje.
set(CONFIG_LV_BUILD_DEMOS         OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES      OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)

# Katalog z lv_conf.h - ten sam plik, ktorego uzywa firmware. Bez tego LVGL szukalby
# konfiguracji w katalogu sim/ i przerwal konfiguracje bledem.
set(LV_BUILD_CONF_DIR ${CONSOLE_SRC}/ui CACHE PATH "" FORCE)

# Trzy zrodla, w kolejnosci:
#   1. managed_components/lvgl__lvgl - kopia pobrana przez ESP-IDF przy budowie firmware (komputer rodzica),
#   2. third_party/lvgl              - kopia rozpakowana przez tools/fetch_lvgl.ps1 (komputer ucznia, bez PlatformIO;
#                                      katalog jest w .gitignore),
#   3. FetchContent                  - CMake sam pobiera zip taga z GitHuba (bez gita; pelne drzewo to 180 MB,
#                                      zip ok. 60 MB). Wymaga internetu przy pierwszej konfiguracji.
set(LVGL_THIRD_PARTY "${CMAKE_CURRENT_LIST_DIR}/../third_party/lvgl")

if(EXISTS ${LVGL_FROM_IDF}/lvgl.h)
    message(STATUS "LVGL: uzywam kopii z managed_components (${LVGL_FROM_IDF})")
    add_subdirectory(${LVGL_FROM_IDF} ${CMAKE_BINARY_DIR}/lvgl_build)
elseif(EXISTS ${LVGL_THIRD_PARTY}/lvgl.h)
    message(STATUS "LVGL: uzywam kopii z third_party (${LVGL_THIRD_PARTY})")
    add_subdirectory(${LVGL_THIRD_PARTY} ${CMAKE_BINARY_DIR}/lvgl_build)
else()
    message(STATUS "LVGL: pobieram ${LVGL_TAG} z GitHuba (zip). Bez internetu: uruchom tools/fetch_lvgl.ps1 albo skopiuj third_party/lvgl z innego komputera.")
    include(FetchContent)
    FetchContent_Declare(lvgl
        URL https://github.com/lvgl/lvgl/archive/refs/tags/${LVGL_TAG}.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(lvgl)
endif()

# lv_conf.h lezy w src/ui i jest wspolny z firmware.
target_include_directories(lvgl PUBLIC ${CONSOLE_SRC}/ui)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE CONSOLE_HOST_BUILD)

# ---------------------------------------------------------------- kod wspolny z firmware
# Celowo BEZ src/board (sterowniki plytki), src/platform (implementacja ESP) i src/main.cpp.
# CONFIGURE_DEPENDS: nowy plik .cpp (np. lekcja) jest wykrywany przy zwyklym `cmake --build`,
# bez recznej rekonfiguracji. W firmware to niedozwolone (ESP-IDF), tu - tak.
file(GLOB_RECURSE CONSOLE_SHARED_SOURCES CONFIGURE_DEPENDS
    ${CONSOLE_SRC}/app/*.cpp
    ${CONSOLE_SRC}/engine/*.cpp
    ${CONSOLE_SRC}/gfx/*.cpp
    ${CONSOLE_SRC}/gfx3d/*.cpp
    ${CONSOLE_SRC}/input/*.cpp
    ${CONSOLE_SRC}/ui/*.cpp
    ${CONSOLE_SRC}/ui/*.c        # czcionki z polskimi literami (tools/gen_pl_fonts.py)
    ${CONSOLE_SRC}/console/*.cpp
    ${CONSOLE_SRC}/games/*.cpp)

add_executable(console_sim
    ${CONSOLE_SHARED_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/platform_win32.cpp
    ${CMAKE_CURRENT_LIST_DIR}/keymap_win32.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gamepad_win32.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CONSOLE_RC})

# Wersja instalowana: aplikacja okienkowa (bez okna konsoli); logi ida do pliku przez --log.
# Build deweloperski zostaje konsolowy - testy i zadania VS Code czytaja stdout.
option(CONSOLE_GUI "console_sim.exe bez okna konsoli (instalator)" OFF)
if(CONSOLE_GUI)
    set_target_properties(console_sim PROPERTIES WIN32_EXECUTABLE ON)
endif()

target_include_directories(console_sim PRIVATE ${CONSOLE_SRC} ${CONSOLE_SRC}/ui ${CMAKE_CURRENT_LIST_DIR})

# keymap.cfg obok pliku wykonywalnego - emulator szuka go tam w pierwszej kolejnosci.
add_custom_command(TARGET console_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_LIST_DIR}/keymap.cfg $<TARGET_FILE_DIR:console_sim>/keymap.cfg)

target_compile_definitions(console_sim PRIVATE
    # lodepng (src/gfx/lodepng): tylko dekoder
    LODEPNG_NO_COMPILE_ENCODER LODEPNG_NO_COMPILE_DISK LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS LODEPNG_NO_COMPILE_CPP
    CONSOLE_HOST_BUILD=1
    CONSOLE_DISPLAY_ROTATION=90
    CONSOLE_VERSION="${CONSOLE_VERSION}"
    # Odkomentuj, aby emulator pokazywal FPS (tak jak firmware z ta sama flaga)
    # CONSOLE_SHOW_FPS=1
)

# winmm: odczyt pada USB (galka analogowa). Biblioteka jest czescia Windowsa.
target_link_libraries(console_sim PRIVATE lvgl gdi32 user32 winmm)

if(MINGW)
    target_compile_options(console_sim PRIVATE -Wall -Wextra -Wno-unused-parameter)
    # Statyczne linkowanie biblioteki standardowej - exe dziala bez DLL-i z MSYS2.
    target_link_options(console_sim PRIVATE -static-libgcc -static-libstdc++ -static)
    # Wydanie (tools/build_installer.ps1, CI): bez symboli - exe 3x mniejszy
    target_link_options(console_sim PRIVATE $<$<CONFIG:Release>:-s>)
endif()

# ---------------------------------------------------------------- program startowy wersji instalowanej
# KotarbaConsole.exe: sprawdza aktualizacje na GitHubie i uruchamia console_sim.exe bez okna konsoli.
# Instalator: installer/console.iss, budowa: tools/build_installer.ps1.
add_executable(KotarbaConsole WIN32 ${CMAKE_CURRENT_LIST_DIR}/launcher_win32.cpp ${CONSOLE_RC})
target_compile_definitions(KotarbaConsole PRIVATE CONSOLE_VERSION="${CONSOLE_VERSION}" UNICODE _UNICODE)
target_link_libraries(KotarbaConsole PRIVATE winhttp bcrypt shell32 user32 gdi32)
if(MINGW)
    target_compile_options(KotarbaConsole PRIVATE -Wall -Wextra)
    target_link_options(KotarbaConsole PRIVATE -static-libgcc -static-libstdc++ -static $<$<CONFIG:Release>:-s>)
endif()
