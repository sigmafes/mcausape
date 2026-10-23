#ifndef NET_MINECRAFT_WORLD_ITEM__AxolotlBucketItem_H__
#define NET_MINECRAFT_WORLD_ITEM__AxolotlBucketItem_H__

#include "Item.h"
#include "ItemInstance.h"
#include "../entity/animal/Axolotl.h"
#include "../entity/player/Player.h"
#include "../level/Level.h"
#include "../level/dimension/Dimension.h"
#include "../level/tile/Tile.h"

/// A bucket with an axolotl in it (aux 1: a tamed one, which takes whoever
/// lets it out as owner). Emptied on a block face, out comes the axolotl --
/// with its water, outside the Aquarium -- and the bucket is empty again.
class AxolotlBucketItem: public Item
{
	typedef Item super;
public:
	AxolotlBucketItem(int id) : super(id) { setMaxStackSize(1); }

	bool useOn(ItemInstance* item, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
		static const int off[6][3] = { {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0} };
		if (face < 0 || face > 5) return false;
		x += off[face][0]; y += off[face][1]; z += off[face][2];
		const int there = level->getTile(x, y, z);
		if (there != 0 && Tile::solid[there]) return false;
		if (!level->isClientSide) {
			if (there == 0 && !level->dimension->isAllWater())
				level->setTile(x, y, z, Tile::water->id);
			Axolotl* a = new Axolotl(level);
			a->moveTo(x + 0.5f, (float) y, z + 0.5f, player->yRot + 180, 0);
			if (item->getAuxValue() == 1) a->owner = player->name;
			level->addEntity(a);
		}
		if (!player->abilities.instabuild) *item = ItemInstance(Item::bucket_empty);
		return true;
	}
};

#endif
