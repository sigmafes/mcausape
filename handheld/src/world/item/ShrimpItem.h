#ifndef NET_MINECRAFT_WORLD_ITEM__ShrimpItem_H__
#define NET_MINECRAFT_WORLD_ITEM__ShrimpItem_H__

#include "FoodItem.h"
#include "../entity/player/Player.h"

/// The shrimp: one heart's half back, and two points of the Aquarium's
/// pressure off.
class ShrimpItem: public FoodItem
{
	typedef FoodItem super;
public:
	ShrimpItem(int id) : super(id, 1, false) {}

	/// Eaten even at full health: it is eaten for the pressure as much.
	ItemInstance* use(ItemInstance* instance, Level* level, Player* player) {
		player->startUsingItem(*instance, getUseDuration(instance));
		return instance;
	}

	ItemInstance useTimeDepleted(ItemInstance* instance, Level* level, Player* player) {
		ItemInstance result = super::useTimeDepleted(instance, level, player);
		player->pressure = player->pressure > 2 ? player->pressure - 2 : 0;
		return result;
	}
};

#endif
