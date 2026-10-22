# The Aquarium: niebla de océano cálido y portal sin plataforma

## Cómo es hoy

`GameRenderer` trata el agua igual en todas partes:

| Qué | Hoy | Dónde |
|---|---|---|
| Tipo de niebla | Exponencial (`GL_EXP`), densidad 0,1: a ~10 bloques ya casi no se ve nada, y a ~20 nada | `setupFog` |
| Color | Fijo, `(0,02; 0,02; 0,2)`: azul marino oscuro | `setupClearColor` |
| Brillo | El color se multiplica por la luz donde está la cámara, que en el Aquarium baja con la profundidad: la niebla se oscurece más aún | `setupClearColor` (`fogBr`) |
| Con el tiempo | No cambia: igual al entrar que tras un minuto | — |
| Partículas | Ninguna: `Level::animateTick` solo anima bloques (`t > 0`), y el agua del Aquarium es aire | `Level::animateTick` |

Por eso se ve corto y azul.

## Cómo lo hace Java Edition (1.13+)

Del código de Java (nombres de los mappings oficiales de Mojang):

**1. Visión bajo el agua progresiva** (`LocalPlayer.getWaterVision`). Cuenta
los ticks con los ojos bajo el agua (`waterVisionTime`):

```
t < 100:  vision = 0,6 · (t / 100)
t < 600:  vision = 0,6 + 0,4 · (t − 100) / 500
t ≥ 600:  vision = 1
```

Sube rápido los primeros 5 segundos y luego despacio hasta los 30 segundos.
Al sacar la cabeza del agua se reinicia.

**2. Distancia de la niebla** (`FogRenderer.setupFog`). Es lineal, no
exponencial:

```
inicio = −8
fin    = 96 · max(0,25; vision)
```

Al entrar el fin está a 24 bloques, a los 5 segundos a ~58 y a los 30
segundos a 96. Esa es la sensación de "al principio veo poco y poco a poco
veo lejos".

**3. Color** (`FogRenderer.setupColor`). Cada bioma tiene un `waterFogColor`.
El océano cálido usa `#041F33`, oscuro por sí solo. Java lo **aclara con la
visión**: multiplica el color hasta que su canal más fuerte llega a 1:

```
escala = min(1/r, 1/g, 1/b)
color  = color · (1 − vision) + color · escala · vision
```

Con visión completa, `#041F33` pasa a `(0,08; 0,61; 1,0)`: celeste. Además,
el color pasa de uno a otro suavemente al cambiar de bioma. Aquí no hay
biomas, así que no hace falta.

**4. Partículas** (`ParticleTypes.UNDERWATER`, `SuspendedParticle`). Son
puntitos de 1–2 píxeles que flotan casi quietos, sin gravedad, y duran 1–4
segundos. Java los crea al azar en los bloques de agua cercanos a la cámara
(`WaterFluid.animateTick`, 1 de cada 10 bloques animados). Son de color
claro, teñidos por la propia niebla.

## Plan

### 1. Portal fijo sin plataforma

- `AquariumLevelSource::buildHub`: se quitan los bloques de prismarina. Quedan
  el marco irrompible y los 4 bloques de portal, flotando en el agua a y=120.
- **Llegada y spawn**: sobre uno de los 4 bloques del portal, elegido al azar
  (`findArrival` y `Level::validateSpawn`). Al llegar dentro del portal, el
  enfriamiento ya existente (se mantiene lleno mientras se está dentro) evita
  que te devuelva al instante, como pasa con el del Nether.
- **Mundos ya creados**: la plataforma ya está guardada en el archivo del
  Aquarium. `buildHub` comprueba si existe y quita los 100 bloques de
  prismarina una vez.

### 2. Visión progresiva

- `LocalPlayer`: contador `waterVisionTime` (sube con los ojos bajo el agua;
  vuelve a 0 al salir) y `getWaterVision()` con la fórmula de Java.
- Es solo del cliente: cada uno lo calcula para sí, sin red.

### 3. Niebla lineal larga en el Aquarium

- `setupFog`: si la dimensión es el Aquarium, niebla lineal con inicio en −8 y
  fin en `96 · max(0,25; vision)`. Nunca pasa de la distancia de dibujado del
  jugador, porque más allá no hay chunks dibujados.
- `setupClearColor`: el color base es **celeste verdoso** y se aclara con la
  visión como en Java. Propongo partir de un tono entre el océano cálido de
  Java y verde agua: `(0,10; 0,72; 0,80)` con visión completa. Se ajusta tras
  probarlo.
- Profundidad: hoy el brillo de la niebla cae con la luz del lugar, y al fondo
  quedaría casi negra. Propongo que la profundidad la atenúe **como mucho a la
  mitad**: se nota que se baja, pero se sigue viendo.

### 4. Partículas flotantes

- Nueva `SuspendedParticle` (`client/particle`): un punto de 1–2 píxeles, sin
  gravedad, con una deriva muy lenta. Vive 20–80 ticks y se desvanece al
  final.
- Aparición: en el Aquarium, cada tick, unas 8 en posiciones al azar en un
  radio de 16 bloques de la cámara, solo donde no haya bloque. Va junto a
  `Level::animateTick`, en `Minecraft::tick`.
- Usa la misma textura de partículas (`particles.png`); el color lo da la
  partícula, sin imagen nueva.
- Coste: unas ~250 partículas vivas a la vez, del orden del humo o del portal
  del Nether. Si se nota en móvil, se reduce el número en pantalla táctil.

### Qué no cambia

El agua del Overworld (lagos y océanos) sigue como está, salvo que digas lo
contrario (ver preguntas).

## Archivos

| Archivo | Cambio |
|---|---|
| `world/level/levelgen/AquariumLevelSource.cpp` | `buildHub` sin plataforma, y la limpieza en mundos viejos |
| `client/Minecraft.cpp`, `world/level/Level.cpp` | Llegada y spawn en un bloque del portal |
| `client/player/LocalPlayer.h/.cpp` | `waterVisionTime`, `getWaterVision()` |
| `client/renderer/GameRenderer.cpp` | `setupFog` y `setupClearColor` para el Aquarium |
| `world/level/dimension/AquariumDimension.cpp` | Color base de la niebla |
| `client/particle/SuspendedParticle.h`, `LevelRenderer.cpp` | La partícula y su nombre (`"suspended"`) |

## Preguntas

1. Los "8 bloques" del portal: el portal tiene **4** bloques (2×2) y el marco
   tiene 8. ¿El spawn va en uno de los 4 de portal (dentro de él) o encima de
   uno de los 8 del marco?
2. ¿Aplico también la niebla progresiva de Java al agua del Overworld (lagos
   y océanos, con su azul de siempre) o solo al Aquarium?
3. ¿Te sirve el color propuesto `(0,10; 0,72; 0,80)` como punto de partida?
