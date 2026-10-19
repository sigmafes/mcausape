//package net.minecraft.world.entity;

#include "../../platform/log.h"

#include "../level/LevelConstants.h"

class Material;

class MobCategory
{
	// How many 256x256 worlds fit in this one.
	static const int WORLD_AREA_FACTOR = (CHUNK_CACHE_WIDTH / 16) * (CHUNK_CACHE_WIDTH / 16);
public:
	static void initMobCategories();

    //
    static const MobCategory monster;
    static const MobCategory creature;
    static const MobCategory waterCreature;
	//@todo: rewrite to std::vector with [] init
	static const MobCategory* const values[];
	static const int numValues;

    int getBaseClassId() const {
        return _baseClassId;
    }

    int getMaxInstancesPerChunk() const {
        return _max;
    }

	// Animals are spawned across the whole map, so their cap grows with its area
	// (a 256x256 world allowed 15; 512x512 allows 60, the same density). Hostile
	// mobs are only spawned within 8 chunks of a player, an area that does not
	// grow with the world, so their cap stays as it was.
	int getMaxInstancesPerLevel() const {
		return _isFriendly ? _maxPerLevel * WORLD_AREA_FACTOR : _maxPerLevel;
	}

    const Material* getSpawnPositionMaterial() const {
        return _spawnPositionMaterial;
    }

    bool isFriendly() const {
        return _isFriendly;
    }

private:
    const int _baseClassId;
    const int _max;
	const int _maxPerLevel;
    mutable const Material* _spawnPositionMaterial;
    const bool _isFriendly;

    MobCategory(int baseClassId, int max, int maxPerLevel, bool isFriendly)
    :   _baseClassId(baseClassId),
        _max(max),
		_maxPerLevel(maxPerLevel),
		_spawnPositionMaterial(NULL),
        _isFriendly(isFriendly)
    {
		//LOGI("Creating a Mobcategory: %d, %d, %p, %d    ", _baseClassId, _max, _spawnPositionMaterial, _isFriendly);
	}

	void setMaterial(const Material* material) const {
		_spawnPositionMaterial = material;
	}
};
