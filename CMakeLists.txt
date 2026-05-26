cmake_minimum_required(VERSION 3.16)
project(gemstone C)

# Use C11 for better modern C support
set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
# If your system uses a different wlroots version name, update this line
pkg_check_modules(WLROOTS REQUIRED IMPORTED_TARGET wlroots-0.20)
pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

# Tell CMake to look for headers in your 'src' or root directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

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