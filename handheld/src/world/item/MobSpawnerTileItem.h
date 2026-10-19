#ifndef NET_MINECRAFT_WORLD_ITEM__MobSpawnerTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__MobSpawnerTileItem_H__

#include "TileItem.h"
#include "../level/tile/MobSpawnerTile.h"
#include "../level/tile/Tile.h"

// The monster spawner as an item: the aux value is the kind of mob, so there is one
// stack per kind, each with its own name ("Zombie Spawner", ...) and picture.
class MobSpawnerTileItem: public TileItem
{
	typedef TileItem super;
public:
	MobSpawnerTileItem(int id)
	:	super(id)
	{
		setMaxDamage(0);
		setStackedByData(true);
	}

	/*@Override*/
	int getIcon(int itemAuxValue) {
		return Tile::mobSpawner->getTexture(2, itemAuxValue);
	}

	/*@Override*/
	int getLevelDataForAuxValue(int auxValue) {
		return auxValue;
	}

	/*@Override*/
	std::string getDescriptionId(const ItemInstance* instance) const {
		int aux = instance->getAuxValue();
		if (aux < 0 || aux >= MobSpawnerTile::KIND_COUNT)
			aux = 0;
		return super::getDescriptionId() + "." + MobSpawnerTile::KIND_NAMES[aux];
	}
};

#endif /*NET_MINECRAFT_WORLD_ITEM__MobSpawnerTileItem_H__*/
