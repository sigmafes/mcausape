#include "StructurePiece.h"
#include "../../Level.h"
#include "../../material/Material.h"
#include "../../tile/Tile.h"
#include "../../tile/entity/TileEntity.h"
#include "../../tile/entity/ChestTileEntity.h"
#include "../../../Direction.h"
#include "../../../item/ItemInstance.h"
#include "../../../../util/Random.h"

StructurePiece::StructurePiece(int genDepth)
:	boundingBox(NULL),
	orientation(Direction::UNDEFINED),
	genDepth(genDepth)
{
}

StructurePiece::~StructurePiece()
{
	delete boundingBox;
}

void StructurePiece::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
}

StructurePiece* StructurePiece::findCollisionPiece(PieceList* pieces, BoundingBox* box)
{
	for (PieceList::iterator it = pieces->begin(); it != pieces->end(); ++it) {
		StructurePiece* piece = *it;
		if (piece->getBoundingBox() != NULL && piece->getBoundingBox()->intersects(box))
			return piece;
	}
	return NULL;
}

int StructurePiece::getWorldX(int x, int z)
{
	switch (orientation) {
	case Direction::NORTH:
	case Direction::SOUTH:
		return boundingBox->x0 + x;
	case Direction::WEST:
		return boundingBox->x1 - z;
	case Direction::EAST:
		return boundingBox->x0 + z;
	default:
		return x;
	}
}

int StructurePiece::getWorldY(int y)
{
	if (orientation == Direction::UNDEFINED)
		return y;
	return y + boundingBox->y0;
}

int StructurePiece::getWorldZ(int x, int z)
{
	switch (orientation) {
	case Direction::NORTH:
		return boundingBox->z1 - z;
	case Direction::SOUTH:
		return boundingBox->z0 + z;
	case Direction::WEST:
	case Direction::EAST:
		return boundingBox->z0 + x;
	default:
		return z;
	}
}

// Only stairs need turning in the fortress.
int StructurePiece::getOrientationData(int tile, int data)
{
	if (tile == Tile::stairs_netherBricks->id) {
		if (orientation == Direction::SOUTH) {
			if (data == 2) return 3;
			if (data == 3) return 2;
		} else if (orientation == Direction::WEST) {
			if (data == 0) return 2;
			if (data == 1) return 3;
			if (data == 2) return 0;
			if (data == 3) return 1;
		} else if (orientation == Direction::EAST) {
			if (data == 0) return 2;
			if (data == 1) return 3;
			if (data == 2) return 1;
			if (data == 3) return 0;
		}
	}
	return data;
}

void StructurePiece::placeBlock(Level* level, int block, int data, int x, int y, int z, BoundingBox* chunkBB)
{
	int worldX = getWorldX(x, z);
	int worldY = getWorldY(y);
	int worldZ = getWorldZ(x, z);

	if (!chunkBB->isInside(worldX, worldY, worldZ))
		return;
	// never through the bedrock floor
	if (worldY == 0)
		return;

	level->setTileAndDataNoUpdate(worldX, worldY, worldZ, block, data);
}

int StructurePiece::getBlock(Level* level, int x, int y, int z, BoundingBox* chunkBB)
{
	int worldX = getWorldX(x, z);
	int worldY = getWorldY(y);
	int worldZ = getWorldZ(x, z);

	if (!chunkBB->isInside(worldX, worldY, worldZ))
		return 0;
	return level->getTile(worldX, worldY, worldZ);
}

void StructurePiece::generateBox(Level* level, BoundingBox* chunkBB, int x0, int y0, int z0, int x1, int y1, int z1, int edgeTile, int fillTile, bool skipAir)
{
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			for (int z = z0; z <= z1; z++) {
				if (skipAir && getBlock(level, x, y, z, chunkBB) == 0)
					continue;
				if (y == y0 || y == y1 || x == x0 || x == x1 || z == z0 || z == z1)
					placeBlock(level, edgeTile, 0, x, y, z, chunkBB);
				else
					placeBlock(level, fillTile, 0, x, y, z, chunkBB);
			}
		}
	}
}

// Pillars: fills down from startY through air and liquid until it meets something.
void StructurePiece::fillColumnDown(Level* level, int tile, int tileData, int x, int startY, int z, BoundingBox* chunkBB)
{
	int worldX = getWorldX(x, z);
	int worldY = getWorldY(startY);
	int worldZ = getWorldZ(x, z);

	if (!chunkBB->isInside(worldX, worldY, worldZ))
		return;

	while ((level->isEmptyTile(worldX, worldY, worldZ) || level->getMaterial(worldX, worldY, worldZ)->isLiquid()) && worldY > 1) {
		level->setTileAndDataNoUpdate(worldX, worldY, worldZ, tile, tileData);
		worldY--;
	}
}

bool StructurePiece::createChest(Level* level, BoundingBox* chunkBB, Random* random, int x, int y, int z, const LootEntry* table, int tableSize, int rolls)
{
	int worldX = getWorldX(x, z);
	int worldY = getWorldY(y);
	int worldZ = getWorldZ(x, z);

	if (!chunkBB->isInside(worldX, worldY, worldZ))
		return false;
	if (level->getTile(worldX, worldY, worldZ) == Tile::chest->id)
		return false;

	level->setTile(worldX, worldY, worldZ, Tile::chest->id);
	TileEntity* te = level->getTileEntity(worldX, worldY, worldZ);
	if (!TileEntity::isType(te, TileEntityType::Chest))
		return true;
	ChestTileEntity* chest = (ChestTileEntity*) te;

	int totalWeight = 0;
	for (int i = 0; i < tableSize; i++)
		totalWeight += table[i].weight;

	for (int r = 0; r < rolls; r++) {
		int pick = random->nextInt(totalWeight);
		const LootEntry* e = table;
		for (int i = 0; i < tableSize; i++) {
			pick -= table[i].weight;
			if (pick < 0) { e = &table[i]; break; }
		}
		int count = e->minCount + random->nextInt(e->maxCount - e->minCount + 1);
		int slot = random->nextInt(chest->getContainerSize());
		chest->setItem(slot, new ItemInstance(e->itemId, count, e->aux));
	}
	return true;
}
