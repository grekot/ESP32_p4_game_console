cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# src/ui musi byc na sciezce include KAZDEGO komponentu, nie tylko naszego: LVGL kompiluje
# swoje zrodla osobno i sam szuka pliku lv_conf.h. Flagi z platformio.ini tu nie wystarcza.
# Razem z CONFIG_LV_CONF_SKIP=n (sdkconfig.defaults) daje to jedna konfiguracje LVGL
# wspoldzielona z emulatorem na Windows (sim/).
idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_CURRENT_LIST_DIR}/src/ui" APPEND)

project(LakeMarioGame)
