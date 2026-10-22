# Sistema de pociones: soporte de pociones (brewing stand)

## Lo que ya existe

| Pieza | Estado |
|---|---|
| Bloque soporte de pociones (`BrewingStandTile`, id 117) | Se coloca y se dibuja. Al usarlo no hace nada: no tiene pantalla ni datos. |
| Ítem soporte de pociones | Existe y tiene receta (vara de blaze + piedra). |
| Botella vacía / botella de agua (`BottleItem`) | Funciona: clic derecho mirando al agua llena una botella. |
| Polvo de blaze, verruga del Nether (`netherStalkSeeds`) | Existen. |
| Pantalla del horno (`FurnaceScreen`) | Inventario filtrado a la izquierda y casillas a la derecha. El fuego (`getLitProgress`) y la flecha (`getBurnProgress`) se dibujan desde `gui/spritesheet.png`. |
| Horno: datos y red | `FurnaceTileEntity` (casillas, tiempo, guardado), `FurnaceMenu` (sincroniza el progreso con `ContainerSetData`), `ContainerOpenPacket` con el tipo `FURNACE`. `ContainerType::BREWING_STAND = 5` ya está definido pero no se usa. |

## Cómo es en Minecraft (Java 1.9+)

- **Casillas:** 1 de ingrediente arriba, 3 de botellas abajo y 1 de
  combustible a la izquierda (polvo de blaze).
- **Funcionamiento:** las botellas se transforman **en su sitio**; no hay
  casilla de resultado. Un ingrediente transforma las 3 botellas a la vez.
- **Tiempos:** cada preparación dura 400 ticks (20 s). Un polvo de blaze da
  20 preparaciones.
- **Animación:** una flecha que baja (progreso) y burbujas que suben.
- **Recetas:**
  - botella de agua + verruga = poción rara (*awkward*)
  - poción rara + ingrediente = poción con efecto
- Las pociones no se apilan.
- **Icono:** botella con el líquido teñido del color del efecto. Respiración
  acuática es `#2E5299`, azul.

## Diseño para mcausape (reciclando el horno)

Lo que pediste, en el formato del horno: a la izquierda el inventario, y a
la derecha 4 casillas, el fuego y la flecha.

```
 [Ingrediente]
                  ──flecha──▶  [Resultado]
 [Botellas]
   (fuego)
 [Combustible]
```

- **Combustible:** solo polvo de blaze. Dura lo mismo que el carbón en el
  horno.
- **Ingrediente:** verruga del Nether o cristal oceánico (por ahora).
- **Botellas:** botella de agua o poción sin efecto.
- **Resultado:** la poción hecha. Se recoge tocándola, como en el horno.
- El fuego y la flecha son los mismos dibujos del horno.

### Recetas

| Botella | Ingrediente | Resultado |
|---|---|---|
| Botella de agua | Verruga del Nether | Poción sin efecto |
| Poción sin efecto | Cristal oceánico | Poción de respiración acuática |

Ninguna se puede beber todavía. Todas guardan una duración de **5 minutos**
para cuando lleguen los efectos.

## Piezas a programar

1. **Ítem `PotionItem`** (un único ítem, el tipo va en su dato, como en Java):
   - 0 = sin efecto, 1 = respiración acuática.
   - No se usa al hacer clic (no se bebe).
   - Nombre según el tipo, y la descripción dice "5:00".
   - Lleva una tabla `PotionType` (nombre, color, duración en ticks =
     6000) para cuando se añadan los efectos.
2. **`BrewingStandTileEntity`** (copia del horno):
   - 4 casillas.
   - Combustible: duración de cada polvo, progreso de la preparación.
   - Guardado en el mundo (NBT).
   - Receta: botella + ingrediente → resultado, si cabe.
3. **`BrewingStandMenu`**: como `FurnaceMenu`, envía el progreso y las casillas
   a los visitantes.
4. **`BrewingStandScreen`**:
   - Copia de `FurnaceScreen` con 4 casillas.
   - El inventario de la izquierda muestra solo lo que cabe en la casilla
     elegida.
   - Fuego (combustible restante) y flecha (progreso).
5. **Bloque:** al usarlo abre la pantalla. Es contenedor: al romperlo suelta lo
   que tiene dentro.
6. **Red:** `ServerPlayer::openBrewingStand` y la rama del tipo
   `BREWING_STAND` en el cliente, igual que el horno.
7. **Texturas:** iconos de las dos pociones en casillas libres de
   `gui/items.png`, hechos a partir de la botella de agua con el líquido
   recoloreado.
8. **Textos** en `en_US.lang`.

## Preguntas

1. **Botellas por preparación:** ¿la casilla de botellas admite varias y se
   preparan **de una en una** (cada una con su ingrediente), o como en Java
   **hasta 3 a la vez** con un solo ingrediente?
2. **Tiempo de preparación:** ¿20 s como en Java, o 10 s como cocinar en el
   horno?
3. **Combustible:** "mismo tiempo del carbón" = un polvo de blaze dura 80 s
   encendido, lo que el carbón en el horno. ¿Así? (Con 20 s por poción salen 4
   pociones por polvo; con 10 s, 8.)
4. **¿Se apilan las pociones?** En Java no (1 por casilla). Si no se apilan,
   la casilla de resultado se llena con una y hay que recogerla para seguir.
   ¿O dejo que se apilen hasta 3 o 16 en la casilla de resultado?
5. **Color del icono:** ¿sin efecto como la botella de agua pero algo más
   oscura, y respiración acuática azul como Java (`#2E5299`) o celeste verdoso
   como el Aquarium?
6. **Creativo:** ¿añado las dos pociones al inventario creativo?
7. **Bloque:** ¿quieres que el soporte muestre botellas en sus brazos cuando
   tiene algo dentro (como en Java), o lo dejamos para después?
