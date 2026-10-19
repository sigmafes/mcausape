cmake_minimum_required(VERSION 3.16)
project(mcpe061native C CXX)

# A native Windows build of the same source tree the web target compiles: SDL2 for
# the window and input, desktop OpenGL (the renderer is fixed-function GL 1.x, so
# no emulation layer sits between it and the driver), and RakNet with real UDP.
#
# This is the feasibility build: one .exe that opens a window and reaches the
# menu. What it does not have yet is listed at the bottom of this file.
#
# Configure with the llvm-mingw toolchain on PATH (see README.native.md):
#
#   cmake -S handheld/project/native -B handheld/project/native/build -G Ninja ^
#         -DCMAKE_BUILD_TYPE=Release ^
#         -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
#   cmake --build handheld/project/native/build

if(NOT WIN32)
    message(FATAL_ERROR "This target builds the Windows executable.")
endif()

# Same 2013-era C++03 as the other targets; extensions on for the GNU spellings.
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(SRC_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/../../src")
set(DATA_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/../../data")
set(RAKNET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib_projects/raknet/jni/RaknetSources")

# SDL2, libpng and zlib: Emscripten already unpacked the sources of the three for
# the web build (it fetches them as "ports"), so they are built from there rather
# than downloaded again. Point this at any folder holding sdl2/, libpng/, zlib/.
set(PORTS_DIR "C:/Users/Rosario/dev/emsdk/upstream/emscripten/cache/ports"
    CACHE PATH "Folder with the SDL2, libpng and zlib sources")

file(GLOB SDL_ROOT  LIST_DIRECTORIES true "${PORTS_DIR}/sdl2/SDL-release-*")
file(GLOB PNG_ROOT  LIST_DIRECTORIES true "${PORTS_DIR}/libpng/libpng-*")
file(GLOB ZLIB_ROOT LIST_DIRECTORIES true "${PORTS_DIR}/zlib/zlib-*")
foreach(d SDL_ROOT PNG_ROOT ZLIB_ROOT)
    if(NOT ${d})
        message(FATAL_ERROR "${d}: not found under ${PORTS_DIR}")
    endif()
endforeach()

#
# SDL2, static
#
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)
set(SDL_TESTS  OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${SDL_ROOT}" "${CMAKE_BINARY_DIR}/sdl2" EXCLUDE_FROM_ALL)

#
# zlib: only what libpng needs (no gz* file layer)
#
set(ZLIB_SOURCES adler32.c compress.c crc32.c deflate.c infback.c inffast.c
                 inflate.c inftrees.c trees.c uncompr.c zutil.c)
list(TRANSFORM ZLIB_SOURCES PREPEND "${ZLIB_ROOT}/")
add_library(mc_zlib STATIC ${ZLIB_SOURCES})
target_include_directories(mc_zlib PUBLIC "${ZLIB_ROOT}")
target_compile_options(mc_zlib PRIVATE -w)

#
# libpng, in plain C (no SIMD filters), with the stock configuration header
#
set(PNG_SOURCES png.c pngerror.c pngget.c pngmem.c pngpread.c pngread.c pngrio.c
                pngrtran.c pngrutil.c pngset.c pngtrans.c pngwio.c pngwrite.c
                pngwtran.c pngwutil.c)
list(TRANSFORM PNG_SOURCES PREPEND "${PNG_ROOT}/")
set(PNG_CONFIG_DIR "${CMAKE_BINARY_DIR}/png_config")
file(MAKE_DIRECTORY "${PNG_CONFIG_DIR}")
configure_file("${PNG_ROOT}/scripts/pnglibconf.h.prebuilt" "${PNG_CONFIG_DIR}/pnglibconf.h" COPYONLY)
add_library(mc_png STATIC ${PNG_SOURCES})
target_include_directories(mc_png PUBLIC "${PNG_ROOT}" "${PNG_CONFIG_DIR}")
target_compile_definitions(mc_png PRIVATE PNG_INTEL_SSE_OPT=0)
target_compile_options(mc_png PRIVATE -w)
target_link_libraries(mc_png PUBLIC mc_zlib)

#
# RakNet: the real thing, with UDP and its own threads (no MC_WASM)
#
file(GLOB RAKNET_SOURCES "${RAKNET_DIR}/*.cpp")
add_library(raknet STATIC ${RAKNET_SOURCES})
target_include_directories(raknet PUBLIC "${RAKNET_DIR}")
target_compile_options(raknet PRIVATE -w)
target_link_libraries(raknet PUBLIC ws2_32 iphlpapi winmm)

#
# Game
#
file(GLOB_RECURSE GAME_SOURCES "${SRC_DIR}/*.cpp")

# Entry points and platform layers belonging to the other targets.
list(FILTER GAME_SOURCES EXCLUDE REGEX "/main_android.*\\.cpp$")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/main_dedicated\\.cpp$")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/main_rpi\\.cpp$")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/AppPlatform_win32\\.cpp$")
# Text entry: the web overlay is for the browser; this build has its own.
list(FILTER GAME_SOURCES EXCLUDE REGEX "/AppPlatform_sdl_dialog_web\\.cpp$")
# Audio: OpenSL ES is Android's, OpenAL is not available here yet, so this build
# takes the silent SoundSystem (see SoundEngine.h).
list(FILTER GAME_SOURCES EXCLUDE REGEX "/SoundSystemSL\\.cpp$")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/SoundSystemAL\\.cpp$")
# The vendored RakNet copy under src/ is built separately above.
list(FILTER GAME_SOURCES EXCLUDE REGEX "/src/raknet/")

add_executable(minecraftpe ${GAME_SOURCES})

target_include_directories(minecraftpe PRIVATE "${SRC_DIR}" "${RAKNET_DIR}")

target_compile_definitions(minecraftpe PRIVATE
    # WIN32 is what the MSVC project defined; the Windows branches of the platform
    # code (time, threads, folders) are keyed on it, not on the compiler's _WIN32.
    WIN32
    MC_SDL2
    MC_NATIVE
    NOMINMAX
    _USE_MATH_DEFINES
    # Where the asset tree is. An absolute path to the source tree for now; a
    # shipped build would put "data" beside the exe instead.
    MC_DATA_DIR="${DATA_DIR}"
)

# This code predates -Wall being survivable; keep real errors visible.
target_compile_options(minecraftpe PRIVATE -w)

target_link_libraries(minecraftpe PRIVATE
    raknet mc_png mc_zlib SDL2-static opengl32)

# One self-contained exe: no libc++/libunwind/winpthreads DLLs beside it.
target_link_options(minecraftpe PRIVATE -static -static-libgcc -static-libstdc++)

# Not in this build yet (the point of the feasibility run is to find out what a
# real one still needs):
#   * sound         -- needs OpenAL Soft (SoundSystemAL.cpp is written for it)
#   * text entry    -- AppPlatform_sdl_dialog_native.cpp answers for the player
#   * the asset path is absolute rather than relative to the exe
#   * chunk generation is still on the main thread, as on the web
