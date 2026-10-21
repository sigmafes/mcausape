#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__BrewingStandTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__BrewingStandTile_H__

#include "Tile.h"
#include "../material/Material.h"
#include "../../item/Item.h"

/** The brewing stand: a rod on three small pads, drawn by TileRenderer in two
    pictures (`drawingBase` picks which). It does nothing when used -- no
    screen yet -- and is placed from its item, as in the original. */
class BrewingStandTile: public Tile
{
	typedef Tile super;
public:
	BrewingStandTile(int id)
	:	super(id, 13 + 9 * 16, Material::metal),
		drawingBase(false)
	{
		setShape(7 / 16.0f, 0, 7 / 16.0f, 9 / 16.0f, 14 / 16.0f, 9 / 16.0f);
	}

	int getTexture(int face, int data) { return drawingBase ? 12 + 9 * 16 : 13 + 9 * 16; }

	bool blocksLight() { return false; }
	bool isSolidRender() { return false; }
	bool isCubeShaped() { return false; }
	int getRenderShape() { return Tile::SHAPE_BREWING_STAND; }
	int getRenderLayer() { return Tile::RENDERLAYER_ALPHATEST; }
	int getResource(int data, Random* random) { return Item::brewingStand->id; }

	bool drawingBase;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__BrewingStandTile_H__*/
