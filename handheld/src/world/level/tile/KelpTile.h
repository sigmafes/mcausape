#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__KelpTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__KelpTile_H__

#include "Tile.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../item/Item.h"

/// Java's kelp: a column of crossed pictures, the top one the tip (tex + 1).
/// Both are animated (KelpTexture). It stands on anything solid or on kelp;
/// the Aquarium's water is air, so it needs no water round it.
class KelpTile: public Tile
{
public:
	KelpTile(int id, int tex) : Tile(id, tex, Material::plant) {
		setShape(0.15f, 0, 0.15f, 0.85f, 1, 0.85f);
	}

	int getTexture(LevelSource* level, int x, int y, int z, int face) {
		return level->getTile(x, y + 1, z) == id ? tex : tex + 1;
	}
	int getTexture(int face, int data) { return tex + 1; }

	bool mayPlace(Level* level, int x, int y, int z, unsigned char face) {
		return Tile::mayPlace(level, x, y, z, face) && canSurvive(level, x, y, z);
	}
	bool canSurvive(Level* level, int x, int y, int z) {
		const int below = level->getTile(x, y - 1, z);
		return below == id || (below > 0 && Tile::solid[below]);
	}
	void neighborChanged(Level* level, int x, int y, int z, int type) {
		if (!canSurvive(level, x, y, z)) {
			spawnResources(level, x, y, z, 0);
			level->setTile(x, y, z, 0);
		}
	}
	int getResource(int data, Random* random) { return Item::kelp->id; }

	AABB* getAABB(Level* level, int x, int y, int z) { return NULL; }
	bool blocksLight() { return false; }
	bool isSolidRender() { return false; }
	bool isCubeShaped() { return false; }
	int getRenderShape() { return Tile::SHAPE_CROSS_TEXTURE; }
	int getRenderLayer() { return Tile::RENDERLAYER_ALPHATEST; }
};

#endif
