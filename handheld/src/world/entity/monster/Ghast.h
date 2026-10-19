#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Ghast_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Ghast_H__

#include "../Mob.h"

// A floating enemy of the Nether, ported from the Console Edition's Ghast: it drifts
// about at random, and when a player it can see is within 64 blocks it faces them,
// shuts its eyes for a second (the charge) and fires a fireball, then rests two
// seconds. It does not spawn naturally yet (that comes with the fortresses).
class Ghast: public Mob
{
	typedef Mob super;
public:
	Ghast(Level* level);

	int getMaxHealth();

	void aiStep();

	int getEntityTypeId() const;

	// Counts against the monster cap, and only one try in twenty spawns one.
	int getCreatureBaseType() const;
	bool canSpawn();

	// 0..20 while charging a shot, negative while it recovers.
	int charge;

protected:
	int getDeathLoot();
	void dropDeathLoot();

private:
	float xTarget, yTarget, zTarget;
	int floatDuration;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_MONSTER__Ghast_H__*/
