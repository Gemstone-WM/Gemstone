cmake_minimum_required(VERSION 3.20)
project(gemstone)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WLROOTS REQUIRED wlroots-0.20)
pkg_check_modules(WAYLAND REQUIRED wayland-server)

add_executable(gemstone
    src/main.c
    src/server/server.c
    src/server/output.c
    src/server/input.c
    src/server/keyboard.c
    src/server/xdg_shell.c
    src/utils/logger.c
)
pkg_check_modules(XKB REQUIRED xkbcommon)

target_include_directories(gemstone PRIVATE ${WLROOTS_INCLUDE_DIRS} ${XKB_INCLUDE_DIRS} src)
target_link_libraries(gemstone PRIVATE ${WLROOTS_LIBRARIES} ${WAYLAND_LIBRARIES} ${XKB_LIBRARIES})

target_compile_options(gemstone PRIVATE ${WLROOTS_CFLAGS_OTHER})