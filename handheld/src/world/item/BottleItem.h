#ifndef NET_MINECRAFT_WORLD_ITEM__BottleItem_H__
#define NET_MINECRAFT_WORLD_ITEM__BottleItem_H__

#include "Item.h"
#include "../level/Level.h"
#include "../level/material/Material.h"
#include "../entity/player/Player.h"
#include "../entity/player/Inventory.h"
#include "../phys/HitResult.h"
#include "../phys/Vec3.h"

/** A glass bottle: used on water it takes a bottle of it. The water stays where
    it was. The full bottle (a BottleItem with `filled` set) does nothing yet --
    there are no potions to make or drink.

    Finds the water the way the empty bucket does (see BucketItem::use): the ray a
    block is aimed with skips liquids, so this casts its own along the player's aim. */
class BottleItem: public Item
{
	typedef Item super;
public:
	BottleItem(int id, bool filled)
	:	super(id),
		filled(filled)
	{}

	/*@Override*/
	ItemInstance* use(ItemInstance* instance, Level* level, Player* player) {
		if (filled)
			return instance;

		Vec3 dir = player->aimDirection;
		if (dir.dot(dir) < 0.01f)
			dir = player->getViewVector(1);
		const Vec3 from(player->x, player->y, player->z);
		HitResult hit = level->clip(from, from + dir.normalized() * 5.0f, true);
		if (!hit.isHit() || hit.type != TILE)
			return instance;
		if (level->getMaterial(hit.x, hit.y, hit.z) != Material::water)
			return instance;

		if (player->abilities.instabuild) {
			// Endless supply: the bottle in hand is not used up.
			player->inventory->add(new ItemInstance(Item::waterBottle));
			return instance;
		}
		if (instance->count > 1) {
			instance->count--;
			ItemInstance full(Item::waterBottle);
			if (!player->inventory->add(&full))
				player->drop(new ItemInstance(full), false);
		} else {
			instance->id = Item::waterBottle->id;
		}
		return instance;
	}

private:
	bool filled;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__BottleItem_H__*/
