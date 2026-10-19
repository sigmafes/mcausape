#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__WitherSkeleton_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__WitherSkeleton_H__

#include "Skeleton.h"

// A darker skeleton with no bow, which fights in melee. Ported from the Console
// Edition's WitherSkeleton. It only appears from monster spawners for now: the
// Nether fortresses that spawn it naturally come later. Its wither effect is not
// in yet either, so its hits do plain damage.
class WitherSkeleton: public Skeleton
{
	typedef Skeleton super;
public:
	WitherSkeleton(Level* level);

	int getMaxHealth();

	void aiStep();

	int getDeathLoot();
	void dropDeathLoot();

	ItemInstance* getCarriedItem();

	int getEntityTypeId() const;

	// Spawns whatever the light: the fortress is lit by glowstone.
	bool canSpawn();

protected:
	void checkHurtTarget(Entity* target, float d);

private:
	ItemInstance sword;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_MONSTER__WitherSkeleton_H__*/
