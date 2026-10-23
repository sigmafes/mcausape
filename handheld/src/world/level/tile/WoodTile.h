#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__WoodTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__WoodTile_H__

//package net.minecraft.world.level.tile;

#include "MultiTextureTile.h"

class WoodTile: public MultiTextureTile
{
	typedef MultiTextureTile super;
public:
	// The aux values match the ones used by TreeTile (logs):
	// 0 is "normal" oak, 1 is dark/spruce and 2 is birch.
	static const int OAK_PLANKS    = 0;
	static const int SPRUCE_PLANKS = 1;
	static const int BIRCH_PLANKS  = 2;
	static const int CRIMSON_PLANKS = 3;	// from crimson stems; never burn
	static const int WARPED_PLANKS  = 4;	// from warped stems; never burn

	WoodTile(int id, const int* textures, int texCount, const Material* material)
	:	super(id, textures, texCount, material)
	{
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__WoodTile_H__*/
