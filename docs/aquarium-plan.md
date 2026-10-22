# The Aquarium: plan de implementación

Una tercera dimensión, toda agua, a la que se llega por portales horizontales
de cristal oceánico, todos conectados a un único portal fijo en su centro. El
cristal se fabrica con fragmentos que suelta el calamar, que también se añade.

Decisiones tomadas:

| Tema | Decisión |
|---|---|
| Vuelta desde el Aquarium | Al **último portal del Overworld** que usó ese jugador. |
| Carga | **Solo mientras alguien está dentro**; se descarga al vaciarse. Lo mismo pasará a valer para el Nether. |
| Luz | **Más oscuro cuanto más hondo**, como un océano. |
| Fondo | Bedrock + **4 capas de arena**, con un poco de relieve; solo arena. |
| Prismarina | **Imagen fija** (adjunta). |
| Cristal oceánico | **4 fragmentos oceánicos → 1 cristal**; los fragmentos los suelta el **calamar**. |
| Criaturas en el Aquarium | **Ninguna** por ahora. |

## 1. El "aire" del Aquarium es agua

Llenar 512×512×128 bloques con agua de verdad serían 33 millones de bloques
de agua que dibujar, simular y guardar. En cambio el Aquarium **guarda aire**
como cualquier dimensión, y la dimensión declara que ese aire es agua:

- `Dimension::isAllWater()`: falso en las demás, verdadero en el Aquarium.
- Las preguntas de "¿hay agua aquí?" que ya existen tras el filtro acuático
  (`Level::checkAndHandleWater`, `containsMaterial`, `Entity::isUnderLiquid`,
  `LevelSource::hasWater`) responden **sí** para el aire de esa dimensión.
- Con eso, sin más: se nada (y se nada estilo Java), se ahoga uno, sale la
  vista bajo el agua, el fuego se apaga y no se puede encender.
- **Sin corriente ni simulación de líquido**: no hay bloques de agua que se
  extiendan. Cero lag por el agua.
- Bloques enteros "dentro del agua" sin hacer nada: romper uno deja aire, y
  allí el aire es agua. No necesita el filtro acuático.
- No se dibujan caras de agua (no hay ninguna). El ambiente lo da la niebla
  azul de bajo el agua, siempre activa en esa dimensión.
- Cubos: el vacío no recoge "agua" del aire (sería agua infinita) salvo que
  quieras lo contrario; uno lleno sí se puede vaciar (no hace nada, el agua ya
  está).

### Luz: más oscuro hacia el fondo

Aire "transparente" en lo que toca a la luz no oscurecería nada. Para que el
fondo sea más oscuro, en el Aquarium el aire resta luz de cielo como el agua
(1 nivel por bloque en lugar de 3, o el fondo, a 120 bloques, sería negro
total). Con 1 por bloque: luz 15 arriba, ~11 a 4 bloques… negro hacia la mitad.
Hay que **ajustar el número probando**: propongo restar 1 cada 8 bloques (luz
15 arriba, ~0 en el fondo, en degradado). Se hace en el cálculo de la columna
de luz de cielo (`LevelChunk::lightBlockAt`, `LightUpdate`) con la regla de la
dimensión, no con bloques.

## 2. La dimensión

| Pieza | Detalle |
|---|---|
| Id | 2 (`Dimension::AQUARIUM`). Overworld 0, Nether -1, como ahora. |
| Clase | `AquariumDimension` (como `HellDimension`): sin ciclo de día (luz fija), niebla azul verdosa, sin cielo ni nubes. |
| Generador | `AquariumLevelSource`: bedrock en y=0; arena de y=1 a y≈4 con un relieve suave (ruido de baja frecuencia, ±2 bloques); aire (agua) hasta arriba; techo de `invisible_bedrock` en y=127 (la barrera de los bordes). Sin cuevas ni población. |
| Archivo | `chunks_aquarium.dat` en la carpeta del mundo, como el Nether tiene el suyo. Ya con `RegionFile::acquire` (un solo escritor por archivo). |
| Portal fijo | Plataforma de prismarina en el centro (x=256, z=256, y≈120) con el marco y el portal ya encendido. Se construye la primera vez que se genera el Aquarium. Sus bloques son irrompibles. |

## 3. Bloques e ítems nuevos

| Cosa | Tipo | Textura | Cómo se obtiene |
|---|---|---|---|
| Prismarina | bloque entero (piedra, pico) | adjunta (fija) | Creativo; está en la plataforma del portal fijo (irrompible ahí). Ver preguntas. |
| Bloque de cristal oceánico | bloque entero, marco del portal | adjunta (bloque) | ver preguntas |
| Cristal oceánico | ítem | adjunta (gema) | 4 fragmentos oceánicos en la mesa (2×2) |
| Fragmento oceánico | ítem | adjunta | Lo suelta el calamar al morir |
| Portal del Aquarium | bloque sin colisión | el del Nether animado, teñido azul celeste verdoso, en horizontal | Se crea al encender el marco |

