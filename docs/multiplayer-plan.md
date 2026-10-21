# Multijugador: diagnóstico y plan de implementación

Estado al Deploy 59. Este documento explica por qué el multijugador (host +
visitantes, dos dimensiones) falla de formas que parecen distintas pero tienen
pocas causas, y el orden en que se arregla de forma definitiva.

## Cómo funciona hoy

1. El visitante recibe `StartGamePacket` (semilla, dimensión, posición).
2. **El visitante genera el mundo entero por su cuenta** a partir de la semilla:
   `Minecraft::prepareLevel` recorre las 1024 columnas de chunks (32×32) y
   asienta la luz de cada una. No hay rama `isClientSide` que lo evite.
3. Recién entonces pide los chunks al host, de a 16 en vuelo, uno por respuesta.
4. Por cada chunk, el host manda **solo las secciones marcadas en
   `updateMap`** (lo que cambió desde que se generó). El visitante las aplica
   bloque por bloque encima de lo que generó él (`ChunkDataPacket`). El Nether,
   desde el Deploy 59, va completo.
5. La luz **no viaja nunca**: cada visitante la calcula.
6. Cuando llega el último chunk, el visitante avisa (`READY_REQUESTEDCHUNKS`) y
   recibe tile entities. Cambiar de dimensión repite los pasos 1 a 6 enteros.

## Causas raíz de los síntomas

| Síntoma | Causa | Dónde |
|---|---|---|
| El visitante ve el mundo "limpio" o distinto (sobre todo el Nether) | El terreno base lo genera el visitante y solo se envían diferencias. Si la generación del visitante no sale idéntica a la del host, nadie lo corrige. La población del Nether (fortalezas, lava que fluye, fuego) no sale idéntica. | `ChunkDataPacket.h`, `Minecraft::prepareLevel*` |
| Cambios hechos antes de entrar no aparecen | `updateMap` se pierde: (a) `ChunkCache::postProcess` lo borraba entero al poblar un chunk, también las ediciones previas (arreglado en el Deploy 58, pero **los mundos guardados antes ya perdieron esas marcas**); (b) `LevelChunk::setData` no marca nada, así que puertas abiertas, cultivos crecidos o la orientación de un horno no viajan. | `ChunkCache.h`, `LevelChunk.cpp:384` |
| Luz incorrecta (glowstone, lava y fuego no iluminan) | La luz nunca se envía. Los bloques recibidos se aplican con `setTileNoUpdate` en un orden en que los vecinos aún no existen, y el chunk completo del Nether se copia sin luz. El parche actual relee la luz de 6 chunks por tick (≈9 s a oscuras). | `ClientSideNetworkHandler.cpp` |
| Entrar o cambiar de dimensión tarda mucho | El visitante genera y alumbra 1024 chunks localmente **antes** de pedir el primero, y luego hace 1024 viajes de ida y vuelta. Un cambio de dimensión lo repite todo. | `Minecraft::prepareLevelChunkStep`, `ClientSideNetworkHandler::levelGenerated` |
| Cola de paquetes, acciones con minutos de retraso | Hay cuatro caminos para enviar (`rakPeer->Send` directo, `raknetInstance->send`, `LevelNet`, `redistributeIn`) y solo uno aplicaba la regla de la web (movimiento no fiable y de baja prioridad). Ya se rompió dos veces (`fce3bbf` y el `LevelNet` original). La fiabilidad y el orden de cada paquete dependen de quién lo envía. | `WebRakNetInstance::sendClassified`, `ServerSideNetworkHandler` |
| Skins que no aparecen, "joined the game" de más, etc. | Paquetes dependientes (la skin necesita al jugador) sin garantía de orden entre sí. | mismo |
| El host se traba cuando un visitante entra al Nether | El host crea el segundo `Level` y genera sus chunks en el hilo principal, a pedido de los paquetes del visitante. | `Minecraft::levelForDimension`, `handle(RequestChunkPacket)` |

La conclusión es que el diseño de 0.6.1 (cada cliente genera su mundo y recibe
diferencias) supone una generación determinista y una LAN rápida. En la web no
se cumple ninguna de las dos, y cada arreglo puntual tapa un caso y deja otro.

## Plan

Cada fase se compila, se despliega y se prueba por separado. El orden importa:
la fase 0 es la que permite saber si las demás funcionan sin depender de
pruebas manuales.

### Fase 0 — Banco de pruebas (antes de tocar nada)

- Script con Playwright: dos pestañas sin interfaz (host y visitante) contra el
  relay real, con exports de depuración como los de `8d77b7c`.
