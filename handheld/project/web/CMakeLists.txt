cmake_minimum_required(VERSION 3.16)
project(mcpe061web CXX)

# Configure with emcmake so the Emscripten toolchain file is picked up:
#
#   source ~/emsdk/emsdk_env.sh
#   emcmake cmake -S . -B build && cmake --build build -j8
#
# Output lands in build/ as minecraftpe.{html,js,wasm,data}.

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This target must be configured through emcmake.")
endif()

# Same 2013-era C++03 as the other targets. Extensions stay on because the
# Emscripten headers use GNU variadic-macro behaviour in EM_ASM/EM_JS.
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(SRC_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/../../src")
set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../data")
set(RAKNET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib_projects/raknet/jni/RaknetSources")
set(SHELL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shell.html")

# SDL2, libpng and zlib all ship as Emscripten ports, so there is nothing to
# find_package() -- these flags fetch and build them on first use. They are
# needed at compile time (for the headers) as well as at link time.
set(EM_PORTS "-sUSE_SDL=2" "-sUSE_LIBPNG=1" "-sUSE_ZLIB=1")

#
# RakNet
#
# Browsers cannot open raw UDP sockets, and this build has no threads either, so
# two things RakNet takes for granted are missing. Both are handled inside RakNet
# under MC_WASM rather than around it: Startup() skips the thread creation the
# game loop now does by hand, and SocketLayer stops making descriptors nothing
# will ever call a syscall on. The traffic itself leaves through the
# SocketLayerOverride installed by WebRakNetInstance.
#
# MC_WASM has to reach this target too, not just the game: the guards are in
# RakPeer and SocketLayer, and RakPeer.h declares a member that only exists with
# it defined.
#
file(GLOB RAKNET_SOURCES "${RAKNET_DIR}/*.cpp")
add_library(raknet STATIC ${RAKNET_SOURCES})
target_include_directories(raknet PUBLIC "${RAKNET_DIR}")
target_compile_definitions(raknet PUBLIC MC_WASM)
target_compile_options(raknet PRIVATE -w -O2 ${EM_PORTS})

#
# Game
#
file(GLOB_RECURSE GAME_SOURCES "${SRC_DIR}/*.cpp")

# Entry points and platform layers belonging to the other targets.
list(FILTER GAME_SOURCES EXCLUDE REGEX "/main_android.*\\.cpp$")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/main_dedicated\\.cpp$")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/main_rpi\\.cpp$")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/AppPlatform_win32\\.cpp$")
# The desktop build's text-entry stand-in; the web build has its own overlay.
list(FILTER GAME_SOURCES EXCLUDE REGEX "/AppPlatform_sdl_dialog_native\\.cpp$")
# OpenSL ES is Android's audio backend; the web uses the OpenAL one, which
# Emscripten implements over WebAudio.
list(FILTER GAME_SOURCES EXCLUDE REGEX "/SoundSystemSL\\.cpp$")
# The vendored RakNet copy under src/ is built separately above.
list(FILTER GAME_SOURCES EXCLUDE REGEX "/src/raknet/")

add_executable(minecraftpe ${GAME_SOURCES})

target_include_directories(minecraftpe PRIVATE
    "${SRC_DIR}"
    "${RAKNET_DIR}"
)

if(NOT DEFINED MC_DEPLOY_NUMBER)
    set(MC_DEPLOY_NUMBER 0)
endif()

target_compile_definitions(minecraftpe PRIVATE
    MC_WASM
    MC_SDL2
    # The deploy workflows set this to the number of commits on main.
    MC_DEPLOY_NUMBER=${MC_DEPLOY_NUMBER}
    # Where the preload package below mounts the asset tree inside the VFS.
    MC_DATA_DIR="/data"
)

# This code predates -Wall being survivable; silence the noise so real errors
# stay visible.
# Optimised: without a level Emscripten builds at -O0, several times slower.
target_compile_options(minecraftpe PRIVATE -w -O2 ${EM_PORTS})

target_link_options(minecraftpe PRIVATE
    ${EM_PORTS}

    # WebGL has no fixed-function pipeline. This emulates GLES 1.1 on top of it.
    # GL_FFP_ONLY is valid because the game never binds a shader of its own, and
    # it lets the emulation skip its shader-interop bookkeeping.
    -sLEGACY_GL_EMULATION=1
    -sGL_FFP_ONLY=1

    # The 3.8M asset tree is baked into a .data file fetched alongside the wasm.
    "--preload-file" "${DATA_DIR}@/data"

    # Saves are mounted from IndexedDB at runtime; see main_sdl.h.
    -lidbfs.js
    -sFORCE_FILESYSTEM=1

    # Chunk buffers and the terrain atlas make this hungrier than the default
    # 16MB heap, and worldgen is recursive enough to overflow the 64KB stack.
    -sINITIAL_MEMORY=268435456
    -sALLOW_MEMORY_GROWTH=1
    # Up to wasm32's whole 4GB instead of the default 2GB.
    -sMAXIMUM_MEMORY=4294967296
    -O2
    -sSTACK_SIZE=4194304

    # _malloc/_free are called from the EM_JS prompt bridge in the web dialog.
    -sEXPORTED_FUNCTIONS=_main,_malloc,_free
    -sEXPORTED_RUNTIME_METHODS=FS,IDBFS,UTF8ToString,stringToUTF8,lengthBytesUTF8,HEAPU8

    -sENVIRONMENT=web
    "--shell-file" "${SHELL_FILE}"

    # Emit minecraftpe.symbols next to the output so wasm stack traces from the
    # browser console (wasm-function[N]) can be resolved to C++ names.
    "--emit-symbol-map"

    # Function names in the wasm itself (its name section), so a crash in the
    # browser console reads LevelChunk::setTileAndData instead of 0x23d3d5,
    # with nothing to look up. Costs a little size, not speed.
    "--profiling-funcs"
)

set_target_properties(minecraftpe PROPERTIES
    SUFFIX ".html"
    LINK_DEPENDS "${SHELL_FILE}"
)

# The music (shell.html's TRACKS), copied next to the output rather than added
# to the --preload-file package above: nothing needs it until the menu is on
# screen, so streaming it keeps the wait before the game appears as it was.
add_custom_command(TARGET minecraftpe POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/music" "$<TARGET_FILE_DIR:minecraftpe>/music")

target_link_libraries(minecraftpe PRIVATE raknet)
