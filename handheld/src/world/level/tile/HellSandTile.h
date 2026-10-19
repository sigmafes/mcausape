#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__HellSandTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__HellSandTile_H__

//package net.minecraft.world.level.tile;

#include "Tile.h"
#include "../material/Material.h"

// Arena de almas (soul sand). En 0.6.1 no tiene comportamiento especial,
// solo su textura y registro (id 88).

class HellSandTile: public Tile
{
public:
	HellSandTile(int id, int tex, const Material* material)
	:	Tile(id, tex, material)
	{
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__HellSandTile_H__*/