- Comprobaciones automáticas:
  1. Bloque colocado por el host antes de que entre el visitante: visible.
  2. Lo mismo en el Nether.
  3. Valor de luz de bloque junto a una glowstone en el visitante = el del host.
  4. Tiempo hasta poder moverse al entrar y al cambiar de dimensión.
  5. Retraso host→visitante de un bloque colocado con 100+ mobs cerca.
- Imprime una tabla. Cada fase siguiente se acepta solo si la tabla mejora.

### Fase 1 — El host manda el mundo; el visitante no genera nada

- Nuevo formato de `ChunkDataPacket`: el chunk **completo** (ids, datos, luz de
  cielo y de bloque, 80 KB en crudo) comprimido con zlib (ya enlazado:
  `-sUSE_ZLIB=1`). Se esperan unos 5-15 KB por chunk.
- El visitante usa una fuente de chunks vacía: crea chunks en blanco y los
  rellena con lo que llega. Se elimina `prepareLevel` para niveles
  `isClientSide`.
- `updateMap` deja de importar para la red (se mantiene para el guardado). Con
  esto desaparecen las causas (a) y (b) de la tabla, la diferencia de
  generación del Nether y el código de "chunk completo del Nether" del
  Deploy 59.
- Al recibir un chunk se marca poblado y no se recalcula su luz.

### Fase 2 — Entrada por streaming

- El host ordena los chunks por distancia al jugador y los **empuja** él, con
  una ventana medida en bytes en vuelo, en lugar de esperar 1024 pedidos.
- El visitante puede moverse en cuanto tiene un anillo de radio 4 (81 chunks)
  alrededor. El resto sigue llegando en segundo plano, del más cercano al más
  lejano.
- Los cambios de bloque de un chunk que todavía no llegó se guardan y se
  aplican cuando llega (ya implementado). Las tile entities de cada chunk
  viajan con su chunk, no al final.
- El cambio de dimensión usa el mismo camino: con la fase 1 ya no hay
  generación local, así que es solo esperar el anillo.

### Fase 3 — Un solo camino de envío

- Una tabla única por tipo de paquete: fiabilidad, prioridad y canal de orden.
  - Canal 0, ordenado: estado del mundo (chunks, bloques, añadir/quitar
    entidades y jugadores, skins).
  - No fiable secuenciado: posición y movimiento de entidades.
  - Canal 1, ordenado: chat y mensajes.
- Todo envío del host (`LevelNet`, `redistributeIn`, listeners, `ServerPlayer`)
  pasa por **una** función que consulta la tabla. Prohibido `rakPeer->Send`
  fuera de ella. Esto vuelve imposibles las regresiones de `fce3bbf`.
- `Level::setData` emite el cambio a los listeners (hoy los cambios de solo
  datos no viajan aunque el bloque ya esté cargado).

### Fase 4 — Dimensiones en el host

- El segundo `Level` se crea fuera del paquete que lo pide, con pantalla de
  carga para el visitante, y genera sus chunks repartidos en varios frames.
- Mundos creados antes de que existiera la pre-generación del Nether: se
  genera la primera vez que alguien lo necesita, repartido en el tiempo, y se
  guarda.
- Guardado periódico de entidades del segundo nivel (hoy solo al cerrarlo).
- Probar con el banco: host y visitante en dimensiones distintas, alternando.

### Fase 5 — Sesión

- Desconexión: detectar la salida del host con un tiempo de espera corto de
  RakNet además del aviso explícito, y mostrar la pantalla siempre.
- Opcional: guardar inventario y posición del visitante por nombre (en la
  carpeta del mundo), atado a `/register`. Es lo que hace que la autenticación
  proteja algo más que el nombre.

## Qué se elimina al terminar

- El chunk completo del Nether por RLE (Deploy 59): queda cubierto por la
  fase 1.
- La pasada de luz de 6 chunks por tick: la luz viene del host.
- `prepareLevel` en el visitante.
- El seguimiento de `updateMap` para la red.

## Riesgos

- **Volumen por el relay.** 1024 chunks × ~10 KB ≈ 10 MB por entrada. Con
  streaming no se espera todo, pero hay que medirlo en la fase 0. Si es
  demasiado, se deja la luz fuera del paquete y el visitante la reconstruye
  desde el heightmap y las fuentes de luz (más barato que generar el mundo).
- **Coste de compresión en el host.** zlib en el hilo principal: se limita la
  cantidad de chunks comprimidos por frame.
- **Versión del protocolo.** Cada fase que cambia paquetes la sube; las
  pestañas viejas verán "outdated" hasta recargar.
