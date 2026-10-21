#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__MagmaCube_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__MagmaCube_H__

#include "Monster.h"

// A bouncing cube of magma, as in the Console Edition's LavaSlime: it does not
// walk but hops, every so often, in the direction it faces -- toward the nearest
// player if one is within 16 blocks, three times as often -- and hurts whoever it
// lands against. squish drives the animation: the cube stretches while it is in
// the air and flattens when it lands (see MagmaCubeRenderer / MagmaCubeModel).
class MagmaCube: public Monster
{
	typedef Monster super;
public:
	MagmaCube(Level* level);

	void tick();
	void updateAi();
	void jumpFromGround();
	void causeFallDamage(float distance);

protected:
	void dropDeathLoot();
	std::string getHurtSound();
	std::string getDeathSound();

public:

	int getMaxHealth();

	int getEntityTypeId() const;

	// Spawns whatever the light: the fortress is lit by glowstone.
	bool canSpawn();

	// Size 2 of the Console Edition's three (1, 2, 4).
	static const int SIZE = 2;

	// Read by the renderer and the model; oSquish is last tick's, to blend between.
	float squish, oSquish, targetSquish;

private:
	int jumpDelay;
	int touchCooldown;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_MONSTER__MagmaCube_H__*/
