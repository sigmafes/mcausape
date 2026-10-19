#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__MagmaCube_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__MagmaCube_H__

#include "Monster.h"

// A bouncing cube of magma that walks up to the player and hits. Its fire is not
// in yet, and it does not hop: it walks like the other monsters.
class MagmaCube: public Monster
{
	typedef Monster super;
public:
	MagmaCube(Level* level);

	int getMaxHealth();

	int getEntityTypeId() const;

	// Spawns whatever the light: the fortress is lit by glowstone.
	bool canSpawn();
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_MONSTER__MagmaCube_H__*/
