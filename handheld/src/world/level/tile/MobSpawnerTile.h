#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__MobSpawnerTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__MobSpawnerTile_H__

#include "EntityTile.h"

// The monster spawner: a solid cage that keeps putting one kind of mob near a player
// while one is close enough (MobSpawnerTileEntity does the work). The kind is the
// block's data value, so each one has its own colour of cage, its own name and its
// own icon.
class MobSpawnerTile: public EntityTile
{
	typedef EntityTile super;
public:
	static const int ZOMBIE = 0;
	static const int SKELETON = 1;
	static const int SPIDER = 2;
	static const int PIG_ZOMBIE = 3;
	static const int WITHER_SKELETON = 4;
	static const int BLAZE = 5;
	static const int MAGMA_CUBE = 6;
	static const int GHAST = 7;
	static const int KIND_COUNT = 8;

	static const std::string KIND_NAMES[];

	MobSpawnerTile(int id, int tex);

	static int mobTypeFor(int data);

	TileEntity* newTileEntity();

	int getTexture(int face, int data);

	// The cage is bars with gaps: drawn with the alpha test so the gaps are
	// clear, and not solid so what is behind them (and the faces of the blocks
	// next to it) are still drawn.
	int getRenderLayer() { return RENDERLAYER_ALPHATEST; }
	bool isSolidRender() { return false; }

	void tick(Level* level, int x, int y, int z, Random* random);
	int getResource(int data, Random* random);
	int getResourceCount(Random* random);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__MobSpawnerTile_H__*/
