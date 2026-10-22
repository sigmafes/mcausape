#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Squid_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Squid_H__

#include "WaterAnimal.h"

/** The squid, ported from the Console Edition's (Squid.cpp, Dec 2014). It
    drifts about in water, pushing off with its tentacles, and lies helpless
    and slowly drowns out of it. Drops 0 to 3 ocean shards. Spawns in the
    Overworld's water; the Aquarium has none (yet). */
class Squid: public WaterAnimal
{
	typedef WaterAnimal super;
public:
	Squid(Level* level);

	int getMaxHealth();
	int getEntityTypeId() const;

	bool isInWater();
	void aiStep();
	void travel(float xa, float ya);
	bool canSpawn();

	float xBodyRot, xBodyRotO;
	float zBodyRot, zBodyRotO;
	float tentacleMovement, oldTentacleMovement;
	float tentacleAngle, oldTentacleAngle;

protected:
	bool useNewAi() { return false; }
	void updateAi();
	const char* getAmbientSound() { return NULL; }
	std::string getHurtSound() { return ""; }
	std::string getDeathSound() { return ""; }
	float getSoundVolume() { return 0.4f; }
	int getDeathLoot();
	void dropDeathLoot();

private:
	float speed;
	float tentacleSpeed;
	float rotateSpeed;
	float tx, ty, tz;
	int dryTicks;
};

#endif
