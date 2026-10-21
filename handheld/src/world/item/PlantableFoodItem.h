#ifndef NET_MINECRAFT_WORLD_ITEM__PlantableFoodItem_H__
#define NET_MINECRAFT_WORLD_ITEM__PlantableFoodItem_H__

#include "FoodItem.h"

/** Food that is also its own seed: a carrot or a potato, eaten with a
    right-click in the air and planted with one on the top of farmland. */
class PlantableFoodItem: public FoodItem
{
	typedef FoodItem super;
public:
	PlantableFoodItem(int id, int nutrition, int resultTile, int targetLand)
	:	super(id, nutrition, false),
		resultTile(resultTile),
		targetLand(targetLand)
	{}

	bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
		if (face != 1) return false;
		if (level->getTile(x, y, z) == targetLand && level->isEmptyTile(x, y + 1, z)) {
			level->setTile(x, y + 1, z, resultTile);
			instance->count--;
			return true;
		}
		return false;
	}

private:
	int resultTile;
	int targetLand;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__PlantableFoodItem_H__*/
