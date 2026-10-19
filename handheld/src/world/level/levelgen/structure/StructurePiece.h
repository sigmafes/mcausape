#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__StructurePiece_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__StructurePiece_H__

#include <list>
#include "BoundingBox.h"

class Level;
class Random;

/**
 * A construction or room of a structure, with a box in the world and an orientation
 * (a Direction). Ported from the Console Edition's StructurePiece, keeping only what
 * the Nether fortress uses.
 *
 * Local coordinates are turned into world ones by the orientation: with NORTH, local
 * (0,0,0) is at (box.x0, box.y0, box.z1); SOUTH only flips z; EAST and WEST swap x and z.
 * Pieces only ever write inside the chunk being decorated (chunkBB), so building one
 * never asks for a chunk that is not there yet.
 */
class StructurePiece
{
public:
	typedef std::list<StructurePiece*> PieceList;

	// One line of a chest's loot table.
	struct LootEntry {
		int itemId;
		int aux;
		int minCount;
		int maxCount;
		int weight;
	};

	virtual ~StructurePiece();

	virtual void addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random);
	virtual bool postProcess(Level* level, Random* random, BoundingBox* chunkBB) = 0;

	BoundingBox* getBoundingBox() { return boundingBox; }
	int getGenDepth() const { return genDepth; }

	static StructurePiece* findCollisionPiece(PieceList* pieces, BoundingBox* box);

	BoundingBox* boundingBox;

protected:
	StructurePiece(int genDepth);

	int getWorldX(int x, int z);
	int getWorldY(int y);
	int getWorldZ(int x, int z);
	int getOrientationData(int tile, int data);

	void placeBlock(Level* level, int block, int data, int x, int y, int z, BoundingBox* chunkBB);
	int getBlock(Level* level, int x, int y, int z, BoundingBox* chunkBB);
	void generateBox(Level* level, BoundingBox* chunkBB, int x0, int y0, int z0, int x1, int y1, int z1, int edgeTile, int fillTile, bool skipAir);
	void fillColumnDown(Level* level, int tile, int tileData, int x, int startY, int z, BoundingBox* chunkBB);
	// A chest filled with `rolls` picks from the table. False if it was not placed here.
	bool createChest(Level* level, BoundingBox* chunkBB, Random* random, int x, int y, int z, const LootEntry* table, int tableSize, int rolls);

	int orientation;
	int genDepth;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__StructurePiece_H__*/
