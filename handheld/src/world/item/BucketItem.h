#ifndef NET_MINECRAFT_WORLD_ITEM__BucketItem_H__
#define NET_MINECRAFT_WORLD_ITEM__BucketItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "../Facing.h"
#include "../level/Level.h"
#include "../level/material/Material.h"
#include "../level/tile/Tile.h"
#include "../entity/player/Player.h"
#include "../phys/HitResult.h"
#include "../phys/Vec3.h"

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

	/** The empty bucket scoops up the liquid it is aimed at.

	    This is use(), not useOn(): the ray the game aims blocks with skips
	    liquids, so useOn() is only ever called with the block *behind* the water
	    and an empty bucket could never take any. This casts its own ray that
	    stops at a liquid source (Level::clip with liquid set; flowing liquid has
	    no source to take). The direction is the player's aim, which a visitor's
	    use travels to the host with, so it works the same in a joined world. */
	/*@Override*/
	ItemInstance* use(ItemInstance* instance, Level* level, Player* player) {
		if (content != 0)
			return instance;

		Vec3 dir = player->aimDirection;
		if (dir.dot(dir) < 0.01f)
			dir = player->getViewVector(1);
		const Vec3 from(player->x, player->y, player->z);
		HitResult hit = level->clip(from, from + dir.normalized() * 5.0f, true);
		if (!hit.isHit() || hit.type != TILE)
			return instance;

		const int x = hit.x, y = hit.y, z = hit.z;
		// A stair, slab, fence... holding water gives it up and keeps standing.
		if (level->isWaterlogged(x, y, z)) {
			level->setWaterlogged(x, y, z, false);
			if (!player->abilities.instabuild)
				instance->id = Item::bucket_water->id;
			return instance;
		}
		const Material* m = level->getMaterial(x, y, z);
		if ((m != Material::water && m != Material::lava) || level->getData(x, y, z) != 0)
			return instance;

		const int tile = level->getTile(x, y, z);
		level->setTile(x, y, z, 0);
		// In creative the bucket is not used up either way, so it stays empty,
		// as in Minecraft: the liquid goes, the bucket does not fill.
		if (!player->abilities.instabuild)
			instance->id = (tile == Tile::lava->id)? Item::bucket_lava->id : Item::bucket_water->id;
		return instance;
	}

	/** A full bucket pours into the free block past the face that was clicked. */
	/*@Override*/
	bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
		if (content == 0)
			return false;

		// Water poured on a block that can hold it fills that block (Java's
		// waterlogging) instead of the space in front of it.
		if (content == Tile::water->id && Tile::canBeWaterlogged(level->getTile(x, y, z)) && !level->isWaterlogged(x, y, z)) {
			level->setWaterlogged(x, y, z, true);
			if (!player->abilities.instabuild)
				instance->id = Item::bucket_empty->id;
			return true;
		}

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
		// Creative has an endless supply: emptying it was what left both the
		// water and the lava bucket empty for good (until the world was
		// re-entered and the inventory refilled).
		if (!player->abilities.instabuild)
			instance->id = Item::bucket_empty->id;
		return true;
	}

private:
	int content;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__BucketItem_H__*/
