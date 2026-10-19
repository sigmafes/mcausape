#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__NetherFortress_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__NetherFortress_H__

#include "StructurePiece.h"
#include "../../biome/Biome.h"

class Level;
class Random;

// The one Nether fortress of a world. The Console Edition, whose Nether is just as
// finite, forces a single fortress near the middle (NetherBridgeFeature); this does
// the same: its place and its whole layout follow from the world seed, so the same
// fortress is worked out again every time the Nether is loaded, and its blocks are
// laid down chunk by chunk as the Nether is generated.
class NetherFortress
{
public:
	NetherFortress(long seed);
	~NetherFortress();

	// Builds the part of the fortress that falls in chunk (chunkX, chunkZ).
	void postProcess(Level* level, Random* random, int chunkX, int chunkZ);

	// Whether a block is inside one of the fortress's pieces: blazes, wither
	// skeletons, pigmen and magma cubes spawn there.
	bool isInside(int x, int y, int z);
	const Biome::MobList& getEnemies() const { return enemies; }

private:
	void build();

	long seed;
	bool built;
	StructurePiece::PieceList pieces;
	BoundingBox* boundingBox;
	Biome::MobList enemies;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__NetherFortress_H__*/
