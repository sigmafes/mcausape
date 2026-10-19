# Plan de implementación — Fase 1: "Nether navegable" (MCPE 0.6.1 WASM)

Documento vivo. Estado al día de hoy: **WP0 completado**, **WP1 completado**,
**WP2 completado y verificado en juego** (mundo nuevo → Nether navegable; se incluyó el
fix del crash de spawn, ver WP2), **WP3 completado y en el build instalado (falta la
verificación visual del usuario)**, **WP4 con código escrito (sin probar en juego)**.

## 0. Definición y decisiones fijadas

**Criterio de éxito de la fase:** un jugador puede encender un portal de obsidiana con
flint & steel, viajar al Nether (netherrack, mar de lava, techo/suelo de bedrock,
glowstone, soul sand, cuevas, cielo rojo sin nubes), moverse, volver por el portal al
Overworld, y todo persiste al recargar la página.

| # | Decisión | Valor |
|---|---|---|
| D1 | Id de dimensión | `Dimension::NETHER = 1` (convención Bedrock); clase `HellDimension` |
| D2 | Estrategia de viaje | Swap de `Level` (destruir/recrear), reutilizando `fixedDimension` del ctor y la generación asíncrona web |
| D3 | Archivos por dimensión | `chunks.dat` (Overworld) / `chunks_nether.dat` (Nether), parametrizando `RegionFile` |
| D4 | Escala de coordenadas | 1:8 con clamp al mundo finito de 256×256 (como 1.0) |
| D5 | Persistencia de dimensión | Campo `Dimension` (int) en el NBT de `level.dat` + posición del jugador donde se guarda hoy |
| D6 | Alcance fase 1 | Sin mobs, fuego, sonidos ni fortalezas (fase 2). Soul sand se registra simple (sin física) |
| D7 | Modo de prueba | Retirado (WP4): `DEV_NEW_WORLDS_IN_NETHER` ya no existe; los mundos nuevos vuelven a ser Overworld |

**Valores del Nether** (verificados en la referencia MCPE 1.0.0): mar de lava y=32;
`ultraWarm`+`hasCeiling`; sky light mínimo; rampa de brillo con ambiente 0.10; tiempo
fijo 0.5; niebla RGB(0.2, 0.03, 0.03) a ×0.4-0.5 de distancia; `mayRespawn=false`.

## 1. Work packages

### [x] WP0 — Preparación
- Backup (sin git) en `%TEMP%\opencode\backup_nether_wp0.zip` (src + tools + data/images).
- Documento del plan (este archivo).

### [~] WP1 — `HellDimension` + `HellBiome` + mundo de prueba
**Nuevos**: `dimension/HellDimension.h/.cpp`, `biome/HellBiome.h`, `biome/HellBiomeSource.h`.
**Modificados**: `Dimension.h/.cpp` (const `NETHER`, case en `getNew`), `Biome.h/.cpp`
(registro `Biome::hell`), `BiomeInclude.h`, `Level.cpp` (flag D7), `LevelRenderer.cpp`
(clouds `foggy`), `GameRenderer.cpp` (fog end).

- [x] HellDimension con foggy/ultraWarm/hasCeiling, rampa 0.10, tiempo 0.5, niebla propia.
- [x] HellBiome (sin mobs, sin vegetación) + registro.
- [x] HellBiomeSource (bioma fijo + temperatura constante).
- [x] Flag D7 (mundo nuevo → Nether).
- [x] Build + instalación en `local-play` (registro de fuente por GLOB; `cmake -S . -B build`
      + `cmake --build`, copia a `local-play`). Compila sin errores; la página arranca sin
      errores nuevos en consola.
- [ ] Prueba visual (usuario): mundo nuevo → cielo/fondo rojo oscuro, sin nubes, oscuro
      (sin luz de cielo), niebla densa, navegable; mundos viejos cargan normal.
      *(Se verifica junto con WP2: el mundo de prueba ahora genera terreno real del Nether;
      usar un mundo NUEVO para evitar mezcla con el terreno temporal de WP1.)*
- Nota: `createRandomLevelSource()` devuelve temporalmente el `RandomLevelSource`
  normal hasta WP2 (permite probar la dimensión sin generador propio).
