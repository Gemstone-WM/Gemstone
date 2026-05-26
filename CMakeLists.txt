cmake_minimum_required(VERSION 3.16)
project(gemstone C)

# Find the required packages using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(WLROOTS REQUIRED IMPORTED_TARGET wlroots-0.20)
pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

add_executable(gemstone 
    src/main.c
    src/listeners.c
    src/server.c
)


target_compile_definitions(gemstone PRIVATE WLR_USE_UNSTABLE)

target_link_libraries(gemstone
    PkgConfig::WLROOTS
    PkgConfig::WAYLAND
    PkgConfig::XKBCOMMON
)