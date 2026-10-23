#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Piglin_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Piglin_H__

#include "Monster.h"

/** Java's piglin (and piglin brute, with `brute`).
    Piglin: 16 health, a golden sword (one in four a golden axe); leaves
    alone a player wearing any piece of gold armour, unless hurt by them.
    Given a gold ingot (used on it, or thrown at its feet) it looks at it
    for six seconds and barters -- drops something from the table. Out of
    the Nether for 15 seconds it becomes a zombified piglin.
    Brute: 50 health, a golden axe, attacks any player, gold or not, never
    barters. Spawns only in bastions (phase 7). */
class Piglin: public Monster
{
	typedef Monster super;
public:
	Piglin(Level* level, bool brute);
	int getEntityTypeId() const;
	int getMaxHealth();
	bool canSpawn();
	void aiStep();
	bool interact(Player* player);
	bool hurt(Entity* source, int dmg);
	ItemInstance* getCarriedItem();
	/// A bastion's brutes stay put, as Java's never despawn.
	bool removeWhenFarAway() { return !brute && super::removeWhenFarAway(); }

	bool admiring();
	bool hasAxe() const;
	bool hasBow() const;
	float getWalkingSpeedModifier();
	void checkHurtTarget(Entity* target, float d);
	const bool brute;

protected:
	Entity* findAttackTarget();
	int getAttackDamage(Entity* target);
	int getDeathLoot() { return 0; }
	void dropDeathLoot();
	const char* getAmbientSound() { return NULL; }
	std::string getHurtSound() { return "mob.zombiepig.zpighurt"; }
	std::string getDeathSound() { return "mob.zombiepig.zpigdeath"; }

private:
	void startAdmiring();
	void barter();
	static bool wearsGold(Player* p);
	int admireTicks, angryAt, angryTicks, outsideTicks, pickupCheck;
	bool inFortress;	///< hostile to every player, gold or not
	int fortressCheck;
};

#endif
