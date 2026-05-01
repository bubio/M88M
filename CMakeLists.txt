cmake_minimum_required(VERSION 3.20)
project(M88M
    VERSION 0.1.0
    LANGUAGES CXX C
    DESCRIPTION "PC-8801 emulator (M88 改) — raylib frontend"
)

# --- Global settings -----------------------------------------------------
set(M88M_VERSION "${M88M_VERSION_MAJOR}.${M88M_VERSION_MINOR}.${M88M_VERSION_PATCH}")
set(MACOSX_BUNDLE_MINIMUM_SYSTEM_VERSION "13.0")
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
if(MSVC)
    add_compile_options(
        /wd4244 /wd4267 /wd4018 /wd4100 /wd4800 /wd4091 /wd4996
    )
else()
    add_compile_options(
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-fno-strict-aliasing>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wno-multichar>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wno-deprecated-declarations>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wno-error=c++11-narrowing>
    )
endif()

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
    M88_PORTABLE
)

target_link_libraries(m88core PUBLIC ZLIB::ZLIB)

# --- raylib ------------------------------------------------------------
include(FetchContent)

# Automatically select binary URL based on platform/arch
if(APPLE)
    # raylib 5.5 release uses a universal binary for macOS
    set(RAYLIB_BINARY_URL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_macos.tar.gz")
elseif(WIN32)
    if(MSVC)
        set(RAYLIB_BINARY_URL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_msvc16.zip")
    else()
        set(RAYLIB_BINARY_URL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip")
    endif()
endif()

if(RAYLIB_BINARY_URL)
    message(STATUS "Downloading raylib binary: ${RAYLIB_BINARY_URL}")
    FetchContent_Declare(
        raylib_bin
        URL ${RAYLIB_BINARY_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(raylib_bin)

    add_library(raylib STATIC IMPORTED GLOBAL)
    if(MSVC)
        set_target_properties(raylib PROPERTIES
            IMPORTED_LOCATION "${raylib_bin_SOURCE_DIR}/lib/raylib.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${raylib_bin_SOURCE_DIR}/include"
        )
    else()
        set_target_properties(raylib PROPERTIES
            IMPORTED_LOCATION "${raylib_bin_SOURCE_DIR}/lib/libraylib.a"
            INTERFACE_INCLUDE_DIRECTORIES "${raylib_bin_SOURCE_DIR}/include"
        )
    endif()
else()
    message(STATUS "No pre-compiled binary for this platform. Falling back to source build.")
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
endif()

include(FetchContent)
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

# --- nativefiledialog-extended -----------------------------------------
FetchContent_Declare(
    nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
    GIT_TAG        v1.2.1
)
FetchContent_MakeAvailable(nfd)

# --- raylib frontend executable ----------------------------------------
file(GLOB RAYLIB_FRONT_SRC src/raylib/*.cpp)

if(RAYLIB_FRONT_SRC)
    if(APPLE)
        add_executable(m88_raylib MACOSX_BUNDLE ${RAYLIB_FRONT_SRC})
        set_target_properties(m88_raylib PROPERTIES
            OUTPUT_NAME "M88M"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.bubio.m88m"
            MACOSX_BUNDLE_BUNDLE_NAME "M88M"
            MACOSX_BUNDLE_BUNDLE_VERSION "${M88M_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${M88M_VERSION}"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/raylib/Info.plist.in"
        )
    else()
        add_executable(m88_raylib ${RAYLIB_FRONT_SRC})
        set_target_properties(m88_raylib PROPERTIES OUTPUT_NAME "m88m")
    endif()

    target_link_libraries(m88_raylib PRIVATE m88core raylib raygui nfd)

    if(MSVC)
        # Force UTF-8 for the entire frontend target (including headers and literals)
        # This resolves the common MSVC encoding issue mentioned in the Teratail link.
        target_compile_options(m88_raylib PRIVATE "/utf-8")
        target_compile_definitions(m88_raylib PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    if(APPLE)
        target_link_libraries(m88_raylib PRIVATE
            iconv
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
            "-framework CoreAudio"
            "-framework AudioToolbox"
        )
    elseif(WIN32)
        target_link_libraries(m88_raylib PRIVATE
            winmm
            gdi32
            shell32
            ole32
            uuid
            comdlg32
        )
    elseif(UNIX)
        target_link_libraries(m88_raylib PRIVATE pthread dl m)
    endif()
else()
    message(STATUS "src/raylib/ has no .cpp files yet — skipping m88_raylib target.")
endif()
