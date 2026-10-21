#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__EnchantTableTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__EnchantTableTile_H__

#include "Tile.h"
#include "../material/Material.h"

/** The enchanting table: a three-quarter-high block with its own top, side and
    underside. Nothing happens when it is used -- there is no screen yet. */
class EnchantTableTile: public Tile
{
	typedef Tile super;
public:
	EnchantTableTile(int id)
	:	super(id, 6 + 10 * 16, Material::stone)
	{
		updateDefaultShape();
	}

	void updateDefaultShape() { setShape(0, 0, 0, 1, 0.75f, 1); }
	bool blocksLight() { return false; }
	bool isSolidRender() { return false; }
	bool isCubeShaped() { return false; }

	int getTexture(int face, int data) {
		if (face == 1) return 6 + 10 * 16;
		if (face == 0) return 9 + 13 * 16;
		return 14 + 11 * 16;
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__EnchantTableTile_H__*/
