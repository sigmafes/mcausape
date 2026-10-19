#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__MobSpawnerTileEntity_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__MobSpawnerTileEntity_H__

#include "TileEntity.h"

// What makes a monster spawner work. Unlike a random tile tick, which reaches any one
// block very rarely, a tile entity ticks every game tick, as the Console Edition's
// MobSpawnerTileEntity does. The kind of mob is the block's data value.
class MobSpawnerTileEntity: public TileEntity
{
	typedef TileEntity super;
public:
	MobSpawnerTileEntity();

	bool shouldSave();
	void tick();

private:
	bool trySpawn(int mobType);

	int spawnDelay;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__MobSpawnerTileEntity_H__*/
