# Bloques con filtro acuático (waterlogging): plan de implementación

Es lo que Java Edition llama *waterlogged* (1.13): un bloque que no ocupa
todo su espacio puede estar dentro de un bloque de agua. Se ve el agua
alrededor, el jugador nada ahí y, al romper el bloque, queda el agua.

## Estado

Implementado: capa de datos, guardado y red (protocolo 14); colocar en una
fuente, romper deja la fuente, cubos lleno y vacío, losa doble saca el agua;
bloques que el agua rompe (no se colocan en agua, el agua que corre los
arrastra; cuadros incluidos); el agua del bloque alimenta y extiende el agua
de alrededor, se nada y se ahoga ahí, vista bajo el agua; dibujo del agua
dentro del bloque sin caras entre agua.

Después: luz (un bloque con agua resta lo que resta el agua), fuego (no
arde ni alimenta un fuego; las entidades se apagan dentro) y el agua que
corre también llena un bloque al colocarlo, no solo una fuente.

## Cómo funciona hoy

- Cada posición guarda **un id (8 bits) y un dato (4 bits)**. El agua es un id
  más, así que poner una losa dentro del agua reemplaza el agua por la losa, y
  al romperla queda aire.
- No hay ningún bit libre para "esta losa además tiene agua": los 4 bits de
  dato ya los usan escaleras, puertas, losas, etc.

## Diseño

### El dato nuevo: una capa de agua por chunk

- Cada chunk gana una capa de **1 bit por bloque** (32.768 bloques = 4 KB):
  `waterlogged`. Va en `LevelChunk` junto a `data`, `skyLight` y
  `blockLight`, con `Level::isWaterlogged(x,y,z)` y `setWaterlogged(...)`.
- Solo cuenta si el bloque está en la lista de bloques con filtro. Cualquier
  otro bloque que se ponga ahí borra el bit.
- **Siempre es agua quieta (fuente)**, como en Java: solo una fuente llena un
  bloque. El agua que corre no llena; la reemplaza, como hoy.

### Guardado (mundos viejos siguen funcionando)

- `ExternalFileLevelStorage` escribe la capa **al final** de cada chunk. Al
  leer, si no queda nada que leer (chunk de antes de esto), la capa queda en
  cero. No hace falta convertir mundos ni subir la versión de guardado.
- El chunk crece 4 KB en crudo, pero casi todo son ceros.

### Red

- `ChunkDataPacket` añade la capa al bloque comprimido (zlib la deja en unos
  pocos bytes).
- `UpdateBlockPacket` ya manda el dato en un byte, del que solo se usan 4 bits:
  el bit `0x10` pasa a ser "con agua". Sin paquetes nuevos.
- Sube la versión del protocolo (14): el anfitrión y los visitantes tienen que
  recargar la página.

## Reglas

### Bloques con filtro (se meten en el agua y la conservan)

Escaleras (todas), losas (simples; la doble es un bloque entero y saca el
agua), vallas, puertas de valla, puertas (cada mitad por separado), vidrio,
panel de vidrio, muros de piedra, trampillas, cofre, cama (las dos mitades),
mesa de encantamiento, soporte de pociones, carteles (de pie y de pared) y
hojas.

Se marcan con un `Tile::canBeWaterlogged()` virtual (falso por defecto) que
devuelven verdadero solo estos bloques.

### Colocar

- Se coloca **en** una fuente de agua → el bloque ocupa la posición y se
  marca el bit. Esto cubre `TileItem` y los ítems que colocan a su manera:
  `DoorItem`, `BedItem`, `SignItem` y las losas.
- **Cubo de agua sobre un bloque con filtro** → se le pone el bit (en Java se
  "llena" la valla o la losa) y el cubo queda vacío.
- **Cubo vacío sobre un bloque con agua** → se le quita el bit y el cubo se
  llena.

### Romper

- Al romper un bloque con el bit puesto queda **una fuente de agua** en su
  lugar, no aire. Va en `Level` (al quitar el bloque) y no en cada tipo de
  bloque, para que lo cubran también las explosiones, los pistones (aquí no
  hay) y el host rompiendo por un visitante.

### Bloques que se rompen dentro del agua

Escalera de mano, antorchas, flores, hongos, cactus, brotes y cuadros.

