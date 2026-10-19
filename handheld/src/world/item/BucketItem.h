#ifndef NET_MINECRAFT_WORLD_ITEM__BucketItem_H__
#define NET_MINECRAFT_WORLD_ITEM__BucketItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "../Facing.h"
#include "../level/Level.h"
#include "../level/material/Material.h"
#include "../level/tile/Tile.h"
#include "../entity/player/Player.h"

// A bucket: empty, or carrying one source block of water or lava. Ported from the
// Console Edition's BucketItem; this is the only way to carry a liquid, and
// obsidian (water poured on lava) and cobblestone (lava poured on water) are
// otherwise unreachable, since neither liquid tile can be picked up by hand.
class BucketItem: public Item
{
	typedef Item super;
public:
	// content is 0 for the empty bucket, or the tile id (Tile::water / Tile::lava)
	// of what a full one carries.
	BucketItem(int id, int content)
	:	super(id),
		content(content)
	{
		setMaxStackSize(1);
	}

	/*@Override*/
	bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
		if (content == 0) {
			// Empty: only a still source (data 0) of water or lava can be scooped up.
			const Material* m = level->getMaterial(x, y, z);
			if ((m == Material::water || m == Material::lava) && level->getData(x, y, z) == 0) {
				int tile = level->getTile(x, y, z);
				level->setTile(x, y, z, 0);
				instance->id = (tile == Tile::lava->id)? Item::bucket_lava->id : Item::bucket_water->id;
				return true;
			}
			return false;
		}

		// Full: poured into the free block past the face that was clicked.
		switch (face) {
			case Facing::DOWN : y--; break;
			case Facing::UP   : y++; break;
			case Facing::NORTH: z--; break;
			case Facing::SOUTH: z++; break;
			case Facing::WEST : x--; break;
			case Facing::EAST : x++; break;
		}

		if (!level->isEmptyTile(x, y, z) && level->getMaterial(x, y, z)->isSolid())
			return false;

		level->setTileAndData(x, y, z, content, 0);
		instance->id = Item::bucket_empty->id;
		return true;
	}

private:
	int content;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__BucketItem_H__*/
