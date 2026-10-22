#ifndef NET_MINECRAFT_WORLD_ITEM__PotionItem_H__
#define NET_MINECRAFT_WORLD_ITEM__PotionItem_H__

#include "Item.h"
#include "../level/Level.h"
#include "../entity/player/Player.h"
#include "../entity/player/Inventory.h"
#include "../effect/MobEffect.h"

/** A potion: drunk like food is eaten (the drink animation and sound, no
    crumbs), it gives its effect for Item::PotionDurationTicks and leaves the
    glass bottle. `effect` is a MobEffects id, or -1 for the awkward potion,
    which can be drunk for nothing, as in Java. */
class PotionItem: public Item
{
	typedef Item super;
public:
	PotionItem(int id, int effect)
	:	super(id),
		effect(effect)
	{}

	int getUseDuration(ItemInstance* itemInstance) { return 32; }
	UseAnim::UseAnimation getUseAnimation() { return UseAnim::drink; }

	ItemInstance* use(ItemInstance* instance, Level* level, Player* player) {
		player->startUsingItem(*instance, getUseDuration(instance));
		return instance;
	}

	ItemInstance useTimeDepleted(ItemInstance* instance, Level* level, Player* player) {
		if (effect >= 0)
			player->addEffect(effect, Item::PotionDurationTicks);
		if (player->abilities.instabuild)
			return *instance;
		if (instance->count > 1) {
			instance->count--;
			ItemInstance bottle(Item::glassBottle);
			if (!player->inventory->add(&bottle))
				player->drop(new ItemInstance(bottle), false);
		} else {
			// The last one becomes the empty bottle in the hand.
			instance->id = Item::glassBottle->id;
			instance->count = 1;
		}
		return *instance;
	}

private:
	int effect;
};

#endif