- Nota (automatización): la ventana del navegador ocluida hace que Chrome limite a
  1 fps; a esos fps el juego descarta el input (guard de 200 ms en `Minecraft::update`),
  así que la verificación visual debe hacerse con la ventana en primer plano.

### [x] WP2 — `HellRandomLevelSource` (el grueso)
**Nuevos**: `levelgen/HellRandomLevelSource.h/.cpp`, `levelgen/LargeHellCaveFeature.h/.cpp`,
`feature/GlowStoneFeature.h`, `feature/HellSpringFeature.h`, `tile/HellSandTile.h`.
**Modificados**: `HellDimension::createRandomLevelSource`, `Tile.cpp` (registro soul sand
88/tex 104 + definición del puntero), `TileInclude.h`, `feature/FeatureInclude.h`.
1. [x] Esqueleto espejo de `RandomLevelSource` (mismo contrato `ChunkSource`, mapa de chunks).
2. [x] `prepareHeights`: mar de lava estático por debajo de y=32; netherrack con ruido
   3D (ruido de 1.0: `perlinNoise1` escala `s/80, hs/60`, `hs = 3s`); interpolación
   idéntica al overworld.
3. [x] `buildSurfaces`: bedrock en suelo (y≤0..4) y techo (y≥123..127); parches de
   soul sand y gravel alrededor de y=64 (`waterHeight`, como 1.0); superficies de lava
   estática debajo de y=64; profundidad de corridas con `perlinNoise3`.
4. [x] Cuevas: `LargeHellCaveFeature` (portado de 1.0): solo excava netherrack, no
   atraviesa lava, techo a y≤120, escala vertical 0.5, densidad `nextInt(10)+1…`.
5. [x] `GlowStoneFeature`: cúmulos de glowstone colgando de netherrack (1500 intentos,
   solo junto a 1 vecino ya colocado).
6. [x] `HellSpringFeature`: manantiales de lava (rodeados de netherrack, 4/5 roca + 1
   hueco) con instaTick para que fluyan.
7. [x] `getMobsAt` sin mobs (bioma hell vacío). Fuego, fortalezas, quartz: fuera
   (quartz no existe en 0.6.1; fuego/fortalezas fase 2/3).
8. [x] Setas marrón/roja sobre netherrack (oscuro): `Mushroom::canSurvive` ya lo permite.
9. [x] Build limpio e instalado en `local-play`; servidor relanzado en :8137.
- Diferencias conscientes vs 1.0: sin fortalezas/quartz/fuego; RNG con la variante local
  (`nextFloat/nextLong` en vez de `nextGaussianFloat`); cuevas aplicadas siempre (el
  overworld las tiene desactivadas y transpuestas en `buildSurfaces` — no se tocó).
- [x] Fix crash de spawn (`memory access out of bounds` durante "Building terrain"):
  `Dimension::isValidSpawn` hacía `Tile::tiles[topTile]->isSolidRender()` sin comprobar
  NULL; en el Nether `getTopTile` devuelve a menudo 0 (aire) porque `Tile::tiles[0]` no
  existe → puntero NULL. Ahora devuelve `false` (el spawn busca otra columna). Localizado
  simbolizando el stack wasm con `--emit-symbol-map` (flag añadido al link en CMakeLists
  del target web: genera `minecraftpe.html.symbols` para traducir `wasm-function[N]`).
- [x] Verificación visual (usuario): mundo NUEVO genera netherrack/lava/glowstone/etc.
  y es navegable.

### [x] WP3 — Portal e ignición
**Nuevos**: `tile/PortalTile.h/.cpp` (PortalTile + `PortalShape` portada de 1.0),
`data/images/custom/portal.png`.
**Modificados**: `Tile.h/.cpp` + `TileInclude.h` (id **90**, tex 106), `FlintAndSteelItem.h`
(`useOn`), `Inventory.cpp` (mechero en la lista creativa), `pack_custom_textures.ps1`
(sección de celdas solo-terreno).
1. [x] `PortalTile`: `HalfTransparentTile` (capa BLEND), luz 11, sin colisión, no
   "pickable" (los clics lo atraviesan), sin drops; data = eje (1=X, 2=Z).
