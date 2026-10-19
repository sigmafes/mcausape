#!/bin/bash
# Builds the WebAssembly target with the portable toolchain under ~/dev.
# Nothing here is installed system-wide; the paths are exported per-run only.
set -e

EMSDK_WIN='C:\Users\Rosario\dev\emsdk'
export EMSDK="$EMSDK_WIN"
export EM_CONFIG="$EMSDK_WIN\.emscripten"
export PATH="/c/Users/Rosario/dev/emsdk/upstream/emscripten:/c/Users/Rosario/dev/emsdk/upstream/bin:/c/Users/Rosario/dev/emsdk/ninja/1.13.2_64bit:/c/Users/Rosario/dev/emsdk/node/24.19.0_64bit:/c/Users/Rosario/dev/emsdk/python/3.13.3_64bit:/c/Users/Rosario/dev/cmake/cmake-4.4.4-windows-x86_64/bin:$PATH"

cd /c/Users/Rosario/Desktop/test

emcc --version
echo "--- configuring ---"
emcmake cmake -S handheld/project/web -B handheld/project/web/build "$@"
