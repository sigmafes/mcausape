#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Blaze_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Blaze_H__

#include "Monster.h"

// A fire spirit of the Nether fortresses, ported from the Console Edition's Blaze:
// it goes after the player like any monster, floats up to meet them, and shoots a
// charge of three small fireballs a few ticks apart, then rests five seconds.
// For now it only comes from monster spawners.
class Blaze: public Monster
{
	typedef Monster super;
public:
	Blaze(Level* level);

	int getMaxHealth();

	void aiStep();

	int getEntityTypeId() const;

	// Spawns whatever the light: the fortress is lit by glowstone.
	bool canSpawn();

protected:
	const char* getAmbientSound();
	std::string getHurtSound();
	std::string getDeathSound();
	void checkHurtTarget(Entity* target, float d);
	int getDeathLoot();
	void dropDeathLoot();

private:
	float allowedHeightOffset;
	int nextHeightOffsetChangeTick;
	int attackCounter;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_MONSTER__Blaze_H__*/
