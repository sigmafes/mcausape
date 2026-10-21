# mptest

The multiplayer checks from `docs/multiplayer-plan.md`: a host tab and a
visitor tab, headless, through the real relay. They drive the game through the
`mcpe_test_*` hooks in `handheld/src/main_sdl.h`.

```sh
node tools/mptest/mptest.mjs                       # the Cloudflare deployment
node tools/mptest/mptest.mjs http://localhost:8000 # a local build
MPTEST_VERBOSE=1 node tools/mptest/mptest.mjs      # with the tabs' console
```

Checks: edits made before the visitor arrives (a block, a data value, a
glowstone) are there, in the Overworld and in the Nether (which the host makes,
writes out and drops before the trip); the block light under the glowstone; how
long until the visitor can play and until it has every chunk, on joining and on
each trip; and how long a live edit takes to reach the visitor with 100 mobs
about. Uses the Chromium Playwright finds (`PLAYWRIGHT_BROWSERS_PATH`).
