#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__BrewingStandTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__BrewingStandTile_H__

#include "EntityTile.h"
#include "../material/Material.h"
#include "../../item/Item.h"

/** The brewing stand: a rod on three small pads with three arms, drawn by
    TileRenderer (`drawingBase` picks the base's picture). Its data says
    which arms hold a bottle (BrewingStandTileEntity::updateArms). Used, it
    opens the brewing screen; broken, it drops what is in it. */
class BrewingStandTile: public EntityTile
{
	typedef EntityTile super;
public:
	BrewingStandTile(int id);

	int getTexture(int face, int data) { return drawingBase ? 12 + 9 * 16 : 13 + 9 * 16; }

	bool blocksLight() { return false; }
	bool isSolidRender() { return false; }
	bool isCubeShaped() { return false; }
	int getRenderShape() { return Tile::SHAPE_BREWING_STAND; }
	int getRenderLayer() { return Tile::RENDERLAYER_ALPHATEST; }
	int getResource(int data, Random* random) { return Item::brewingStand->id; }

	TileEntity* newTileEntity();
	bool use(Level* level, int x, int y, int z, Player* player);
	void onRemove(Level* level, int x, int y, int z);

	bool drawingBase;

	/// Burning or not: a second tile (brewingStand_lit) that gives light, as
	/// the furnace has (FurnaceTile::setLit). Its contents stay put.
	static void setLit(bool lit, Level* level, int x, int y, int z);
	static bool noDrop;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__BrewingStandTile_H__*/
