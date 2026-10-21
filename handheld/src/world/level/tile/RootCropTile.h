#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__RootCropTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__RootCropTile_H__

#include "CropTile.h"
#include "../../item/Item.h"
#include "../../entity/item/ItemEntity.h"
#include "../Level.h"

/** A carrot or potato plant. Grows like wheat (eight ages) but has four pictures:
    the first three (ages 0-1, 2-3, 4-6) are in a row from `tex`, and the ripe one
    is its own cell, `ripeTex` -- carrots put theirs next to the others, potatoes
    use a separate cell. What it drops is the item it was planted from: one, and
    up to three more once it has grown. */
class RootCropTile: public CropTile
{
	typedef CropTile super;
public:
	RootCropTile(int id, int tex, int ripeTex, Item** crop)
	:	super(id, tex),
		ripeTex(ripeTex),
		crop(crop)
	{}

	int getTexture(LevelSource* level, int x, int y, int z, int face) {
		return getTexture(face, level->getData(x, y, z));
	}

	int getTexture(int face, int data) {
		if (data < 0) data = 7;
		if (data == 7) return ripeTex;
		if (data == 6) data = 5;
		return tex + (data >> 1);
	}

	int getResource(int data, Random* random) {
		return (*crop)->id;
	}

	int getResourceCount(Random* random) {
		return 1;
	}

	void spawnResources(Level* level, int x, int y, int z, int data, float odds) {
		// Not CropTile's (it drops wheat seeds): Tile's, for the one item, then
		// the extras that depend on how far it grew.
		Tile::spawnResources(level, x, y, z, data, odds);
		if (level->isClientSide)
			return;
		for (int i = 0; i < 3; i++) {
			if (level->random.nextInt(5 * 3) > data) continue;
			ItemEntity* item = new ItemEntity(level, float(x) + 0.5f, float(y) + 0.5f, float(z) + 0.5f, ItemInstance(*crop));
			item->throwTime = 10;
			level->addEntity(item);
		}
	}

private:
	int ripeTex;
	Item** crop;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__RootCropTile_H__*/
