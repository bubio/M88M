cmake_minimum_required(VERSION 3.20)
project(M88M
    LANGUAGES CXX C
    DESCRIPTION "PC-8801 emulator (M88 改) — raylib frontend"
)

# --- Global settings -----------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
    # Single-arch by default; pass -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
    # at configure time for a universal binary.
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
    endif()
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

# FMGEN's table maths rely on type-punning; -fno-strict-aliasing keeps the
# emitted code identical to the MSVC original.
add_compile_options(
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-fno-strict-aliasing>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wno-multichar>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wno-deprecated-declarations>
)

# --- zlib ---------------------------------------------------------------
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    message(STATUS "System zlib not found — using bundled src/zlib")
    file(GLOB ZLIB_SRC src/zlib/*.c)
    add_library(m88_zlib STATIC ${ZLIB_SRC})
    target_include_directories(m88_zlib PUBLIC src/zlib)
    add_library(ZLIB::ZLIB ALIAS m88_zlib)
endif()

# --- Emulation core (m88core) -------------------------------------------
file(GLOB COMMON_SRC  src/common/*.cpp)
file(GLOB DEVICES_SRC src/devices/*.cpp)
file(GLOB PC88_SRC    src/pc88/*.cpp)

# Drop sources that don't belong in the portable core build.
list(FILTER DEVICES_SRC EXCLUDE REGEX "Z80_x86\\.cpp$")
list(FILTER DEVICES_SRC EXCLUDE REGEX "Z80Test\\.cpp$")
list(FILTER DEVICES_SRC EXCLUDE REGEX "Z80Debug\\.cpp$")
# list(FILTER DEVICES_SRC EXCLUDE REGEX "z80diag\\.cpp$")
list(FILTER PC88_SRC    EXCLUDE REGEX "memview\\.cpp$")
list(FILTER PC88_SRC    EXCLUDE REGEX "ioview\\.cpp$")

add_library(m88core STATIC
    ${COMMON_SRC}
    ${DEVICES_SRC}
    ${PC88_SRC}
)

target_include_directories(m88core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pc88
    ${CMAKE_CURRENT_SOURCE_DIR}/src/devices
    ${CMAKE_CURRENT_SOURCE_DIR}/src/if
)

target_compile_definitions(m88core PUBLIC
    M88_NO_Z80_X86
)

target_link_libraries(m88core PUBLIC ZLIB::ZLIB)

# --- raylib ------------------------------------------------------------
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO  ON  CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RMODELS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG        4.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raygui)

# raygui ships a single header; expose its include dir as an interface lib.
add_library(raygui INTERFACE)
target_include_directories(raygui INTERFACE ${raygui_SOURCE_DIR}/src)

# --- raylib frontend executable ----------------------------------------
file(GLOB RAYLIB_FRONT_SRC src/raylib/*.cpp)

if(RAYLIB_FRONT_SRC)
    add_executable(m88_raylib ${RAYLIB_FRONT_SRC})
    target_link_libraries(m88_raylib PRIVATE m88core raylib raygui)
    set_target_properties(m88_raylib PROPERTIES OUTPUT_NAME "m88")

    if(APPLE)
        target_link_libraries(m88_raylib PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
            "-framework CoreAudio"
            "-framework AudioToolbox"
        )
    elseif(UNIX)
        target_link_libraries(m88_raylib PRIVATE pthread dl m)
    endif()
else()
    message(STATUS "src/raylib/ has no .cpp files yet — skipping m88_raylib target.")
endif()
