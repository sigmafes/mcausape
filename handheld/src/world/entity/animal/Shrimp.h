#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Shrimp_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Shrimp_H__

#include "Fish.h"

/// The Aquarium's shrimp: swims in schools as the fish do (Fish's AI), is
/// small, and always drops one shrimp.
class Shrimp: public Fish
{
	typedef Fish super;
public:
	Shrimp(Level* level);
	int getEntityTypeId() const;
	int getMaxSpawnClusterSize() { return 6; }
protected:
	int getDeathLoot();
	void dropDeathLoot();
};

#endif
