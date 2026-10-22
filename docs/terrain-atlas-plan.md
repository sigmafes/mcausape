# Ampliar el atlas de bloques (terrain.png): análisis

`terrain.png` mide 256×256: 16×16 casillas de 16 px, 256 texturas. Está lleno.
Este documento recoge **todo** lo que en el motor asume ese tamaño, por qué
ampliarlo "a lo bruto" rompe el juego entero, y cómo hacerlo sin romper nada.

## Estado

Hecho. `client/renderer/TerrainAtlas.h` (px/py/SizeF) en `TileRenderer`
(144 usos), `Textures::tick` (animaciones), `TerrainParticle`, `ItemRenderer`
e `ItemInHandRenderer` (rama bloque/ítem), y `terrain.png` es 512×512 con el
atlas viejo intacto arriba a la izquierda. Casillas nuevas: 256–1023. Los
ítems, partículas y la GUI siguen con sus atlas de 256. Sin prueba de
píxeles todavía.

De paso: la superposición de "cabeza dentro de un bloque" leía la casilla
mal (usaba `tex % 16` como píxeles) y mostraba una esquina del atlas.

## Cómo encuentra el juego la textura de un bloque

Cada bloque devuelve un **número de textura** (`Tile::getTexture`, un `int`):
`fila * 16 + columna`. Ese número se convierte en coordenadas UV en muchos
sitios distintos, siempre con la misma cuenta copiada a mano:

```cpp
int xt = (tex & 0xf) << 4;      // columna * 16 px
int yt = tex & 0xf0;            // fila * 16 px
float u0 = xt / 256.0f;         // 256 = ancho del atlas
float v0 = yt / 256.0f;
```

No hay una función central: la cuenta está repetida en cada renderizador.

## Por qué ampliarlo rompe todo

Si se cambia el PNG a 512×512 sin tocar el código, **todo** se ve mal a la
vez, y por varias razones distintas que se suman:

1. **Las UV se dividen por 256.** Con una imagen de 512, `xt / 256` apunta al
   doble de lejos: cada cara muestra un trozo de 2×2 casillas que no es el
   suyo. Todos los bloques del mundo salen mezclados. Es lo primero que se
   ve y parece que "se rompió todo".
2. **Las animaciones escriben en la casilla equivocada.** El agua, la lava,
   el fuego y el portal se redibujan cada tick con `glTexSubImage2D` en
   `tex % 16 * 16, tex / 16 * 16` (`Textures::tick`). Si el atlas cambia de
   distribución, pintan encima de otros bloques.
3. **La interfaz usa el mismo código para dos atlas.** `ItemRenderer`,
   `ItemSpriteRenderer`, `ItemInHandRenderer` y `GuiComponent::blit` dibujan
   tanto bloques (terrain.png) como ítems (gui/items.png) con la misma cuenta
   de "/256". Si se cambia a /512 para arreglar los bloques, se rompen todos
   los ítems, el inventario y los iconos del HUD (que también usan `blit`
   con imágenes de 256).
4. **Los números de textura están escritos a mano por todas partes.**
   Cientos de constantes tipo `8 + 5 * 16` en los bloques, la grieta al
   romper (`15 * 16 + progreso`), el agua que corre (2×2 casillas:
   `tex`, `tex + 1`, `tex + 16`, `tex + 17`)… Si el atlas pasa a tener 32
   columnas, `fila * 16 + columna` ya no es la casilla correcta y **todas**
   esas constantes quedan mal.

Cualquiera de las cuatro basta para que el juego parezca destrozado. Arreglar
una deja las otras, y por eso un intento parcial "rompe todo".

## Inventario de lo que hay que tocar

| Sitio | Qué hace | Cambios |
|---|---|---|
| `client/renderer/TileRenderer.cpp` | Todas las formas de bloque (cubo, cruz, antorcha, escaleras, puertas, líquidos, tallos, vallas…) | ~19 cálculos de casilla y ~107 divisiones por 256 |
| `client/renderer/Textures.cpp` (`tick`) | Sube las animaciones (agua, lava, fuego, portal) a su casilla | 1 |
| `client/particle/TerrainParticle.h` | Partículas al romper un bloque | 1 cálculo (4 líneas) |
| `client/renderer/entity/ItemRenderer.cpp` | Bloques planos en el suelo y en el inventario | 2 sitios; separar bloque e ítem |
| `client/renderer/entity/ItemSpriteRenderer.cpp` | Sprites de ítems/bloques lanzados | 1; separar |
| `client/renderer/ItemInHandRenderer.cpp` | Bloque o ítem en la mano | ~17 líneas; separar |
| `client/gui/GuiComponent.cpp` (`blit`) | Toda la interfaz, con imágenes de 256 | Añadir una variante con tamaño de atlas; **no** cambiar la actual |
| `client/renderer/LevelRenderer.cpp` | Grieta al romper (`15 * 16 + progreso`) | Ninguno (ver el diseño) |
| Constantes de textura en cada `Tile` | Números de casilla escritos a mano | Ninguno (ver el diseño) |

