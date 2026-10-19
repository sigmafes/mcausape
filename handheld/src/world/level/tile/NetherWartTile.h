#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__NetherWartTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__NetherWartTile_H__

#include "Bush.h"

// Nether wart: a crop that only grows on soul sand and needs no light. Four ages
// (0..3) drawn with three pictures; ported from the Console Edition's
// NetherStalkTile.
class NetherWartTile : public Bush
{
	typedef Bush super;
public:
	static const int MAX_AGE = 3;

	NetherWartTile(int id, int tex);

	bool mayPlaceOn(int tile);
	bool canSurvive(Level* level, int x, int y, int z);
	void tick(Level* level, int x, int y, int z, Random* random);

	int getTexture(int face, int data);
	int getRenderShape();
	int getRenderLayer();

	void spawnResources(Level* level, int x, int y, int z, int data, float odds);
	int getResource(int data, Random* random);
	int getResourceCount(Random* random);

	void growCropsToMax(Level* level, int x, int y, int z);
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE__NetherWartTile_H__ */