Bloques en casillas nuevas del atlas ampliado (256 en adelante); ítems en las
65 libres de `gui/items.png`.

## 4. El calamar

- Modelo, textura y animación (tentáculos que se abren y cierran al nadar) de
  `M2noa/MCLCE-Archive` (hay que añadir el repo a la sesión, en lectura).
- **Aparece en el agua del Overworld** (ríos, lagos, océanos), como en Java;
  no en el Aquarium (vacío por ahora).
- Nada solo en agua; fuera del agua se ahoga, como en Java.
- Al morir suelta **fragmentos oceánicos** (ver preguntas para cuántos).
- Pasivo, no ataca. Visible para visitantes como cualquier mob (sus
  posiciones ya viajan por la red).

## 5. El portal

- **Marco** horizontal, 4×4 sin esquinas, 8 bloques de cristal oceánico:

  ```
  . C C .
  C . . C
  C . . C
  . C C .
  ```

- **Encendido:** con el marco completo, vaciar un cubo de agua en uno de los
  4 huecos centrales los convierte en 4 bloques de portal. Se comprueba al
  verter el cubo (`BucketItem`), no con agua que corre.
- **Viaje de ida:** entrar en el portal (pisarlo, caer o nadar en él) lleva
  al portal fijo del Aquarium, con el mismo enfriamiento que el portal del
  Nether. El juego **guarda la posición de ese portal del Overworld** en el
  jugador (en su NBT, así que se guarda con el mundo y, para visitantes, con
  sus datos por nombre).
- **Vuelta:** entrar en el portal fijo lleva de vuelta a ese portal guardado.
  Si ya no existe (lo rompieron), se aparece junto a donde estaba. Si el
  jugador nunca usó uno (por ejemplo, llegó con un comando), al punto de
  aparición del mundo.

## 6. Cargar dimensiones solo cuando hay alguien (Aquarium y Nether)

Hoy el anfitrión tiene el Nether siempre cargado (se decidió así para evitar
el bajón de fps al entrar un visitante). Con tres dimensiones eso serían
~260 MB solo en chunks, justo donde ya hubo problemas de memoria. Así que las
dos dimensiones extra pasan a **cargarse al entrar alguien y descargarse
cuando se vacían**, pero **sin bajón de fps**:

- El nivel se crea al pedirlo, y sus chunks se cargan **repartidos en varios
  frames** (lo que hoy hace la precarga del Nether), mientras el que viaja ve
  una pantalla de carga.
- Primero los chunks alrededor del punto de llegada, para que el viajero
  pueda moverse enseguida; el resto, de fondo.
- Al vaciarse: se guarda y se descarga, liberando su memoria.
- En el anfitrión, `otherLevel` (uno) pasa a una lista de niveles por
  dimensión: `levelForDimension`, el viaje de visitantes y el guardado
  periódico se generalizan.

## Fases

1. **Dimensión base:** `AquariumDimension`, generador (bedrock, arena con
   relieve, techo), archivo propio, "el aire es agua", niebla, luz que
   oscurece. Se entra con un comando de depuración para probarla.
2. **Niveles bajo demanda:** lista de niveles en el anfitrión, carga
   repartida con pantalla de carga, descarga al vaciarse; aplicado también al
   Nether.
3. **Bloques e ítems:** prismarina, bloque de cristal oceánico, cristal,
   fragmento, receta 2×2, texturas.
4. **Portal:** marco, encendido con cubo, bloque de portal horizontal teñido,
   portal fijo con su plataforma, ida y vuelta al último portal.
5. **Calamar:** modelo/textura/animación de LCE, aparición en agua del
   Overworld, ahogarse fuera del agua, drop de fragmentos.
6. **Multijugador:** visitantes en el Aquarium y en el portal, pasando por
   los niveles bajo demanda.

## Preguntas que quedan

1. **Bloque de cristal oceánico vs. cristal (ítem):** los 4 fragmentos dan
   un cristal (la gema). ¿Cómo se obtiene el **bloque** del marco? Opciones:
   (a) 4 cristales en 2×2 → 1 bloque (recomendado: el marco cuesta 8 bloques
   = 32 cristales = 128 fragmentos, un objetivo largo), (b) 9 cristales → 1
   bloque, (c) el cristal (gema) **es** el bloque al colocarlo.
2. **Fragmentos por calamar:** ¿cuántos suelta? Propongo 1 a 3 (más con
   botín si algún día existe encantamiento).
3. **Prismarina en supervivencia:** ¿solo creativo por ahora, o con receta
   (en Java: 4 fragmentos de prismarina, que aquí no existen)?
