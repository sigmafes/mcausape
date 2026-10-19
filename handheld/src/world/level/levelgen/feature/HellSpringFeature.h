#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__HellSpringFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__HellSpringFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "Feature.h"
#include "../../Level.h"
#include "../../tile/Tile.h"
#include "../../material/Material.h"
#include "../../../../util/Random.h"

// Manantiales de lava del Nether: como SpringFeature pero rodeados de
// netherrack. Adaptado de MCPE 1.0.0 (HellSpringFeature).

class HellSpringFeature: public Feature
{
    int tile;
    bool insideRock;

public:
    HellSpringFeature(int tile, bool insideRock = false) {
        this->tile = tile;
        this->insideRock = insideRock;
    }

    bool place(Level* level, Random* random, int x, int y, int z) {
        if (level->getTile(x, y + 1, z) != Tile::netherrack->id) return false;

        if (level->getTile(x, y, z) != 0 && level->getTile(x, y, z) != Tile::netherrack->id) return false;

        int rockCount = 0;
        if (level->getTile(x - 1, y, z) == Tile::netherrack->id) rockCount++;
        if (level->getTile(x + 1, y, z) == Tile::netherrack->id) rockCount++;
        if (level->getTile(x, y, z - 1) == Tile::netherrack->id) rockCount++;
        if (level->getTile(x, y, z + 1) == Tile::netherrack->id) rockCount++;
        if (level->getTile(x, y - 1, z) == Tile::netherrack->id) rockCount++;

        int holeCount = 0;
        if (level->isEmptyTile(x - 1, y, z)) holeCount++;
        if (level->isEmptyTile(x + 1, y, z)) holeCount++;
        if (level->isEmptyTile(x, y, z - 1)) holeCount++;
        if (level->isEmptyTile(x, y, z + 1)) holeCount++;
        if (level->isEmptyTile(x, y - 1, z)) holeCount++;

        if ((!insideRock && rockCount == 4 && holeCount == 1) || rockCount == 5) {
            level->setTile(x, y, z, tile);
            level->instaTick = true;
            Tile::tiles[tile]->tick(level, x, y, z, random);
            level->instaTick = false;
        }

        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__HellSpringFeature_H__*/