No influyen: la red y el guardado (no guardan texturas), el formato de los
vértices (UV en `float`, cualquier tamaño vale) y WebGL (512×512 es pequeño
y potencia de dos, así que el mipmapping sigue funcionando).

## Diseño que no rompe nada: cuatro cuadrantes

La clave es **no cambiar lo que significa un número de textura que ya
existe**. El atlas pasa a 512×512 dividido en cuatro cuadrantes de 256×256,
y cada cuadrante funciona exactamente como el atlas de hoy:

| Números | Cuadrante | Contenido |
|---|---|---|
| 0–255 | arriba a la izquierda | **el terrain.png actual, sin mover un píxel** |
| 256–511 | arriba a la derecha | nuevos |
| 512–767 | abajo a la izquierda | nuevos |
| 768–1023 | abajo a la derecha | nuevos |

Una única función hace la conversión:

```cpp
// cuadrante = tex >> 8, casilla dentro = tex & 255 (16 columnas, como hoy)
px = ((tex >> 8) & 1) * 256 + (tex & 15) * 16;
py = ((tex >> 9) & 1) * 256 + ((tex >> 4) & 15) * 16;
u  = px / 512.0f;   v = py / 512.0f;
```

Consecuencias:

- **Todas las constantes actuales siguen valiendo**: el número 88 sigue
  siendo la casilla de siempre (columna 8, fila 5, del primer cuadrante).
- **Las cuentas `tex + 1` y `tex + 16` siguen funcionando** dentro de cada
  cuadrante (el agua de 2×2, las texturas por fases, las puertas). Lo único
  que no hay que hacer es poner un grupo de casillas contiguas cruzando el
  borde de un cuadrante.
- La grieta (240–249) y las animaciones están en el primer cuadrante y no
  cambian de número.
- **768 casillas nuevas.** Si algún día no alcanzara, el mismo esquema
  crece a 1024×1024 con 16 cuadrantes.

## Pasos

1. **Función central** `TerrainAtlas` (cálculo de px/py y de UV, tamaño del
   atlas) y sustituir con ella cada cuenta de la tabla de arriba. Solo los
   sitios que dibujan bloques: los ítems siguen con su atlas de 256.
2. **Separar bloque e ítem** en `ItemRenderer`, `ItemSpriteRenderer` e
   `ItemInHandRenderer` (ya distinguen con `item->id < 256`; solo hay que
   usar la cuenta correcta en cada rama), y una variante de `blit` para el
   atlas de bloques.
3. **Animaciones** (`Textures::tick`) con la misma función.
4. **Imagen nueva**: 512×512 con el terrain.png actual copiado tal cual
   arriba a la izquierda y el resto transparente.
5. **Comprobar que no cambió nada**: con el mismo mundo y la misma posición,
   capturas antes y después deben ser **idénticas píxel a píxel** (bloques,
   agua y lava animadas, grietas, partículas, inventario, ítem en la mano,
   ítems en el suelo). Con el diseño de cuadrantes, cualquier diferencia es
   un error, así que la prueba es objetiva. Se puede automatizar con el
   mismo Playwright de `tools/mptest`.
6. Solo después de eso, el primer bloque nuevo con número ≥ 256.

## Riesgos que quedan

- **Olvidar un sitio.** Es el riesgo real: una forma de bloque poco usada
  (cama, tallo, soporte de pociones…) con la cuenta vieja. Con el diseño de
  cuadrantes, un olvido **no rompe** nada de lo existente: solo dibujaría
  mal los bloques nuevos (≥ 256) en esa forma, y se ve enseguida al
  probarlos. Antes de dar el paso 1 por terminado, una búsqueda de
  `0xf0`, `& 0xf`, `% 16` y `/ 256` en los renderizadores debe dar solo
  cosas revisadas.
- **Sangrado entre casillas** al alejarse (mipmapping). Pasa hoy igual; el
  borde entre cuadrantes no cambia nada.
- **Paquetes de texturas** de terceros con un terrain.png de 256 dejarían de
  encajar. Este juego no los usa.

## Recomendación

Hacerlo, con el diseño de cuadrantes, como un paso propio antes de añadir
bloques: es un cambio mecánico en unos 8 archivos del motor de dibujo, no
toca la lógica de ningún bloque ni el guardado, y la prueba de píxeles
idénticos dice sin ambigüedad si quedó bien.
