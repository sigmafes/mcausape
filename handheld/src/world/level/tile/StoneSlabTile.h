#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__StoneSlabTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__StoneSlabTile_H__

//package net.minecraft.world.level.tile;

#include "Tile.h"

class Level;
class LevelSource;

class StoneSlabTile: public Tile
{
	typedef Tile super;
public:
    static const int STONE_SLAB = 0;
    static const int SAND_SLAB  = 1;
    static const int WOOD_SLAB  = 2;
    static const int COBBLESTONE_SLAB = 3;
	static const int BRICK_SLAB = 4;
	static const int SMOOTHBRICK_SLAB = 5;
	static const int BIRCH_SLAB = 6;
	static const int SPRUCE_SLAB = 7;

	static const int TYPE_MASK = 7;
	static const int TOP_SLOT_BIT = 8;


    static const std::string SLAB_NAMES[];
	// The second pair of slabs (see Tile::stoneSlab2): nether brick and quartz.
	static const int NETHERBRICK_SLAB = 0;
	static const int QUARTZ_SLAB = 1;
	static const int PRISMARINE_BRICK_SLAB = 2;
	static const int DARK_PRISMARINE_SLAB = 3;
	static const std::string SLAB_NAMES2[];
	static const int SLAB_NAMES2_COUNT;
	static const int SLAB_NAMES_COUNT;

    StoneSlabTile(int id, bool fullSize, int slabSet = 0);

	// The half-height tile that belongs with this one, and the full block.
	Tile* getHalfTile();
	Tile* getFullTile();
	// Whether this id is any half slab, of either set.
	static bool isHalfSlabId(int tileId);

	int getTexture(int face, int data);
    int getTexture(int face);

    bool isSolidRender();
    bool isCubeShaped();

	void updateShape(LevelSource* level, int x, int y, int z);
	void updateDefaultShape();

	void addAABBs( Level* level, int x, int y, int z, const AABB* box, std::vector<AABB>& boxes );

	//void onPlace(Level* level, int x, int y, int z);
	int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue);

	int getResource(int data, Random* random);
    int getResourceCount(Random* random);

    bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face);

protected:
    int getSpawnResourcesAuxValue(int data);
private:
	bool fullSize;
	int slabSet;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__StoneSlabTile_H__*/
