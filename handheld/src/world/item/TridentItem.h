#ifndef NET_MINECRAFT_WORLD_ITEM__TridentItem_H__
#define NET_MINECRAFT_WORLD_ITEM__TridentItem_H__

#include "WeaponItem.h"

/// The trident: a melee weapon only (never thrown), 6 everywhere, the
/// Aquarium included, where swords hardly hurt (WeaponItem).
class TridentItem: public WeaponItem
{
	typedef WeaponItem super;
public:
	TridentItem(int id) : super(id, Item::Tier::IRON) { setMaxDamage(250); }
	int getAttackDamage(Entity* entity) { return 6; }
};

#endif
