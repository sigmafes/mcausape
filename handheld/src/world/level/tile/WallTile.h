#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__WallTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__WallTile_H__

#include "Tile.h"
#include "../LevelSource.h"
#include "../material/Material.h"
#include "../../phys/AABB.h"

/** A cobblestone wall: a post with low arms to whatever it connects to. Like a
    fence, it is a full block and a half tall to walk into (so nothing steps
    over it) but drawn a little lower, and a post alone stands up straight. */
class WallTile: public Tile
{
	typedef Tile super;
public:
	WallTile(int id, int tex)
	:	super(id, tex, Material::stone)
	{
	}

	bool blocksLight() { return false; }
	bool isSolidRender() { return false; }
	bool isCubeShaped() { return false; }
	int getRenderShape() { return Tile::SHAPE_WALL; }

	/// Walls are drawn with the texture of the block they are made of, on every face.
	int getTexture(int face, int data) { return tex; }

	AABB* getAABB(Level* level, int x, int y, int z) {
		updateShape(level, x, y, z);
		tmpBB.set((float)x + xx0, (float)y, (float)z + zz0, (float)x + xx1, (float)y + 1.5f, (float)z + zz1);
		return &tmpBB;
	}

	void updateShape(LevelSource* level, int x, int y, int z) {
		float west = 4 / 16.0f, east = 12 / 16.0f, north = 4 / 16.0f, south = 12 / 16.0f;
		if (connectsTo(level, x, y, z - 1)) north = 0;
		if (connectsTo(level, x, y, z + 1)) south = 1;
		if (connectsTo(level, x - 1, y, z)) west = 0;
		if (connectsTo(level, x + 1, y, z)) east = 1;
		// A straight run is a thin slab, not a slab with a post in it.
		if (north == 0 && south == 1 && west > 0 && east < 1) { west = 5 / 16.0f; east = 11 / 16.0f; }
		else if (west == 0 && east == 1 && north > 0 && south < 1) { north = 5 / 16.0f; south = 11 / 16.0f; }
		setShape(west, 0, north, east, 1, south);
	}

	bool connectsTo(LevelSource* level, int x, int y, int z) {
		int t = level->getTile(x, y, z);
		if (t <= 0) return false;
		if (Tile::tiles[t] == this || (Tile::tiles[t] != NULL && Tile::tiles[t]->getRenderShape() == Tile::SHAPE_WALL)) return true;
		if (t == Tile::fenceGate->id || t == Tile::fenceGate_spruce->id || t == Tile::fenceGate_birch->id) return true;
		Tile* other = Tile::tiles[t];
		return other != NULL && other->material->isSolidBlocking() && other->isCubeShaped() && other->material != Material::vegetable;
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__WallTile_H__*/
