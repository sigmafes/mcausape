#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__GlowStoneFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__GlowStoneFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "Feature.h"
#include "../../Level.h"
#include "../../tile/Tile.h"
#include "../../../../util/Random.h"

// Racimos de glowstone. Solo se coloca colgando del netherrack (techo),
// como en MCPE 1.0.0 (GlowStoneFeature).

class GlowStoneFeature: public Feature
{
public:
    bool place(Level* level, Random* random, int x, int y, int z) {
        if (!level->isEmptyTile(x, y, z)) return false;
        if (level->getTile(x, y + 1, z) != Tile::netherrack->id) return false;

        level->setTile(x, y, z, Tile::lightGem->id);

        for (int i = 0; i < 1500; i++) {
            int x2 = x + random->nextInt(8) - random->nextInt(8);
            int y2 = y - random->nextInt(12);
            int z2 = z + random->nextInt(8) - random->nextInt(8);

            if (!level->isEmptyTile(x2, y2, z2)) continue;

            int count = 0;
            if (level->getTile(x2 - 1, y2, z2) == Tile::lightGem->id) count++;
            if (level->getTile(x2 + 1, y2, z2) == Tile::lightGem->id) count++;
            if (level->getTile(x2, y2 - 1, z2) == Tile::lightGem->id) count++;
            if (level->getTile(x2, y2 + 1, z2) == Tile::lightGem->id) count++;
            if (level->getTile(x2, y2, z2 - 1) == Tile::lightGem->id) count++;
            if (level->getTile(x2, y2, z2 + 1) == Tile::lightGem->id) count++;

            if (count == 1) {
                level->setTile(x2, y2, z2, Tile::lightGem->id);
            }
        }

        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__GlowStoneFeature_H__*/
