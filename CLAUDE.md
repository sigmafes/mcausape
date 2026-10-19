# Working on this repo

A web build of Minecraft Pocket Edition 0.6.1, running as WebAssembly through
Emscripten and published to GitHub Pages. Personal project — nothing about it
belongs in any client-facing tracker.

## Publishing

Pushes to `main` do not auto-deploy. `.github/workflows/pages.yml` is
`workflow_dispatch:`-only, so a push on its own publishes nothing — deploy with:

```sh
git push origin main
gh workflow run pages.yml --ref main
```

A run takes ~3 minutes. Check the live site, not just the green tick — a
successful run is a claim about a build, not about what the CDN is serving:

```sh
gh run list --workflow=pages.yml --limit 1 --json databaseId,status,conclusion,headSha
curl -sI https://sigmafes.github.io/mcausape/minecraftpe.wasm | grep -i last-modified
```

A `Last-Modified` from minutes ago is the evidence; so is grepping the served
page for a string that only exists in the new commit.

## Building

Needs [emsdk](https://emscripten.org/docs/getting_started/downloads.html),
pinned to **6.0.6** — `LEGACY_GL_EMULATION` is a lightly maintained corner of
Emscripten and the build is not worth letting drift.

```sh
source ~/emsdk/emsdk_env.sh
cd handheld/project/web
emcmake cmake -S . -B build
cmake --build build -j8
cd build && python3 -m http.server 8000   # http, not file://
```

## How the game decides it is on a phone

The page works it out **before the module loads** — a coarse primary pointer
with no hover — sets `window.mcpeTouch`, and `main_sdl.h` reads that back into
`g_touchscreen`. One source of truth, because the page needs the same answer for
its own layout and two detections that disagree are worse than one. Force it
either way with `?touch=1` or `?touch=0`; that is also how you reproduce a
phone-only bug in a desktop browser, alongside devtools device emulation.

## Input, and the trap in it

Three input paths come off `IInputHolder`, and **they are consumed in three
different places**. This matters because a bug in one leaves the other two
working, which reads as "touch works, one thing is broken" and sends you looking
in the wrong file:

| Input | Consumed by |
|---|---|
| `getMoveInput()` — d-pad | `player->input`, ticked with the player |
| `getBuildInput()` — tap to place/dig | `Minecraft::handleBuildAction` |
| `getTurnInput()` — drag to look | `MouseHandler::poll()`, called **only** from `GameRenderer::render` |

`GameRenderer::render` gates the turn block on `mc->mouseGrabbed || mc->useTouchscreen()`.
That `useTouchscreen()` half is load-bearing: `grabMouse()` returns early on a
touch build, so `mouseGrabbed` is false for the whole session and gating on it
alone means the camera cannot turn at all while everything else still works.

Do not "simplify" this by setting `mouseGrabbed = true` on touch. The flag has a
second meaning — `allowGuiClicks = !mouseGrabbed`, because a grabbed cursor is
parked and a dig click would otherwise also hit the hotbar — so that trade gives
you the camera and costs you the hotbar.

## RakNet (if multiplayer work resumes)

`handheld/src/raknet/` and `handheld/project/lib_projects/raknet/` are two
copies of RakNet and were byte-identical. The web build compiles the second;
the header the game includes comes from the first. **Patch both.**

`tools/wiretest/` covers the parts of this that cannot be checked by reading
them. Run `raknet.cpp` after touching anything under `#if defined(MC_WASM)`.

## Style

Match what is already here. Commit messages explain the **mechanism** — what was
actually happening and why the fix is the shape it is — and comments in the
platform seams say why a workaround exists, because the surrounding code is from
2013 and the seams are where a 2026 browser disagrees with it. The game's own
simulation, worldgen and rendering code is close to untouched; keep it that way
and fix things in the platform layer where you can.
