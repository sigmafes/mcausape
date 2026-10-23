#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Fish_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Fish_H__

#include "WaterAnimal.h"

/** Java's cod and salmon (AbstractFish): they swim about in water towards a
    random point now and then, dart off when hurt, and out of water flop on
    the ground until they suffocate. Drop the raw fish, and now and then bone
    meal. Spawn in the Overworld's water and in the Aquarium. */
class Fish: public WaterAnimal
{
	typedef WaterAnimal super;
public:
	Fish(Level* level, bool salmon);

	int getMaxHealth();
	int getEntityTypeId() const;
	int getMaxSpawnClusterSize();

	bool isInWater();
	void aiStep();
	void travel(float xa, float ya);
	bool canSpawn();

	const bool salmon;
	/// In water, as of its last tick (for the model and renderer).
	bool swimming() const { return wasInWater; }

protected:
	bool useNewAi() { return false; }
	void updateAi();
	const char* getAmbientSound() { return NULL; }
	std::string getHurtSound() { return ""; }
	std::string getDeathSound() { return ""; }
	int getDeathLoot();
	void dropDeathLoot();

protected:
	bool waterAt(float x, float y, float z);
	/// Out of water, each tick on the host: flop and suffocate.
	virtual void dryStep();
	/// How much faster than a cod it swims.
	virtual float swimScale() { return 1.0f; }
	float tx, ty, tz;
	bool hasTarget;
	int dryTicks;
	int panicTicks;
};

#endif
