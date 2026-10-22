# Nado de Java Edition (1.13+): plan de implementación

## Cómo lo hace Java (referencia)

Resumen del código de Java 1.13–1.20 (nombres de los mappings oficiales de
Mojang). Es lo que se copia, adaptado a 0.6.1.

| Pieza | Java | Valores |
|---|---|---|
| Empezar / dejar de nadar | `Entity.updateSwimming()` cada tick | Empieza si corre **y** tiene los ojos bajo el agua (`isUnderWater`) y no va montado. Sigue mientras corra y esté en agua (`isInWater`); para si deja de correr o sale. |
| Pose y hitbox | `Pose.SWIMMING`, `Player.updatePlayerPose()` | 0,6 × 0,6 (como al planear con élitros). Ojos a 0,4. Al dejar de nadar solo se pone de pie si **cabe** (`canEnterPose(STANDING)`); si no, se agacha o sigue tumbado (así se pasa por huecos de 1 bloque). |
| Transición suave | `LivingEntity.swimAmount` / `getSwimAmount(partial)` | Sube 0,09 por tick hasta 1 al nadar y baja 0,09 hasta 0 al parar (~11 ticks, ½ s). Todo lo visual se mezcla con este valor. |
| Cuerpo tumbado | `PlayerRenderer.setupRotations()` | Gira el cuerpo en X: `lerp(swimAmount, 0, -90 - xRot)` en el agua (sigue la mirada: nadar hacia abajo lo inclina de cabeza) y `-90` fuera. Si nada de verdad, además traslada `(0, -1, 0.3)` para que el giro sea sobre el pecho. |
| Brazos | `HumanoidModel.setupAnim()` | Ciclo de crol: `t = limbSwing % 26`, en tres tramos (0–14 brazos hacia delante por arriba, 14–22 bajan a los lados, 22–26 vuelven), con `rotlerpRad` y `quadraticArmUpdate`, todo mezclado por `swimAmount`. Con el jugador quieto en el agua los brazos quedan en reposo (el ciclo avanza con `limbSwing`, que solo crece al moverse). |
| Piernas | `HumanoidModel.setupAnim()` | `xRot = lerp(swimAmount, xRot, 0.3 * cos(limbSwing * 0.33333334 + π))`, opuestas entre sí: pataleo corto y suave. |
| Cabeza | `HumanoidModel.setupAnim()` | Al nadar la cabeza mira hacia delante (`xRot` de la cabeza = `-45°` mezclado), no hacia el suelo. |
| Velocidad | `LivingEntity.travel()` | En agua: fricción `0,8`, o `0,9` si corre (nadando siempre corre), es decir ~2,25× más rápido que al bracear normal. La mirada decide la dirección vertical: mirando hacia abajo baja; hacia arriba, sube un poco. |

**Bedrock** hace lo mismo con distinto sistema de animación (archivos
`animations/player.animation.json`: `animation.player.swim` y
`animation.player.move.legs.swim`). Las curvas sirven de referencia visual
pero no se pueden copiar tal cual: Bedrock usa Molang, no código.

**Mods**: los ports a versiones viejas (p. ej. *Swimming+*, el "1.13 swimming
backport" para 1.12) copian estas mismas funciones de Java. No aportan nada
nuevo; el punto de partida es el código de Java de arriba.

## Estado

Fases 1 a 4 implementadas. Diferencia con el plan: `heightOffset` no cambia
al nadar (la `y` del jugador sigue a 1,62 de los pies, en red y en todo el
juego); solo se encoge la caja desde los pies y la cámara baja 1,22 mezclado
con `swimAmount`. Así no hay saltos de posición al empezar o terminar.

## Dónde va en este juego

| Qué | Archivo | Nota |
|---|---|---|
| Bandera de nado | `world/entity/SharedFlags.h` | `FLAG_SWIMMING = 4`, como en Java. Las banderas compartidas ya viajan por red (así se ve agacharse en otros jugadores), así que los visitantes se ven nadar sin paquetes nuevos. |
| Empezar / parar | `Entity` o `Player::tick` | `updateSwimming()` igual que Java, usando el sprint que ya existe (`FLAG_SPRINTING`, doble toque de W o Ctrl). Solo lo decide el jugador local (o el host para sí mismo); los demás lo leen de la bandera. |
| Hitbox | `Player.cpp` | `setSize(0.6, 0.6)` y `heightOffset` (ojos) a 0,4 al nadar; `setSize(0.6, 1.8)` / `1.62` al terminar, solo si cabe de pie (comprobar con `level->getCubes`, como ya hace `resetPos`). |
| Velocidad | `Mob::travel` (rama `isInWater`) | Cambiar `0.80f` por `0.90f` si nada, y el empuje vertical según la mirada. Es código de simulación original: el cambio queda limitado a ese `if`. |
| Valor de transición | `Mob` | `swimAmount` / `swimAmountO` (anterior, para interpolar entre ticks), actualizado en `tick()`. |
| Giro del cuerpo | `PlayerRenderer.cpp` / `MobRenderer::setupRotations` | Rotación en X y traslación mezcladas con `swimAmount`. Revisar también la vista en tercera persona y la cámara frontal. |
| Brazos, piernas y cabeza | `HumanoidModel.cpp` (`setupAnim`) | El ciclo de 26 de Java, mezclado con `swimAmount`. Hay que pasarle al modelo el `swimAmount` del jugador que se dibuja (hoy recibe banderas como `sneaking`). |
| Cámara | `GameRenderer` | La cámara baja a la altura de los ojos nueva (0,4) de forma suave, no de golpe. |

## Fases

1. **Lógica y velocidad.** Bandera, `updateSwimming`, hitbox de 0,6 con la
   regla de "ponerse de pie solo si cabe", velocidad 0,9 y dirección con la
   mirada. Se puede probar sin animación: pasar nadando por un túnel de agua
   de 1 bloque de alto.
2. **Cuerpo tumbado.** `swimAmount` y el giro de 90° suave en
   `PlayerRenderer`, en primera y tercera persona.
3. **Brazos, piernas y cabeza.** El ciclo de crol y el pataleo en
   `HumanoidModel`.
4. **Multijugador.** Comprobar que un visitante ve nadar al host y al revés
   (la bandera ya viaja), y que el host acepta la hitbox baja del visitante
   (no lo empuja hacia arriba ni lo da por atascado).
5. **Detalles.** Cámara suave al tumbarse; nadar no gasta comida (aquí no hay
   hambre); burbujas y sonido de chapoteo opcionales.

## Riesgos

- **La hitbox decide el tamaño en la red.** El host ve a cada visitante con
  la hitbox del visitante; si no la conoce, puede empujarlo fuera de un
  túnel de 1 bloque. Por eso el host también debe aplicar el tamaño de nado
  cuando ve la bandera del visitante.
- **`heightOffset` está por todas partes en 0.6.1** (posiciones de red,
  `moveTo`, `resetPos`). Cambiarlo al nadar hay que hacerlo con cuidado para
  no desplazar al jugador 1,2 bloques cada vez que empieza o termina de nadar.
