#ifndef NET_MINECRAFT_WORLD_ITEM__PoisonFoodItem_H__
#define NET_MINECRAFT_WORLD_ITEM__PoisonFoodItem_H__

#include "FoodItem.h"
#include "../effect/MobEffect.h"
#include "../entity/player/Player.h"

/// Food that poisons whoever eats it: the pufferfish, the spider eye.
class PoisonFoodItem: public FoodItem
{
	typedef FoodItem super;
public:
	PoisonFoodItem(int id, int nutrition, int poisonTicks)
	:	super(id, nutrition, false),
		poisonTicks(poisonTicks)
	{}

	ItemInstance useTimeDepleted(ItemInstance* instance, Level* level, Player* player) {
		ItemInstance result = super::useTimeDepleted(instance, level, player);
		player->addEffect(MobEffects::Poison, poisonTicks);
		return result;
	}

private:
	int poisonTicks;
};

#endif
