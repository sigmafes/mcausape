#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Hoglin_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Hoglin_H__

#include "Monster.h"

/** Java's hoglin (and zoglin, with `zoglin`). The hoglin: 40 health, a
    charge that throws its target up (6), keeps clear of warped fungus,
    breeds with crimson fungus, drops pork and leather, and turns zoglin
    after 15 seconds out of the Nether. The zoglin is hostile, unbreedable
    and fears nothing. A baby is half the size (data flag, so visitors see
    it), weaker, and grows up in 20 minutes. */
class Hoglin: public Monster
{
	typedef Monster super;
public:
	Hoglin(Level* level, bool zoglin);
	int getEntityTypeId() const;
	int getMaxHealth();
	bool canSpawn();
	void aiStep();
	bool interact(Player* player);
	bool isBaby();
	void setBaby(bool baby);

	void addAdditonalSaveData(CompoundTag* tag);
	void readAdditionalSaveData(CompoundTag* tag);

	const bool zoglin;

protected:
	Entity* findAttackTarget();
	bool doHurtTarget(Entity* target);
	int getDeathLoot();
	void dropDeathLoot();
	const char* getAmbientSound() { return NULL; }
	std::string getHurtSound() { return "mob.zombiepig.zpighurt"; }
	std::string getDeathSound() { return "mob.zombiepig.zpigdeath"; }

private:
	bool nearWarpedFungus();
	int inLove, breedCooldown, outsideTicks, growTicks, fearCheck;
	bool afraid;
};

#endif