2. [x] `trySpawnPortal`: marco de obsidiana 2×3…21×21 — misma `PortalShape` que 1.0
   (techo de obsidiana obligatorio; `numPortalBlocks == width*height` en el chequeo).
3. [x] Creación de bloques con `setTileAndDataNoUpdate` para que `neighborChanged` no
   destruya el portal a medio construir (equivale a UPDATE_CLIENTS en 1.0).
4. [x] `FlintAndSteelItem::useOn`: clic en obsidiana → `trySpawnPortal` en la celda
   adyacente; consume durabilidad (en creativo no). Sin fallback a fuego: `FireTile`
   tiene toda su lógica desactivada (`//@fire`).
5. [x] Textura del portal: vórtice púrpura 16×16 generado y empaquetado en la celda
   (10,6) = índice 106 (comprobado en el atlas; la fila 6 solo tenía 103/104/105).
6. [x] Build OK y copiado a `local-play`.
- Nota de implementación: `HalfTransparentTile.h` no incluye `Tile.h` (depende del orden);
  `PortalTile.h` lo incluye explícitamente. `PortalTile.cpp` incluye `FireTile.h` para
  poder comparar `Tile::fire->id`.
- Prueba manual pendiente: marco de obsidiana (interior 2×3), clic con el mechero →
  aparecen los bloques del portal; romper un bloque del marco → el portal desaparece.
Referencia: `MCPE-1.0.0-Restored` (`PortalBlock.h`).

### [~] WP4 — Viaje + persistencia (el riesgo) — código escrito, sin probar en juego
- [x] Build fusionado WP3+WP4 compila limpio y está instalado en `local-play`
      (17-10 15:59); prueba en juego pendiente.
Paso 0: entender ciclo de red/teardown (`leaveGame`, `netCallback`, `hostMultiplayer`).
- 4a. Storage por dimensión (`RegionFile` parametrizable; `createChunkStorage(Dimension*)`
  devuelve storage propio por dimensión, propiedad del `LevelStorage`).
- 4b. Persistencia de `Dimension` en `level.dat` + `selectLevel` respeta la dimensión.
- 4c. `Minecraft::changeDimension(int)`: snapshot del jugador → saveAll + saveLevelData +
  sync IDBFS → teardown (sin menú) → nuevo `Level` con `fixedDimension` →
  transferencia en `_levelGenerated` (inventario + posición 1:8) → portal de llegada
  (buscar o construir) → cooldown anti-rebote.
- 4d. `PortalTile::entityInside` dispara el viaje.
- Prueba: ida/vuelta ×2, inventario intacto, F5 en el Nether, sin corrupción de chunks.

### [ ] WP5 — Pulido y cierre
- Regresión del Overworld, quitar flag D7, actualizar este documento.
- Fuera de fase 1: fuego funcional, spawns de pigman, sonidos, partículas, fortalezas,
  nether wart/fence, ghast, física de soul sand, animación del portal.

## 2. Estimación

| WP | Tamaño | Estado |
|---|---|---|
| WP0 | XS | hecho |
| WP1 | S | hecho |
| WP2 | L | hecho (verificación visual pendiente) |
| WP3 | M | hecho (verificación visual pendiente) |
| WP4 | L (riesgo) | pendiente |
| WP5 | S | pendiente |

## 3. Riesgos principales
- Ciclo de red (`netCallback`) en el swap → paso 0 de WP4.
- Spawn del Nether (`validateSpawn`) → `isValidSpawn` base ya lo tolera (bedrock es sólido);
  llegada real por plataforma/portal en WP4.
- Pérdida de datos al destruir el Level → guardar + sync IDBFS antes del teardown.
- Saves viejos: `Dimension` ausente ⇒ default 0; `chunks_nether.dat` se crea al primer viaje.
- ~~Spawn del Nether~~ resuelto: `Dimension::isValidSpawn` con guarda de NULL (WP2).

## 4. Build / prueba
Desde `handheld\project\web` con el entorno emsdk del proyecto:
`cmake -S . -B build` (los `.cpp` nuevos entran por GLOB) → `cmake --build build` →
copiar a `local-play` (`build-web.sh`/`install-web.sh`); si cambia `data/images` → borrar
`build\minecraftpe.data` antes. Juego en http://localhost:8137/ (`start-game.bat`).
