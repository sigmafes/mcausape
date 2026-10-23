#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__NetherTiles_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__NetherTiles_H__

#include "Tile.h"
#include "Bush.h"
#include "FireTile.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../entity/Entity.h"
#include "../../item/Item.h"

/// Grows on nylium (and, for some, soul soil or netherrack): the fungi,
/// roots and sprouts of the Nether forests. Crossed pictures, no light needed.
class NetherPlantTile: public Bush
{
public:
	NetherPlantTile(int id, int tex) : Bush(id, tex, Material::replaceable_plant) {
		setTicking(false);
		setShape(0.2f, 0, 0.2f, 0.8f, 0.6f, 0.8f);
	}
	bool canSurvive(Level* level, int x, int y, int z) { return mayPlaceOn(level->getTile(x, y - 1, z)); }
protected:
	bool mayPlaceOn(int t) {
		return t == Tile::crimsonNylium->id || t == Tile::warpedNylium->id || t == Tile::soulSoil->id
			|| t == Tile::netherrack->id || t == ((Tile*) Tile::grass)->id || t == Tile::dirt->id;
	}
};

/** Weeping vines (hang down, `down`) and twisting vines (grow up): a column
    like kelp, the end block the tip (tex + 1), the rest the plant (tex).
    TileRenderer hands the cross renderer 1 for a plant block. */
class NetherVineTile: public Tile
{
public:
	const bool down;
	NetherVineTile(int id, int tex, bool down) : Tile(id, tex, Material::replaceable_plant), down(down) {
		setShape(0.25f, 0, 0.25f, 0.75f, 1, 0.75f);
	}
	int getTexture(int face, int data) { return data == 1 ? tex : tex + 1; }
	bool mayPlace(Level* level, int x, int y, int z, unsigned char face) {
		return Tile::mayPlace(level, x, y, z, face) && canSurvive(level, x, y, z);
	}
	bool canSurvive(Level* level, int x, int y, int z) {
		const int s = level->getTile(x, down ? y + 1 : y - 1, z);
		return s == id || (s > 0 && Tile::solid[s]);
	}
	void neighborChanged(Level* level, int x, int y, int z, int type) {
		if (!canSurvive(level, x, y, z)) { spawnResources(level, x, y, z, 0); level->setTile(x, y, z, 0); }
	}
	AABB* getAABB(Level* level, int x, int y, int z) { return NULL; }
	bool blocksLight() { return false; }
	bool isSolidRender() { return false; }
	bool isCubeShaped() { return false; }
	int getRenderShape() { return Tile::SHAPE_CROSS_TEXTURE; }
	int getRenderLayer() { return Tile::RENDERLAYER_ALPHATEST; }
};

/// The magma block: a little light, and it burns what stands on it
/// unless it is sneaking.
class MagmaTile: public Tile
{
public:
	MagmaTile(int id, int tex) : Tile(id, tex, Material::stone) {}
	void stepOn(Level* level, int x, int y, int z, Entity* entity) {
		if (!level->isClientSide && !entity->isSneaking()) entity->hurt(NULL, 1);
	}
};

/// Soul fire: blue fire that burns forever on soul sand and soul soil and
/// nowhere else; it does not spread. Fire put on those becomes it.
class SoulFireTile: public FireTile
{
public:
	SoulFireTile(int id, int tex) : FireTile(id, tex) {}
	static bool soulBase(int t) { return t == Tile::hellSand->id || t == Tile::soulSoil->id; }
	void tick(Level* level, int x, int y, int z, Random* random) {
		if (!soulBase(level->getTile(x, y - 1, z))) level->setTile(x, y, z, 0);
	}
	bool mayPlace(Level* level, int x, int y, int z, unsigned char face) { return soulBase(level->getTile(x, y - 1, z)); }
	void neighborChanged(Level* level, int x, int y, int z, int type) {
		if (!soulBase(level->getTile(x, y - 1, z))) level->setTile(x, y, z, 0);
	}
	void onPlace(Level* level, int x, int y, int z) {
		if (!soulBase(level->getTile(x, y - 1, z))) level->setTile(x, y, z, 0);
	}
};

/// Nether gold ore: 2 to 6 gold nuggets.
class NetherGoldOreTile: public Tile
{
public:
	NetherGoldOreTile(int id, int tex) : Tile(id, tex, Material::stone) {}
	int getResource(int data, Random* random) { return Item::goldNugget->id; }
	int getResourceCount(Random* random) { return 2 + random->nextInt(5); }
};

#endif
