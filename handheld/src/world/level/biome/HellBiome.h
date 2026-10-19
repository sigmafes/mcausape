#ifndef NET_MINECRAFT_WORLD_LEVEL_BIOME__HellBiome_H__
#define NET_MINECRAFT_WORLD_LEVEL_BIOME__HellBiome_H__

//package net.minecraft.world.level.biome;

#include "Biome.h"
#include "../../entity/EntityTypes.h"

class HellBiome: public Biome
{
public:
    HellBiome() {
        setName("Hell");
        clearMobs();

        // Zombie pigmen, as in LCE's HellBiome: weight 100, in groups of 4.
        // PigZombie::canSpawn does not look at light, which is what lets them
        // appear in a dimension that has none.
        _enemies.push_back(MobSpawnerData(MobTypes::PigZombie, 100, 4, 4));
        // Ghasts and magma cubes, with the Java Edition's weights. Ghast::canSpawn
        // lets only one try in twenty through, which keeps them rare.
        _enemies.push_back(MobSpawnerData(MobTypes::Ghast, 50, 4, 4));
        _enemies.push_back(MobSpawnerData(MobTypes::MagmaCube, 2, 4, 4));
    }

    // El Nether no tiene vegetacion
    Feature* getTreeFeature(Random* random) {
        return NULL;
    }
    Feature* getGrassFeature(Random* random) {
        return NULL;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_BIOME__HellBiome_H__*/
