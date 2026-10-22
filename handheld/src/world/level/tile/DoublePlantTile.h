#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__DoublePlantTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__DoublePlantTile_H__

#include "Bush.h"
#include "../../item/Item.h"

/// Java's tall grass: two blocks of crossed pictures, the top marked by data
/// bit 8. Placing the bottom puts the top over it; either half going takes
/// the other with it. Tinted like the short grass.
class DoublePlantTile: public Bush
{
public:
	static const int TopBit = 8;

	DoublePlantTile(int id, int tex) : Bush(id, tex, Material::replaceable_plant) {
		setTicking(false);
		setShape(0.1f, 0, 0.1f, 0.9f, 1, 0.9f);
	}

	int getTexture(int face, int data) { return (data & TopBit) ? tex + 1 : tex; }
	int getColor(LevelSource* level, int x, int y, int z) { return 0x339933; }
	int getColor(int data) { return 0x339933; }

	bool mayPlace(Level* level, int x, int y, int z, unsigned char face) {
		return Bush::mayPlace(level, x, y, z, face) && level->isEmptyTile(x, y + 1, z);
	}

	void onPlace(Level* level, int x, int y, int z) {
		if (!(level->getData(x, y, z) & TopBit) && level->isEmptyTile(x, y + 1, z))
			level->setTileAndData(x, y + 1, z, id, TopBit);
	}

	bool canSurvive(Level* level, int x, int y, int z) {
		if (level->getData(x, y, z) & TopBit) return level->getTile(x, y - 1, z) == id;
		return mayPlaceOn(level->getTile(x, y - 1, z));
	}

	void neighborChanged(Level* level, int x, int y, int z, int type) {
		const bool top = (level->getData(x, y, z) & TopBit) != 0;
		const int other = level->getTile(x, top ? y - 1 : y + 1, z);
		if (other != id || !canSurvive(level, x, y, z)) {
			if (!top) spawnResources(level, x, y, z, 0);
			level->setTile(x, y, z, 0);
		}
	}

	int getResource(int data, Random* random) {
		return random->nextInt(8) == 0 ? Item::seeds_wheat->id : -1;
	}
};

#endif
