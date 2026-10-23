#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Drowned_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Drowned_H__

#include "Zombie.h"

/** The Aquarium's drowned: a zombie (its health and bite) that swims after
    its target as the player does -- up and down as well as across. One in
    seven or so carries a trident (looks only; it is never dropped). */
class Drowned: public Zombie
{
	typedef Zombie super;
public:
	Drowned(Level* level);
	int getEntityTypeId() const;
	void aiStep();
	void travel(float xa, float ya);
	ItemInstance* getCarriedItem();
	/// Worked out from the entity id, which host and visitors share.
	bool hasTrident() const;
	bool canBreatheUnderwater() { return true; }
};

#endif
