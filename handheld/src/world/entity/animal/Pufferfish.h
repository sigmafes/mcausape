#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Pufferfish_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Pufferfish_H__

#include "Fish.h"

/** Java's pufferfish: a fish that puffs up when a player comes within two
    blocks -- half puffed at once, fully after two seconds -- and lets down
    again a step every three seconds once nobody is near. Puffed, touching a
    player stings (1 + its puff) and poisons for 15 seconds. The puffing is
    worked out on every machine from where the players are, so a visitor
    sees it without a packet; the sting only on the host. */
class Pufferfish: public Fish
{
	typedef Fish super;
public:
	Pufferfish(Level* level);

	int getEntityTypeId() const;
	int getMaxSpawnClusterSize() { return 3; }
	void aiStep();

	int puffState;

protected:
	int getDeathLoot();
	void dropDeathLoot();

private:
	void setPuff(int state);
	int inflateCounter;
	int deflateTimer;
};

#endif
