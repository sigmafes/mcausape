#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Drowned_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Drowned_H__

#include "Zombie.h"

class Player;

/** The Aquarium's drowned: a zombie's health and bite, but in the water it
    hunts as a swimmer -- straight at the nearest player within 48 blocks,
    up, down and across, lying flat in the player's swimming pose. About
    15% carry a trident (looks only; never dropped). */
class Drowned: public Zombie
{
	typedef Zombie super;
public:
	static const int HuntRange = 48;

	Drowned(Level* level);
	int getEntityTypeId() const;
	void travel(float xa, float ya);
	ItemInstance* getCarriedItem();
	bool canBreatheUnderwater() { return true; }
	/// Worked out from the entity id, which host and visitors share.
	bool hasTrident() const;
	/// In water, as of its last tick: the renderer lays it flat.
	bool swimming() const { return wasInWater; }

	/// The nearest player (not in creative) within `range`, or NULL.
	static Player* nearestPrey(Mob* from, float range);

protected:
	/// The monsters' AI "jumps" in water to stay afloat: here that is a lift
	/// every tick, which sent it up like a balloon.
	void updateAi();
};

#endif