- **Colocar** uno en una fuente o en agua que corre: no se coloca. El ítem
  sigue en la mano, como en Java cuando el agua no deja.
- **Agua que llega** a uno que ya estaba: se rompe y suelta el ítem, como ya
  pasa hoy con algunos (`LiquidTile` y su lista de bloques que el agua
  arrastra). Se completa esa lista con los que falten.
- El cuadro no es un bloque, es una entidad: se comprueba al colgarlo y se
  suelta si el agua llega a su posición.

## El agua de un bloque con filtro tiene que comportarse como agua

Esta es la parte larga: en todo el juego "¿hay agua aquí?" se pregunta como
"¿el bloque de aquí es de material agua?", y una losa con agua no lo es. Hay
que sumar el bit en cada sitio que importe, **sin** cambiar `getMaterial`
(eso haría que la losa dejara de ser sólida para todo lo demás).

| Qué | Dónde | Efecto |
|---|---|---|
| El agua se extiende desde ahí | `LiquidTile` (vecinos y conteo de fuentes) | El bloque con agua cuenta como fuente: el agua fluye hacia abajo y a los lados desde él, y dos fuentes vecinas crean otra. |
| El agua no entra "dentro" del bloque desde un lado | `LiquidTile` | El agua que corre no reemplaza ni llena un bloque con filtro (en Java tampoco). |
| Nadar, flotar, empuje | `Level::checkAndHandleWater`, `containsMaterial` | Estar en un bloque con agua es estar en el agua: se nada, se aplica la corriente, se puede nadar estilo Java. |
| Ahogo y vista bajo el agua | `Entity::isUnderLiquid` | Con los ojos dentro de un bloque con agua: burbujas, niebla azul, sonido. |
| Apagar fuego | `Entity` / `FireTile` | Igual que en el agua normal. |
| Luz | `Level::getLightBlock` / la propagación de luz | El agua resta luz; el bloque con agua resta como mínimo lo mismo que el agua. |
| Mobs | pathfinding | El calamar puede estar ahí y los mobs lo tratan como agua. Prioridad baja. |

## Dibujo

- En la reconstrucción de un chunk (`Chunk::rebuild`) se dibuja primero el
  bloque, en su capa, y luego **el agua en la misma posición**, en la capa
  transparente del agua, con el renderizador de líquidos que ya existe (como
  agua quieta, nivel lleno).
- Caras: entre un bloque con agua y una fuente de agua vecina, o entre dos
  bloques con agua, no se dibuja la cara del agua (si no, se verían planos de
  agua dentro del agua). Se ajusta `LiquidTile::shouldRenderFace`.
- La cara superior del agua solo se dibuja si encima no hay agua, como hoy.

## Fases

1. **Capa de datos, guardado y red.** El bit en `LevelChunk`, el guardado con
   compatibilidad hacia atrás, `ChunkDataPacket` y el bit `0x10` de
   `UpdateBlockPacket`. Sin reglas todavía; se prueba poniendo el bit a mano.
2. **Colocar y romper.** `canBeWaterlogged`, colocar en una fuente, romper y
   dejar la fuente, los cubos, y los bloques que no se pueden colocar en el
   agua.
3. **Comportamiento del agua.** Que el bloque con agua extienda el agua, que
   el jugador nade y se ahogue ahí, la vista bajo el agua y la luz.
4. **Dibujo.** El agua dentro del bloque y las caras ocultas entre agua.
5. **Multijugador.** Que un visitante vea y rompa bloques con agua y que todo
   le llegue igual que al anfitrión (incluido grief off, que le devuelve el
   bloque *con* su agua).

## Riesgos

- **El agua que corre "dentro" de bloques.** `LiquidTile` es el código más
  delicado del juego original y lo más fácil de romper. Se toca lo mínimo: un
  "¿hay agua aquí?" que incluya el bit, en los pocos puntos donde decide.
- **Puertas y camas** tienen dos mitades que se ponen y se quitan juntas. Cada
  mitad lleva su propio bit, y al romper una puerta tienen que quedar las dos
  fuentes.
- **Rendimiento del dibujo:** cada bloque con agua dibuja dos cosas. En
  construcciones grandes bajo el agua se nota algo más al reconstruir chunks,
  igual que en Java.
