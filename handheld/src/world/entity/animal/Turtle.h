#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Turtle_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Turtle_H__

#include "Fish.h"

/** The Aquarium's sea turtle: swims about slowly as the fish do, and is
    neutral -- whoever hurts it is chased and bitten for half a minute, or
    until they get away. Drops one or two turtle scutes. */
class Turtle: public Fish
{
	typedef Fish super;
public:
	Turtle(Level* level);
	int getEntityTypeId() const;
	int getMaxHealth();
	int getMaxSpawnClusterSize() { return 2; }
	bool hurt(Entity* source, int dmg);
	void aiStep();
protected:
	int getDeathLoot();
	void dropDeathLoot();
private:
	int angryAt;		///< entity id of who hurt it, 0 if calm
	int angryTicks;
	int biteCooldown;
};

#endif
