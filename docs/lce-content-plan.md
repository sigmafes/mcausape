# Content from the Console Edition: plan

What was asked for, what is already here, and the order it is being done in.
Reference: `chrisGrando/MinecraftLCE-7thGen`. Item ids below are the game's
own (`new Item(n)` is vanilla id `256 + n`); tile ids are raw.

## Already in the game

| Asked for | State |
|---|---|
| Paper, leather, book | items exist (ids 83, 78, 84) and have recipes; were missing from the creative list |
| Blaze rod, blaze powder, magma cream, gold nugget, glass bottle | **icons** are in `items.png` ((12,6), (13,9), (13,10), (12,7), (12,8)); no items yet |
| Pumpkin, jack o' lantern, pumpkin seeds | textures are in `terrain.png` / `items.png`; the tiles are declared in `Tile.h` and never created |
| Enchanting table | top texture (6,10) is in `terrain.png`; no tile |
| Brewing stand | texture (13,9) in `terrain.png`, icon in `items.png`; no tile |
| Fence, fence gate, door, trapdoor | oak only |
| Cobblestone wall, carrot, potato | nothing (no icons, no textures) |

## Order

Each phase compiles, is committed and deployed on its own.

1. **Items.** Gold nugget, blaze rod, blaze powder, magma cream, carrot, potato,
   baked potato, golden carrot, glass bottle, water bottle. Their recipes and
   furnace results, the drops of pigmen / blazes / magma cubes, food values, the
   missing icons (carrot, potato, baked potato, golden carrot copied from LCE's
   `items.png`, water bottle drawn from the glass bottle), names, creative list.
2. **Pumpkins.** `PumpkinTile` (plain and lit, facing the player), pumpkin stem
   (reusing `StemTile` the way the melon does), pumpkin seeds, recipes (seeds,
   jack o' lantern = pumpkin + torch).
3. **Crops.** Carrots and potatoes as `CropTile`s with LCE's four growth
   pictures, planted by the items themselves, drops 1-4.
4. **Spruce and birch wood.** Fence, fence gate, trapdoor, door for each
   (tiles 160-167, door items), textures recoloured from the oak ones, recipes
   that need the matching planks.
5. **Cobblestone wall** (plain and mossy): new render shape, collision 1.5 high,
   connects to walls, fences' neighbours and solid blocks.
6. **Enchanting table and brewing stand.** Blocks only, no screen: they can be
   placed, broken and drawn, and do nothing when used.
7. **Glass bottle on water** fills it (the bucket's liquid ray), nothing else
   for potions yet.

## Not doing (yet)

Potions, enchanting, brewing screens, rotten flesh, the poisonous potato, pumpkin
pie, carrot on a stick, naturally generated pumpkins.

## State

Phases 1-6 are in. Spare cells used in `terrain.png`: (13,10)/(13,11) spruce door
(upper/lower), (7,13)/(7,14) birch door, (11,10) spruce and (12,10) birch trapdoor,
(14,11) enchanting table side, (9,13) its underside. In `items.png`: (0,9) spruce
door, (1,9) birch door, (2,9) brewing stand. Tile ids: 139 cobblestone wall, 143
mossy wall, 141/142 carrots/potatoes, 160-167 spruce/birch fence, gate, trapdoor,
door; 116 enchanting table, 117 brewing stand. The enchanting table's diamonds are
`Item::emerald` (the 0.6.1 name).
