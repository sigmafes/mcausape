#!/bin/bash
# Copies a fresh web build into local-play/ so serve.js can hand it to the
# browser. minecraftpe.html has to become index.html: that is what the page
# loads at http://localhost:8137/.
set -e
SRC=/c/Users/Rosario/Desktop/test/handheld/project/web/build
DST=/c/Users/Rosario/Desktop/test/local-play

cp "$SRC/minecraftpe.html" "$DST/index.html"
for f in minecraftpe.js minecraftpe.wasm minecraftpe.data menu.mp3; do
	[ -f "$SRC/$f" ] && cp "$SRC/$f" "$DST/$f"
done

ls -lh "$DST"
