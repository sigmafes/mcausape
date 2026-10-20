#!/usr/bin/env bash
# Cloudflare Pages build command for the static site. Mirrors
# .github/workflows/pages.yml step for step: install the pinned Emscripten,
# build the web target, assemble the files Pages serves into site/.
#
# Dashboard settings this script assumes:
#   Build command:    bash cloudflare/pages-build.sh
#   Build output dir: site
set -euo pipefail

# Cloudflare Pages runs build commands from the repository root.
REPO_ROOT="$(pwd)"
EMSDK_VERSION=6.0.6

git clone --depth 1 https://github.com/emscripten-core/emsdk.git /tmp/emsdk
cd /tmp/emsdk
./emsdk install "$EMSDK_VERSION"
./emsdk activate "$EMSDK_VERSION"
source ./emsdk_env.sh

cd "$REPO_ROOT/handheld/project/web"
emcmake cmake -S . -B build
cmake --build build -j"$(nproc)"

mkdir -p "$REPO_ROOT/site"
cp build/minecraftpe.html "$REPO_ROOT/site/index.html"
cp build/minecraftpe.js   "$REPO_ROOT/site/"
cp build/minecraftpe.wasm "$REPO_ROOT/site/"
cp build/minecraftpe.data "$REPO_ROOT/site/"
cp build/menu.mp3 "$REPO_ROOT/site/" 2>/dev/null || true
ls -lh "$REPO_ROOT/site"
