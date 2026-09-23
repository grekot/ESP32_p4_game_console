# Emulator konsoli Lake na Windows.
#
# Buduje TEN SAM kod co firmware (src/app, engine, gfx, input, ui, lake, games) i dokleja
# implementacje platform:: opartą o Win32 GDI. Nie wymaga SDL ani innych bibliotek.
#
#   cmake -S sim -B sim/build -G Ninja
#   cmake --build sim/build
#   sim/build/lake_sim.exe

cmake_minimum_required(VERSION 3.20)
project(lake_sim LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(LAKE_SRC ${CMAKE_CURRENT_LIST_DIR}/../src)

# ---------------------------------------------------------------- LVGL
# Ta sama wersja co na plytce. Jesli firmware zostal juz zbudowany, uzywamy kopii sciagnietej
# przez menedzera komponentow ESP-IDF (zero pobierania, gwarancja identycznej wersji);
# w przeciwnym razie pobieramy ten sam tag z GitHuba.
set(LVGL_TAG v9.5.0)
# Mozna wskazac wlasna kopie LVGL: -DLAKE_LVGL_DIR=sciezka (pusta/nieistniejaca = pobierz z GitHuba).
set(LAKE_LVGL_DIR "${CMAKE_CURRENT_LIST_DIR}/../managed_components/lvgl__lvgl"
    CACHE PATH "Katalog ze zrodlami LVGL")
set(LVGL_FROM_IDF ${LAKE_LVGL_DIR})

# Konsola nie uzywa przykladow, dem ani ThorVG (grafika wektorowa) - skracamy kompilacje.
set(CONFIG_LV_BUILD_DEMOS         OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES      OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)

# Katalog z lv_conf.h - ten sam plik, ktorego uzywa firmware. Bez tego LVGL szukalby
# konfiguracji w katalogu sim/ i przerwal konfiguracje bledem.
set(LV_BUILD_CONF_DIR ${LAKE_SRC}/ui CACHE PATH "" FORCE)

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
target_include_directories(lvgl PUBLIC ${LAKE_SRC}/ui)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE LAKE_HOST_BUILD)

# ---------------------------------------------------------------- kod wspolny z firmware
# Celowo BEZ src/board (sterowniki plytki), src/platform (implementacja ESP) i src/main.cpp.
# CONFIGURE_DEPENDS: nowy plik .cpp (np. lekcja) jest wykrywany przy zwyklym `cmake --build`,
# bez recznej rekonfiguracji. W firmware to niedozwolone (ESP-IDF), tu - tak.
file(GLOB_RECURSE LAKE_SHARED_SOURCES CONFIGURE_DEPENDS
    ${LAKE_SRC}/app/*.cpp
    ${LAKE_SRC}/engine/*.cpp
    ${LAKE_SRC}/gfx/*.cpp
    ${LAKE_SRC}/input/*.cpp
    ${LAKE_SRC}/ui/*.cpp
    ${LAKE_SRC}/lake/*.cpp
    ${LAKE_SRC}/games/*.cpp)

add_executable(lake_sim
    ${LAKE_SHARED_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/platform_win32.cpp
    ${CMAKE_CURRENT_LIST_DIR}/keymap_win32.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gamepad_win32.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp)

target_include_directories(lake_sim PRIVATE ${LAKE_SRC} ${LAKE_SRC}/ui ${CMAKE_CURRENT_LIST_DIR})

# keymap.cfg obok pliku wykonywalnego - emulator szuka go tam w pierwszej kolejnosci.
add_custom_command(TARGET lake_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_LIST_DIR}/keymap.cfg $<TARGET_FILE_DIR:lake_sim>/keymap.cfg)

target_compile_definitions(lake_sim PRIVATE
    LAKE_HOST_BUILD=1
    LAKE_DISPLAY_ROTATION=90
    # Odkomentuj, aby emulator pokazywal FPS (tak jak firmware z ta sama flaga)
    # LAKE_SHOW_FPS=1
)

# winmm: odczyt pada USB (galka analogowa). Biblioteka jest czescia Windowsa.
target_link_libraries(lake_sim PRIVATE lvgl gdi32 user32 winmm)

if(MINGW)
    target_compile_options(lake_sim PRIVATE -Wall -Wextra -Wno-unused-parameter)
    # Statyczne linkowanie biblioteki standardowej - exe dziala bez DLL-i z MSYS2.
    target_link_options(lake_sim PRIVATE -static-libgcc -static-libstdc++ -static)
endif()
