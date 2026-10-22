# The Aquarium: plan de implementación

Una tercera dimensión, toda agua, con su portal horizontal de cristal oceánico
y un único portal de llegada fijo en el centro.

## Idea central: el "aire" del Aquarium es agua

Rellenar 512×512×128 bloques con agua de verdad sería carísimo: 33 millones
de bloques de agua que dibujar, que calcular como líquido y que guardar. En
cambio, el Aquarium **guarda aire**, como cualquier dimensión, y es la
dimensión la que dice "aquí el aire es agua":

- `Dimension::isAllWater()` (falso en las demás, verdadero en el Aquarium).
- Las preguntas de "¿hay agua aquí?" que ya existen tras el filtro acuático
  (`Level::checkAndHandleWater`, `containsMaterial`, `Entity::isUnderLiquid`,
  `LevelSource::hasWater`) responden **sí** para el aire en esa dimensión.
- Resultado: se nada, se nada estilo Java, se ahoga uno, sale la vista bajo el
  agua, se apaga el fuego. **Sin corriente** y **sin simulación de líquido**:
  no hay bloques de agua que se extiendan, así que cero lag por agua.
- Los bloques enteros "dentro del agua" salen solos: un bloque roto deja aire,
  y el aire allí es agua. No hace falta la capa de filtro acuático.
- No se dibujan caras de agua (no hay agua que dibujar). El ambiente lo da la
  niebla azul de "bajo el agua", que ya existe, puesta siempre en esa
  dimensión.

## La dimensión

| Pieza | Detalle |
|---|---|
| Id | 2 (`Dimension::AQUARIUM`). Overworld 0, Nether -1, como ahora. |
| Clase | `AquariumDimension` (como `HellDimension`): sin ciclo de día si se elige así, color de niebla azul verdoso, sin cielo visible. |
| Generador | `AquariumLevelSource`: bedrock en y=0, arena en y=1–N, aire (agua) hasta arriba, y en y=127 un techo de `invisible_bedrock` (la barrera que ya existe en los bordes). Sin cuevas ni población, para empezar. |
| Archivo | `chunks_aquarium.dat` en la carpeta del mundo, como el Nether tiene el suyo. |

## Bloques nuevos

| Bloque | Textura | Notas |
|---|---|---|
| Prismarina | De `chrisGrando/MinecraftLCE-7thGen` (hay que añadir el repo a la sesión para leerlo). En Java es animada; ver preguntas. | Bloque entero de piedra. |
| Cristal oceánico | La imagen adjunta (16×16). | Bloque del marco del portal. |
| Portal del Aquarium | El del Nether (animado), teñido azul celeste verdoso, **horizontal**. | Como el portal del Nether: sin colisión, se rompe si se rompe el marco. |

Van en casillas nuevas del atlas ampliado (256 en adelante) y con ids de
bloque libres.

## El portal

- **Marco**, horizontal, 4×4 sin esquinas (8 bloques de cristal oceánico):

  ```
  . C C .
  C . . C
  C . . C
  . C C .
  ```

- **Encendido:** con el marco completo, vaciar un cubo de agua en uno de los
  4 huecos del centro los convierte en 4 bloques de portal. Se comprueba al
  verter el cubo (en `BucketItem`), no con agua que corre, para no tener que
  revisar el marco en cada tick de agua.
- **Viaje:** el jugador que entra en el portal (de pie, cayendo o nadando)
  viaja con el mismo mecanismo y el mismo enfriamiento que el portal del
  Nether.
- **Llegada al Aquarium:** siempre al **único portal fijo**, en el centro del
  mundo (x=256, z=256), cerca del techo (y≈120). Se construye la primera vez
  que se genera el Aquarium (plataforma de prismarina con el marco y el
  portal encendido) y no se puede romper.
- **Vuelta:** ver preguntas.

## Multijugador

- Hoy el anfitrión mantiene dos niveles a la vez (Overworld y Nether). Pasar
  a tres toca `levelForDimension`, `otherLevel` y el viaje de visitantes, que
  hoy asumen "el otro". Se generaliza a una lista de niveles.
- **Memoria:** cada dimensión cargada entera son ~85 MB. Con tres siempre
  cargadas serían ~260 MB solo en chunks, justo donde ya tuvimos problemas.
  El Aquarium casi no tiene nada (aire y arena), pero sus chunks ocupan lo
  mismo. Ver preguntas.
- Visitantes: entran y salen por el mismo camino que el Nether (el anfitrión
  envía los chunks del Aquarium, `ChunkDataPacket`, ya sin cambios).

## Fases

1. **Dimensión vacía:** `AquariumDimension`, generador (bedrock, arena,
   techo), archivo propio, "el aire es agua" para nadar, ahogarse, la niebla
   y el fuego. Se prueba entrando con un comando de depuración.
2. **Bloques:** prismarina, cristal oceánico, texturas, ítems en creativo.
3. **Portal:** marco horizontal, encendido con cubo, bloque de portal
   horizontal teñido, viaje de ida al portal fijo y de vuelta.
4. **Multijugador:** tres niveles en el anfitrión, visitantes en el Aquarium.

## Preguntas

1. **Vuelta desde el Aquarium:** al cruzar el portal fijo, ¿a dónde vuelve
   el jugador? Opciones: (a) al último portal del Overworld que usó (lo
   recomiendo), (b) al punto de aparición del mundo.
2. **Memoria / carga del Aquarium:** (a) cargarlo solo mientras alguien está
   dentro y descargarlo cuando se vacía (recomendado; la primera entrada
   tarda unos segundos), o (b) siempre cargado como el Nether (más memoria).
3. **Luz:** en el agua la luz baja con la profundidad. ¿El Aquarium debe ser
   (a) iluminado de forma pareja, sin oscurecer hacia el fondo (recomendado:
   se ve todo y es barato), o (b) oscuro en el fondo como un océano real?
4. **Terreno del fondo:** ¿solo arena plana, o con relieve (dunas, montículos
   de prismarina)? Para empezar propongo arena plana con un poco de relieve.
5. **Prismarina animada:** en Java cambia de color lentamente. ¿La quieres
   animada o con una sola imagen fija?
6. **Cómo se obtienen** la prismarina y el cristal oceánico en supervivencia:
   ¿solo en creativo por ahora, con receta de crafteo (¿cuál?), o generados
   en el Aquarium?
7. **Mobs:** ¿el Aquarium tiene criaturas (aparecen peces o calamares, que
   habría que crear), o vacío por ahora?
8. **Altura de la arena:** ¿cuántas capas de arena sobre la bedrock?
   Propongo 4.
