#include "NetherBridgePieces.h"
#include "../../Level.h"
#include "../../LevelConstants.h"
#include "../../storage/LevelData.h"
#include "../../tile/Tile.h"
#include "../../tile/MobSpawnerTile.h"
#include "../../../Direction.h"
#include "../../../item/Item.h"
#include "../../../../util/Random.h"
#include <cstdlib>

namespace NetherBridgePieces
{

static const int MAX_DEPTH = 30;
// the fortress starts at 64 and reaches down to here at the lowest
static const int LOWEST_Y_POSITION = 10;

// Shorthands for the two blocks nearly everything is made of.
#define NB (Tile::netherBrick->id)
#define NF (Tile::netherFence->id)

// The fortress chest, as in the Java Edition (1.7) minus what this game has no item
// for (saddles, horse armour), with the bastions' loot added in.
static int fortressLoot(StructurePiece::LootEntry* out)
{
	StructurePiece::LootEntry table[] = {
		{ Item::emerald->id,          0, 1, 3, 5 },    // diamond
		{ Item::ironIngot->id,        0, 1, 5, 5 },
		{ Item::goldIngot->id,        0, 1, 3, 15 },
		{ Item::sword_gold->id,       0, 1, 1, 5 },
		{ Item::chestplate_gold->id,  0, 1, 1, 5 },
		{ Item::flintAndSteel->id,    0, 1, 1, 5 },
		{ Item::netherStalkSeeds->id, 0, 3, 7, 5 },
		{ Tile::obsidian->id,         0, 2, 4, 2 },
		// From Java's bastion chests.
		{ Item::goldNugget->id,       0, 4, 12, 8 },
		{ Item::netheriteScrap->id,   0, 1, 2, 3 },
		{ Tile::ancientDebris->id,    0, 1, 2, 3 },
		{ Item::magmaCream->id,       0, 2, 5, 5 },
		{ Item::hatchet_gold->id,     0, 1, 1, 4 },
		{ Item::boots_gold->id,       0, 1, 1, 4 },
		{ Tile::goldBlock->id,        0, 1, 1, 2 },
		{ Item::netheriteIngot->id,   0, 1, 1, 1 },
	};
	int n = sizeof(table) / sizeof(table[0]);
	for (int i = 0; i < n; i++)
		out[i] = table[i];
	return n;
}

/** The long walks' hazards (new worlds' fortresses): small lava pits let
    into the walking surface - a block or two, walled by the deck round them
    - and a few patches of magma, inside x0..x1, z0..z1 at height y. Picked
    from the piece's position so the chunks it spans agree. */
void NetherBridgePiece::hazards(Level* level, BoundingBox* chunkBB, int x0, int x1, int z0, int z1, int y)
{
	if (!level->getLevelData() || level->getLevelData()->getGeneratorVersion() < 1) return;
	Random rr((long)boundingBox->x0 * 734287LL + (long)boundingBox->z0 * 912931LL + boundingBox->y0 * 31);
	const int w = x1 - x0 + 1, d = z1 - z0 + 1;
	const int pits = rr.nextInt(d > 8 ? 3 : 2);
	for (int i = 0; i < pits; ++i) {
		const int x = x0 + rr.nextInt(w), z = z0 + rr.nextInt(d);
		placeBlock(level, Tile::calmLava->id, 0, x, y, z, chunkBB);
		if (rr.nextInt(2) && z + 1 <= z1) placeBlock(level, Tile::calmLava->id, 0, x, y, z + 1, chunkBB);
	}
	const int magma = 1 + rr.nextInt(d > 8 ? 4 : 2);
	for (int i = 0; i < magma; ++i) {
		const int x = x0 + rr.nextInt(w), z = z0 + rr.nextInt(d);
		for (int k = 0; k < 2 + rr.nextInt(3); ++k) {
			const int mx = x + rr.nextInt(3) - 1, mz = z + rr.nextInt(3) - 1;
			if (mx >= x0 && mx <= x1 && mz >= z0 && mz <= z1 && getBlock(level, mx, y, mz, chunkBB) == NB)
				placeBlock(level, Tile::magmaBlock->id, 0, mx, y, mz, chunkBB);
		}
	}
}

//
// Factory
//
static NetherBridgePiece* findAndCreateBridgePieceFactory(PieceWeight* piece, StructurePiece::PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int depth)
{
	switch (piece->pieceClass) {
	case EPieceClass_BridgeStraight:   return BridgeStraight::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_BridgeCrossing:   return BridgeCrossing::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_RoomCrossing:     return RoomCrossing::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_StairsRoom:       return StairsRoom::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_MonsterThrone:    return MonsterThrone::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_CastleEntrance:   return CastleEntrance::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_CastleSmallCorridorPiece:          return CastleSmallCorridorPiece::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_CastleSmallCorridorRightTurnPiece: return CastleSmallCorridorRightTurnPiece::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_CastleSmallCorridorLeftTurnPiece:  return CastleSmallCorridorLeftTurnPiece::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_CastleCorridorStairsPiece:         return CastleCorridorStairsPiece::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_CastleCorridorTBalconyPiece:       return CastleCorridorTBalconyPiece::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_CastleSmallCorridorCrossingPiece:  return CastleSmallCorridorCrossingPiece::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	case EPieceClass_CastleStalkRoom:  return CastleStalkRoom::createPiece(pieces, random, footX, footY, footZ, direction, depth);
	default:                           return NULL;
	}
}

// Every createPiece is the same: make the box, and give up if it leaves the world or
// runs into another piece.
#define CREATE_PIECE(Class, offX, offY, offZ) \
Class* Class::createPiece(PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int genDepth) \
{ \
	BoundingBox* box = BoundingBox::orientBox(footX, footY, footZ, offX, offY, offZ, width, height, depth, direction); \
	if (!isOkBox(box) || StructurePiece::findCollisionPiece(pieces, box) != NULL) { \
		delete box; \
		return NULL; \
	} \
	return new Class(genDepth, random, box, direction); \
}

#define PIECE_CTOR(Class) \
Class::Class(int genDepth, Random* random, BoundingBox* box, int direction) \
:	NetherBridgePiece(genDepth) \
{ \
	orientation = direction; \
	boundingBox = box; \
}

//
// NetherBridgePiece
//
NetherBridgePiece::NetherBridgePiece(int genDepth)
:	StructurePiece(genDepth)
{
}

int NetherBridgePiece::updatePieceWeight(WeightList* currentPieces)
{
	bool hasAnyPieces = false;
	int totalWeight = 0;
	for (WeightList::iterator it = currentPieces->begin(); it != currentPieces->end(); ++it) {
		PieceWeight* piece = *it;
		if (piece->maxPlaceCount > 0 && piece->placeCount < piece->maxPlaceCount)
			hasAnyPieces = true;
		totalWeight += piece->weight;
	}
	return hasAnyPieces? totalWeight : -1;
}

NetherBridgePiece* NetherBridgePiece::generatePiece(StartPiece* startPiece, WeightList* currentPieces, PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int depth)
{
	int totalWeight = updatePieceWeight(currentPieces);
	bool doStuff = totalWeight > 0 && depth <= MAX_DEPTH;

	int numAttempts = 0;
	while (numAttempts < 5 && doStuff) {
		numAttempts++;

		int weightSelection = random->nextInt(totalWeight);
		for (WeightList::iterator it = currentPieces->begin(); it != currentPieces->end(); ++it) {
			PieceWeight* piece = *it;
			weightSelection -= piece->weight;
			if (weightSelection < 0) {
				if (!piece->doPlace(depth) || (piece == startPiece->previousPiece && !piece->allowInRow))
					break;

				NetherBridgePiece* structurePiece = findAndCreateBridgePieceFactory(piece, pieces, random, footX, footY, footZ, direction, depth);
				if (structurePiece != NULL) {
					piece->placeCount++;
					startPiece->previousPiece = piece;
					if (!piece->isValid())
						currentPieces->remove(piece);
					return structurePiece;
				}
			}
		}
	}
	return BridgeEndFiller::createPiece(pieces, random, footX, footY, footZ, direction, depth);
}

StructurePiece* NetherBridgePiece::generateAndAddPiece(StartPiece* startPiece, PieceList* pieces, Random* random, int footX, int footY, int footZ, int direction, int depth, bool isCastle)
{
	// Grows no further than 7 chunks from where it started.
	if (std::abs(footX - startPiece->getBoundingBox()->x0) > 7 * 16 || std::abs(footZ - startPiece->getBoundingBox()->z0) > 7 * 16)
		return BridgeEndFiller::createPiece(pieces, random, footX, footY, footZ, direction, depth);

	WeightList* availablePieces = isCastle? &startPiece->availableCastlePieces : &startPiece->availableBridgePieces;
	StructurePiece* newPiece = generatePiece(startPiece, availablePieces, pieces, random, footX, footY, footZ, direction, depth + 1);
	if (newPiece != NULL) {
		pieces->push_back(newPiece);
		startPiece->pendingChildren.push_back(newPiece);
	}
	return newPiece;
}

StructurePiece* NetherBridgePiece::generateChildForward(StartPiece* startPiece, PieceList* pieces, Random* random, int xOff, int yOff, bool isCastle)
{
	switch (orientation) {
	case Direction::NORTH: return generateAndAddPiece(startPiece, pieces, random, boundingBox->x0 + xOff, boundingBox->y0 + yOff, boundingBox->z0 - 1, orientation, getGenDepth(), isCastle);
	case Direction::SOUTH: return generateAndAddPiece(startPiece, pieces, random, boundingBox->x0 + xOff, boundingBox->y0 + yOff, boundingBox->z1 + 1, orientation, getGenDepth(), isCastle);
	case Direction::WEST:  return generateAndAddPiece(startPiece, pieces, random, boundingBox->x0 - 1, boundingBox->y0 + yOff, boundingBox->z0 + xOff, orientation, getGenDepth(), isCastle);
	case Direction::EAST:  return generateAndAddPiece(startPiece, pieces, random, boundingBox->x1 + 1, boundingBox->y0 + yOff, boundingBox->z0 + xOff, orientation, getGenDepth(), isCastle);
	}
	return NULL;
}

StructurePiece* NetherBridgePiece::generateChildLeft(StartPiece* startPiece, PieceList* pieces, Random* random, int yOff, int zOff, bool isCastle)
{
	switch (orientation) {
	case Direction::NORTH:
	case Direction::SOUTH: return generateAndAddPiece(startPiece, pieces, random, boundingBox->x0 - 1, boundingBox->y0 + yOff, boundingBox->z0 + zOff, Direction::WEST, getGenDepth(), isCastle);
	case Direction::WEST:
	case Direction::EAST:  return generateAndAddPiece(startPiece, pieces, random, boundingBox->x0 + zOff, boundingBox->y0 + yOff, boundingBox->z0 - 1, Direction::NORTH, getGenDepth(), isCastle);
	}
	return NULL;
}

StructurePiece* NetherBridgePiece::generateChildRight(StartPiece* startPiece, PieceList* pieces, Random* random, int yOff, int zOff, bool isCastle)
{
	switch (orientation) {
	case Direction::NORTH:
	case Direction::SOUTH: return generateAndAddPiece(startPiece, pieces, random, boundingBox->x1 + 1, boundingBox->y0 + yOff, boundingBox->z0 + zOff, Direction::EAST, getGenDepth(), isCastle);
	case Direction::WEST:
	case Direction::EAST:  return generateAndAddPiece(startPiece, pieces, random, boundingBox->x0 + zOff, boundingBox->y0 + yOff, boundingBox->z1 + 1, Direction::SOUTH, getGenDepth(), isCastle);
	}
	return NULL;
}

// A piece must stay above y 10 and inside the finite world (0 .. LEVEL_WIDTH - 1,
// leaving the outermost column free).
bool NetherBridgePiece::isOkBox(BoundingBox* box)
{
	if (box == NULL) return false;
	if (box->y0 <= LOWEST_Y_POSITION) return false;
	if (box->x0 <= 0 || box->z0 <= 0) return false;
	if (box->x1 >= LEVEL_WIDTH - 1 || box->z1 >= LEVEL_DEPTH - 1) return false;
	return true;
}

//
// BridgeStraight
//
PIECE_CTOR(BridgeStraight)
CREATE_PIECE(BridgeStraight, -1, -3, 0)

void BridgeStraight::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildForward((StartPiece*) startPiece, pieces, random, 1, 3, false);
}

bool BridgeStraight::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 3, 0, width - 1, 4, depth - 1, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 1, 5, 0, 3, 7, depth - 1, 0, 0, false);

	// hand rails
	generateBox(level, chunkBB, 0, 5, 0, 0, 5, depth - 1, NB, NB, false);
	generateBox(level, chunkBB, 4, 5, 0, 4, 5, depth - 1, NB, NB, false);

	// supports
	generateBox(level, chunkBB, 0, 2, 0, 4, 2, 5, NB, NB, false);
	generateBox(level, chunkBB, 0, 2, 13, 4, 2, 18, NB, NB, false);
	generateBox(level, chunkBB, 0, 0, 0, 4, 1, 3, NB, NB, false);
	generateBox(level, chunkBB, 0, 0, 15, 4, 1, 18, NB, NB, false);

	for (int x = 0; x <= 4; x++) {
		for (int z = 0; z <= 2; z++) {
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);
			fillColumnDown(level, NB, 0, x, -1, 18 - z, chunkBB);
		}
	}

	generateBox(level, chunkBB, 0, 1, 1, 0, 4, 1, NF, NF, false);
	generateBox(level, chunkBB, 0, 3, 4, 0, 4, 4, NF, NF, false);
	generateBox(level, chunkBB, 0, 3, 14, 0, 4, 14, NF, NF, false);
	generateBox(level, chunkBB, 0, 1, 17, 0, 4, 17, NF, NF, false);
	generateBox(level, chunkBB, 4, 1, 1, 4, 4, 1, NF, NF, false);
	generateBox(level, chunkBB, 4, 3, 4, 4, 4, 4, NF, NF, false);
	generateBox(level, chunkBB, 4, 3, 14, 4, 4, 14, NF, NF, false);
	generateBox(level, chunkBB, 4, 1, 17, 4, 4, 17, NF, NF, false);

	hazards(level, chunkBB, 1, 3, 3, 15, 4);
	return true;
}

//
// BridgeEndFiller: a broken-off end of a bridge
//
BridgeEndFiller::BridgeEndFiller(int genDepth, Random* random, BoundingBox* box, int direction)
:	NetherBridgePiece(genDepth)
{
	orientation = direction;
	boundingBox = box;
	selfSeed = random->nextInt();
}

CREATE_PIECE(BridgeEndFiller, -1, -3, 0)

bool BridgeEndFiller::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	Random selfRandom(selfSeed);

	// floor
	for (int x = 0; x <= 4; x++) {
		for (int y = 3; y <= 4; y++) {
			int z = selfRandom.nextInt(8);
			generateBox(level, chunkBB, x, y, 0, x, y, z, NB, NB, false);
		}
	}

	// hand rails
	{
		int z = selfRandom.nextInt(8);
		generateBox(level, chunkBB, 0, 5, 0, 0, 5, z, NB, NB, false);
	}
	{
		int z = selfRandom.nextInt(8);
		generateBox(level, chunkBB, 4, 5, 0, 4, 5, z, NB, NB, false);
	}

	// supports
	for (int x = 0; x <= 4; x++) {
		int z = selfRandom.nextInt(5);
		generateBox(level, chunkBB, x, 2, 0, x, 2, z, NB, NB, false);
	}
	for (int x = 0; x <= 4; x++) {
		for (int y = 0; y <= 1; y++) {
			int z = selfRandom.nextInt(3);
			generateBox(level, chunkBB, x, y, 0, x, y, z, NB, NB, false);
		}
	}

	return true;
}

//
// BridgeCrossing
//
PIECE_CTOR(BridgeCrossing)
CREATE_PIECE(BridgeCrossing, -8, -3, 0)

BridgeCrossing::BridgeCrossing(Random* random, int west, int north)
:	NetherBridgePiece(0)
{
	orientation = random->nextInt(4);

	switch (orientation) {
	case Direction::NORTH:
	case Direction::SOUTH:
		boundingBox = new BoundingBox(west, 64, north, west + width - 1, 64 + height - 1, north + depth - 1);
		break;
	default:
		boundingBox = new BoundingBox(west, 64, north, west + depth - 1, 64 + height - 1, north + width - 1);
		break;
	}
}

void BridgeCrossing::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildForward((StartPiece*) startPiece, pieces, random, 8, 3, false);
	generateChildLeft((StartPiece*) startPiece, pieces, random, 3, 8, false);
	generateChildRight((StartPiece*) startPiece, pieces, random, 3, 8, false);
}

bool BridgeCrossing::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 7, 3, 0, 11, 4, 18, NB, NB, false);
	generateBox(level, chunkBB, 0, 3, 7, 18, 4, 11, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 8, 5, 0, 10, 7, 18, 0, 0, false);
	generateBox(level, chunkBB, 0, 5, 8, 18, 7, 10, 0, 0, false);
	// hand rails
	generateBox(level, chunkBB, 7, 5, 0, 7, 5, 7, NB, NB, false);
	generateBox(level, chunkBB, 7, 5, 11, 7, 5, 18, NB, NB, false);
	generateBox(level, chunkBB, 11, 5, 0, 11, 5, 7, NB, NB, false);
	generateBox(level, chunkBB, 11, 5, 11, 11, 5, 18, NB, NB, false);
	generateBox(level, chunkBB, 0, 5, 7, 7, 5, 7, NB, NB, false);
	generateBox(level, chunkBB, 11, 5, 7, 18, 5, 7, NB, NB, false);
	generateBox(level, chunkBB, 0, 5, 11, 7, 5, 11, NB, NB, false);
	generateBox(level, chunkBB, 11, 5, 11, 18, 5, 11, NB, NB, false);

	// supports
	generateBox(level, chunkBB, 7, 2, 0, 11, 2, 5, NB, NB, false);
	generateBox(level, chunkBB, 7, 2, 13, 11, 2, 18, NB, NB, false);
	generateBox(level, chunkBB, 7, 0, 0, 11, 1, 3, NB, NB, false);
	generateBox(level, chunkBB, 7, 0, 15, 11, 1, 18, NB, NB, false);
	for (int x = 7; x <= 11; x++) {
		for (int z = 0; z <= 2; z++) {
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);
			fillColumnDown(level, NB, 0, x, -1, 18 - z, chunkBB);
		}
	}

	generateBox(level, chunkBB, 0, 2, 7, 5, 2, 11, NB, NB, false);
	generateBox(level, chunkBB, 13, 2, 7, 18, 2, 11, NB, NB, false);
	generateBox(level, chunkBB, 0, 0, 7, 3, 1, 11, NB, NB, false);
	generateBox(level, chunkBB, 15, 0, 7, 18, 1, 11, NB, NB, false);
	for (int x = 0; x <= 2; x++) {
		for (int z = 7; z <= 11; z++) {
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);
			fillColumnDown(level, NB, 0, 18 - x, -1, z, chunkBB);
		}
	}

	return true;
}

//
// StartPiece
//
StartPiece::StartPiece(Random* random, int west, int north)
:	BridgeCrossing(random, west, north),
	previousPiece(NULL)
{
	// The Console Edition's weights, raised for the throne, entrance and garden so
	// that every fortress has blazes and nether wart.
	bridgeWeights[0] = PieceWeight(EPieceClass_BridgeStraight, 30, 0, true);
	bridgeWeights[1] = PieceWeight(EPieceClass_BridgeCrossing, 10, 4);
	bridgeWeights[2] = PieceWeight(EPieceClass_MonsterThrone, 15, 2);
	bridgeWeights[3] = PieceWeight(EPieceClass_CastleEntrance, 15, 1);
	bridgeWeights[4] = PieceWeight(EPieceClass_RoomCrossing, 10, 4);
	bridgeWeights[5] = PieceWeight(EPieceClass_StairsRoom, 10, 3);

	castleWeights[0] = PieceWeight(EPieceClass_CastleStalkRoom, 30, 2);
	castleWeights[1] = PieceWeight(EPieceClass_CastleSmallCorridorPiece, 25, 0, true);
	castleWeights[2] = PieceWeight(EPieceClass_CastleSmallCorridorCrossingPiece, 15, 5);
	castleWeights[3] = PieceWeight(EPieceClass_CastleSmallCorridorRightTurnPiece, 5, 10);
	castleWeights[4] = PieceWeight(EPieceClass_CastleSmallCorridorLeftTurnPiece, 5, 10);
	castleWeights[5] = PieceWeight(EPieceClass_CastleCorridorStairsPiece, 10, 3, true);
	castleWeights[6] = PieceWeight(EPieceClass_CastleCorridorTBalconyPiece, 7, 2);

	for (int i = 0; i < 6; i++) availableBridgePieces.push_back(&bridgeWeights[i]);
	for (int i = 0; i < 7; i++) availableCastlePieces.push_back(&castleWeights[i]);
}

//
// RoomCrossing
//
PIECE_CTOR(RoomCrossing)
CREATE_PIECE(RoomCrossing, -2, 0, 0)

void RoomCrossing::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildForward((StartPiece*) startPiece, pieces, random, 2, 0, false);
	generateChildLeft((StartPiece*) startPiece, pieces, random, 0, 2, false);
	generateChildRight((StartPiece*) startPiece, pieces, random, 0, 2, false);
}

bool RoomCrossing::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 0, 0, width - 1, 1, depth - 1, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 0, 2, 0, 6, 7, 6, 0, 0, false);

	// walls
	generateBox(level, chunkBB, 0, 2, 0, 1, 6, 0, NB, NB, false);
	generateBox(level, chunkBB, 0, 2, 6, 1, 6, 6, NB, NB, false);
	generateBox(level, chunkBB, 5, 2, 0, 6, 6, 0, NB, NB, false);
	generateBox(level, chunkBB, 5, 2, 6, 6, 6, 6, NB, NB, false);
	generateBox(level, chunkBB, 0, 2, 0, 0, 6, 1, NB, NB, false);
	generateBox(level, chunkBB, 0, 2, 5, 0, 6, 6, NB, NB, false);
	generateBox(level, chunkBB, 6, 2, 0, 6, 6, 1, NB, NB, false);
	generateBox(level, chunkBB, 6, 2, 5, 6, 6, 6, NB, NB, false);

	// entries
	generateBox(level, chunkBB, 2, 6, 0, 4, 6, 0, NB, NB, false);
	generateBox(level, chunkBB, 2, 5, 0, 4, 5, 0, NF, NF, false);
	generateBox(level, chunkBB, 2, 6, 6, 4, 6, 6, NB, NB, false);
	generateBox(level, chunkBB, 2, 5, 6, 4, 5, 6, NF, NF, false);
	generateBox(level, chunkBB, 0, 6, 2, 0, 6, 4, NB, NB, false);
	generateBox(level, chunkBB, 0, 5, 2, 0, 5, 4, NF, NF, false);
	generateBox(level, chunkBB, 6, 6, 2, 6, 6, 4, NB, NB, false);
	generateBox(level, chunkBB, 6, 5, 2, 6, 5, 4, NF, NF, false);

	for (int x = 0; x <= 6; x++)
		for (int z = 0; z <= 6; z++)
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);

	return true;
}

//
// StairsRoom
//
PIECE_CTOR(StairsRoom)
CREATE_PIECE(StairsRoom, -2, 0, 0)

void StairsRoom::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildRight((StartPiece*) startPiece, pieces, random, 6, 2, false);
}

bool StairsRoom::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 0, 0, width - 1, 1, depth - 1, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 0, 2, 0, 6, 10, 6, 0, 0, false);

	// walls
	generateBox(level, chunkBB, 0, 2, 0, 1, 8, 0, NB, NB, false);
	generateBox(level, chunkBB, 5, 2, 0, 6, 8, 0, NB, NB, false);
	generateBox(level, chunkBB, 0, 2, 1, 0, 8, 6, NB, NB, false);
	generateBox(level, chunkBB, 6, 2, 1, 6, 8, 6, NB, NB, false);
	generateBox(level, chunkBB, 1, 2, 6, 5, 8, 6, NB, NB, false);

	// wall decorations
	generateBox(level, chunkBB, 0, 3, 2, 0, 5, 4, NF, NF, false);
	generateBox(level, chunkBB, 6, 3, 2, 6, 5, 2, NF, NF, false);
	generateBox(level, chunkBB, 6, 3, 4, 6, 5, 4, NF, NF, false);

	// stair
	placeBlock(level, NB, 0, 5, 2, 5, chunkBB);
	generateBox(level, chunkBB, 4, 2, 5, 4, 3, 5, NB, NB, false);
	generateBox(level, chunkBB, 3, 2, 5, 3, 4, 5, NB, NB, false);
	generateBox(level, chunkBB, 2, 2, 5, 2, 5, 5, NB, NB, false);
	generateBox(level, chunkBB, 1, 2, 5, 1, 6, 5, NB, NB, false);

	// top floor
	generateBox(level, chunkBB, 1, 7, 1, 5, 7, 4, NB, NB, false);
	generateBox(level, chunkBB, 6, 8, 2, 6, 8, 4, 0, 0, false);

	// entries
	generateBox(level, chunkBB, 2, 6, 0, 4, 8, 0, NB, NB, false);
	generateBox(level, chunkBB, 2, 5, 0, 4, 5, 0, NF, NF, false);

	for (int x = 0; x <= 6; x++)
		for (int z = 0; z <= 6; z++)
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);

	return true;
}

//
// MonsterThrone
//
MonsterThrone::MonsterThrone(int genDepth, Random* random, BoundingBox* box, int direction)
:	NetherBridgePiece(genDepth),
	hasPlacedMobSpawner(false)
{
	orientation = direction;
	boundingBox = box;
}

CREATE_PIECE(MonsterThrone, -2, 0, 0)

bool MonsterThrone::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	generateBox(level, chunkBB, 0, 2, 0, 6, 7, 7, 0, 0, false);

	// floors
	generateBox(level, chunkBB, 1, 0, 0, 5, 1, 7, NB, NB, false);
	generateBox(level, chunkBB, 1, 2, 1, 5, 2, 7, NB, NB, false);
	generateBox(level, chunkBB, 1, 3, 2, 5, 3, 7, NB, NB, false);
	generateBox(level, chunkBB, 1, 4, 3, 5, 4, 7, NB, NB, false);

	// rails
	generateBox(level, chunkBB, 1, 2, 0, 1, 4, 2, NB, NB, false);
	generateBox(level, chunkBB, 5, 2, 0, 5, 4, 2, NB, NB, false);
	generateBox(level, chunkBB, 1, 5, 2, 1, 5, 3, NB, NB, false);
	generateBox(level, chunkBB, 5, 5, 2, 5, 5, 3, NB, NB, false);
	generateBox(level, chunkBB, 0, 5, 3, 0, 5, 8, NB, NB, false);
	generateBox(level, chunkBB, 6, 5, 3, 6, 5, 8, NB, NB, false);
	generateBox(level, chunkBB, 1, 5, 8, 5, 5, 8, NB, NB, false);

	placeBlock(level, NF, 0, 1, 6, 3, chunkBB);
	placeBlock(level, NF, 0, 5, 6, 3, chunkBB);
	generateBox(level, chunkBB, 0, 6, 3, 0, 6, 8, NF, NF, false);
	generateBox(level, chunkBB, 6, 6, 3, 6, 6, 8, NF, NF, false);
	generateBox(level, chunkBB, 1, 6, 8, 5, 7, 8, NF, NF, false);
	generateBox(level, chunkBB, 2, 8, 8, 4, 8, 8, NF, NF, false);

	// the blaze spawner
	if (!hasPlacedMobSpawner) {
		int y = getWorldY(5), x = getWorldX(3, 5), z = getWorldZ(3, 5);
		if (chunkBB->isInside(x, y, z)) {
			hasPlacedMobSpawner = true;
			level->setTileAndData(x, y, z, Tile::mobSpawner->id, MobSpawnerTile::BLAZE);
		}
	}

	for (int x = 0; x <= 6; x++)
		for (int z = 0; z <= 6; z++)
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);

	return true;
}

//
// CastleEntrance
//
PIECE_CTOR(CastleEntrance)
CREATE_PIECE(CastleEntrance, -5, -3, 0)

void CastleEntrance::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildForward((StartPiece*) startPiece, pieces, random, 5, 3, true);
}

bool CastleEntrance::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 3, 0, 12, 4, 12, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 0, 5, 0, 12, 13, 12, 0, 0, false);

	// walls
	generateBox(level, chunkBB, 0, 5, 0, 1, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 11, 5, 0, 12, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 2, 5, 11, 4, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 8, 5, 11, 10, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 5, 9, 11, 7, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 2, 5, 0, 4, 12, 1, NB, NB, false);
	generateBox(level, chunkBB, 8, 5, 0, 10, 12, 1, NB, NB, false);
	generateBox(level, chunkBB, 5, 9, 0, 7, 12, 1, NB, NB, false);

	// roof
	generateBox(level, chunkBB, 2, 11, 2, 10, 12, 10, NB, NB, false);

	// entrance decoration
	generateBox(level, chunkBB, 5, 8, 0, 7, 8, 0, NF, NF, false);

	// wall decorations
	for (int i = 1; i <= 11; i += 2) {
		generateBox(level, chunkBB, i, 10, 0, i, 11, 0, NF, NF, false);
		generateBox(level, chunkBB, i, 10, 12, i, 11, 12, NF, NF, false);
		generateBox(level, chunkBB, 0, 10, i, 0, 11, i, NF, NF, false);
		generateBox(level, chunkBB, 12, 10, i, 12, 11, i, NF, NF, false);
		placeBlock(level, NB, 0, i, 13, 0, chunkBB);
		placeBlock(level, NB, 0, i, 13, 12, chunkBB);
		placeBlock(level, NB, 0, 0, 13, i, chunkBB);
		placeBlock(level, NB, 0, 12, 13, i, chunkBB);
		placeBlock(level, NF, 0, i + 1, 13, 0, chunkBB);
		placeBlock(level, NF, 0, i + 1, 13, 12, chunkBB);
		placeBlock(level, NF, 0, 0, 13, i + 1, chunkBB);
		placeBlock(level, NF, 0, 12, 13, i + 1, chunkBB);
	}
	placeBlock(level, NF, 0, 0, 13, 0, chunkBB);
	placeBlock(level, NF, 0, 0, 13, 12, chunkBB);
	placeBlock(level, NF, 0, 0, 13, 0, chunkBB);
	placeBlock(level, NF, 0, 12, 13, 0, chunkBB);

	// inside decorations
	for (int z = 3; z <= 9; z += 2) {
		generateBox(level, chunkBB, 1, 7, z, 1, 8, z, NF, NF, false);
		generateBox(level, chunkBB, 11, 7, z, 11, 8, z, NF, NF, false);
	}

	// supports
	generateBox(level, chunkBB, 4, 2, 0, 8, 2, 12, NB, NB, false);
	generateBox(level, chunkBB, 0, 2, 4, 12, 2, 8, NB, NB, false);

	generateBox(level, chunkBB, 4, 0, 0, 8, 1, 3, NB, NB, false);
	generateBox(level, chunkBB, 4, 0, 9, 8, 1, 12, NB, NB, false);
	generateBox(level, chunkBB, 0, 0, 4, 3, 1, 8, NB, NB, false);
	generateBox(level, chunkBB, 9, 0, 4, 12, 1, 8, NB, NB, false);

	for (int x = 4; x <= 8; x++) {
		for (int z = 0; z <= 2; z++) {
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);
			fillColumnDown(level, NB, 0, x, -1, 12 - z, chunkBB);
		}
	}
	for (int x = 0; x <= 2; x++) {
		for (int z = 4; z <= 8; z++) {
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);
			fillColumnDown(level, NB, 0, 12 - x, -1, z, chunkBB);
		}
	}

	// lava well
	generateBox(level, chunkBB, 5, 5, 5, 7, 5, 7, NB, NB, false);
	generateBox(level, chunkBB, 6, 1, 6, 6, 4, 6, 0, 0, false);
	placeBlock(level, NB, 0, 6, 0, 6, chunkBB);
	placeBlock(level, Tile::lava->id, 0, 6, 5, 6, chunkBB);
	// make the lava flow at once
	int x = getWorldX(6, 6);
	int y = getWorldY(5);
	int z = getWorldZ(6, 6);
	if (chunkBB->isInside(x, y, z)) {
		level->instaTick = true;
		Tile::lava->tick(level, x, y, z, random);
		level->instaTick = false;
	}

	return true;
}

//
// CastleStalkRoom: the nether wart garden
//
PIECE_CTOR(CastleStalkRoom)
CREATE_PIECE(CastleStalkRoom, -5, -3, 0)

void CastleStalkRoom::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildForward((StartPiece*) startPiece, pieces, random, 5, 3, true);
	generateChildForward((StartPiece*) startPiece, pieces, random, 5, 11, true);
}

bool CastleStalkRoom::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 3, 0, 12, 4, 12, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 0, 5, 0, 12, 13, 12, 0, 0, false);

	// walls
	generateBox(level, chunkBB, 0, 5, 0, 1, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 11, 5, 0, 12, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 2, 5, 11, 4, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 8, 5, 11, 10, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 5, 9, 11, 7, 12, 12, NB, NB, false);
	generateBox(level, chunkBB, 2, 5, 0, 4, 12, 1, NB, NB, false);
	generateBox(level, chunkBB, 8, 5, 0, 10, 12, 1, NB, NB, false);
	generateBox(level, chunkBB, 5, 9, 0, 7, 12, 1, NB, NB, false);

	// roof
	generateBox(level, chunkBB, 2, 11, 2, 10, 12, 10, NB, NB, false);

	// wall decorations
	for (int i = 1; i <= 11; i += 2) {
		generateBox(level, chunkBB, i, 10, 0, i, 11, 0, NF, NF, false);
		generateBox(level, chunkBB, i, 10, 12, i, 11, 12, NF, NF, false);
		generateBox(level, chunkBB, 0, 10, i, 0, 11, i, NF, NF, false);
		generateBox(level, chunkBB, 12, 10, i, 12, 11, i, NF, NF, false);
		placeBlock(level, NB, 0, i, 13, 0, chunkBB);
		placeBlock(level, NB, 0, i, 13, 12, chunkBB);
		placeBlock(level, NB, 0, 0, 13, i, chunkBB);
		placeBlock(level, NB, 0, 12, 13, i, chunkBB);
		placeBlock(level, NF, 0, i + 1, 13, 0, chunkBB);
		placeBlock(level, NF, 0, i + 1, 13, 12, chunkBB);
		placeBlock(level, NF, 0, 0, 13, i + 1, chunkBB);
		placeBlock(level, NF, 0, 12, 13, i + 1, chunkBB);
	}
	placeBlock(level, NF, 0, 0, 13, 0, chunkBB);
	placeBlock(level, NF, 0, 0, 13, 12, chunkBB);
	placeBlock(level, NF, 0, 0, 13, 0, chunkBB);
	placeBlock(level, NF, 0, 12, 13, 0, chunkBB);

	// inside decorations
	for (int z = 3; z <= 9; z += 2) {
		generateBox(level, chunkBB, 1, 7, z, 1, 8, z, NF, NF, false);
		generateBox(level, chunkBB, 11, 7, z, 11, 8, z, NF, NF, false);
	}

	// inside stair
	const int stairs = Tile::stairs_netherBricks->id;
	int stairOrientation = getOrientationData(stairs, 3);
	for (int i = 0; i <= 6; i++) {
		int z = i + 4;
		for (int x = 5; x <= 7; x++)
			placeBlock(level, stairs, stairOrientation, x, 5 + i, z, chunkBB);
		if (z >= 5 && z <= 8)
			generateBox(level, chunkBB, 5, 5, z, 7, i + 4, z, NB, NB, false);
		else if (z >= 9 && z <= 10)
			generateBox(level, chunkBB, 5, 8, z, 7, i + 4, z, NB, NB, false);
		if (i >= 1)
			generateBox(level, chunkBB, 5, 6 + i, z, 7, 9 + i, z, 0, 0, false);
	}
	for (int x = 5; x <= 7; x++)
		placeBlock(level, stairs, stairOrientation, x, 12, 11, chunkBB);
	generateBox(level, chunkBB, 5, 6, 7, 5, 7, 7, NF, NF, false);
	generateBox(level, chunkBB, 7, 6, 7, 7, 7, 7, NF, NF, false);
	generateBox(level, chunkBB, 5, 13, 12, 7, 13, 12, 0, 0, false);

	// farmland catwalks
	generateBox(level, chunkBB, 2, 5, 2, 3, 5, 3, NB, NB, false);
	generateBox(level, chunkBB, 2, 5, 9, 3, 5, 10, NB, NB, false);
	generateBox(level, chunkBB, 2, 5, 4, 2, 5, 8, NB, NB, false);
	generateBox(level, chunkBB, 9, 5, 2, 10, 5, 3, NB, NB, false);
	generateBox(level, chunkBB, 9, 5, 9, 10, 5, 10, NB, NB, false);
	generateBox(level, chunkBB, 10, 5, 4, 10, 5, 8, NB, NB, false);
	int eastOrientation = getOrientationData(stairs, 0);
	int westOrientation = getOrientationData(stairs, 1);
	placeBlock(level, stairs, westOrientation, 4, 5, 2, chunkBB);
	placeBlock(level, stairs, westOrientation, 4, 5, 3, chunkBB);
	placeBlock(level, stairs, westOrientation, 4, 5, 9, chunkBB);
	placeBlock(level, stairs, westOrientation, 4, 5, 10, chunkBB);
	placeBlock(level, stairs, eastOrientation, 8, 5, 2, chunkBB);
	placeBlock(level, stairs, eastOrientation, 8, 5, 3, chunkBB);
	placeBlock(level, stairs, eastOrientation, 8, 5, 9, chunkBB);
	placeBlock(level, stairs, eastOrientation, 8, 5, 10, chunkBB);

	// farmlands: soul sand with nether wart on it
	generateBox(level, chunkBB, 3, 4, 4, 4, 4, 8, Tile::hellSand->id, Tile::hellSand->id, false);
	generateBox(level, chunkBB, 8, 4, 4, 9, 4, 8, Tile::hellSand->id, Tile::hellSand->id, false);
	generateBox(level, chunkBB, 3, 5, 4, 4, 5, 8, Tile::netherStalk->id, Tile::netherStalk->id, false);
	generateBox(level, chunkBB, 8, 5, 4, 9, 5, 8, Tile::netherStalk->id, Tile::netherStalk->id, false);
	// 2-5 of the soul sand are gold blocks (the bastions' treasure). Chosen
	// from the piece's position, not the chunk's random, so every chunk this
	// room spans agrees on which.
	{
		Random rr((long)boundingBox->x0 * 341873128712LL + (long)boundingBox->z0 * 132897987541LL + boundingBox->y0);
		const int n = 2 + rr.nextInt(4);
		for (int i = 0; i < n; ++i) {
			const int bx = (rr.nextInt(2) ? 8 : 3) + rr.nextInt(2), bz = 4 + rr.nextInt(5);
			placeBlock(level, Tile::goldBlock->id, 0, bx, 4, bz, chunkBB);
			placeBlock(level, 0, 0, bx, 5, bz, chunkBB);
		}
	}

	// supports
	generateBox(level, chunkBB, 4, 2, 0, 8, 2, 12, NB, NB, false);
	generateBox(level, chunkBB, 0, 2, 4, 12, 2, 8, NB, NB, false);

	generateBox(level, chunkBB, 4, 0, 0, 8, 1, 3, NB, NB, false);
	generateBox(level, chunkBB, 4, 0, 9, 8, 1, 12, NB, NB, false);
	generateBox(level, chunkBB, 0, 0, 4, 3, 1, 8, NB, NB, false);
	generateBox(level, chunkBB, 9, 0, 4, 12, 1, 8, NB, NB, false);

	for (int x = 4; x <= 8; x++) {
		for (int z = 0; z <= 2; z++) {
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);
			fillColumnDown(level, NB, 0, x, -1, 12 - z, chunkBB);
		}
	}
	for (int x = 0; x <= 2; x++) {
		for (int z = 4; z <= 8; z++) {
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);
			fillColumnDown(level, NB, 0, 12 - x, -1, z, chunkBB);
		}
	}

	return true;
}

//
// CastleSmallCorridorPiece
//
PIECE_CTOR(CastleSmallCorridorPiece)
CREATE_PIECE(CastleSmallCorridorPiece, -1, 0, 0)

void CastleSmallCorridorPiece::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildForward((StartPiece*) startPiece, pieces, random, 1, 0, true);
}

bool CastleSmallCorridorPiece::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 0, 0, 4, 1, 4, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 0, 2, 0, 4, 5, 4, 0, 0, false);

	// walls
	generateBox(level, chunkBB, 0, 2, 0, 0, 5, 4, NB, NB, false);
	generateBox(level, chunkBB, 4, 2, 0, 4, 5, 4, NB, NB, false);
	generateBox(level, chunkBB, 0, 3, 1, 0, 4, 1, NF, NF, false);
	generateBox(level, chunkBB, 0, 3, 3, 0, 4, 3, NF, NF, false);
	generateBox(level, chunkBB, 4, 3, 1, 4, 4, 1, NF, NF, false);
	generateBox(level, chunkBB, 4, 3, 3, 4, 4, 3, NF, NF, false);

	// roof
	generateBox(level, chunkBB, 0, 6, 0, 4, 6, 4, NB, NB, false);

	// supports
	for (int x = 0; x <= 4; x++)
		for (int z = 0; z <= 4; z++)
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);

	hazards(level, chunkBB, 1, 3, 0, 4, 1);
	return true;
}

//
// CastleSmallCorridorCrossingPiece
//
PIECE_CTOR(CastleSmallCorridorCrossingPiece)
CREATE_PIECE(CastleSmallCorridorCrossingPiece, -1, 0, 0)

void CastleSmallCorridorCrossingPiece::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildForward((StartPiece*) startPiece, pieces, random, 1, 0, true);
	generateChildLeft((StartPiece*) startPiece, pieces, random, 0, 1, true);
	generateChildRight((StartPiece*) startPiece, pieces, random, 0, 1, true);
}

bool CastleSmallCorridorCrossingPiece::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 0, 0, 4, 1, 4, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 0, 2, 0, 4, 5, 4, 0, 0, false);

	// walls
	generateBox(level, chunkBB, 0, 2, 0, 0, 5, 0, NB, NB, false);
	generateBox(level, chunkBB, 4, 2, 0, 4, 5, 0, NB, NB, false);
	generateBox(level, chunkBB, 0, 2, 4, 0, 5, 4, NB, NB, false);
	generateBox(level, chunkBB, 4, 2, 4, 4, 5, 4, NB, NB, false);

	// roof
	generateBox(level, chunkBB, 0, 6, 0, 4, 6, 4, NB, NB, false);

	// supports
	for (int x = 0; x <= 4; x++)
		for (int z = 0; z <= 4; z++)
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);

	return true;
}

//
// CastleSmallCorridorRightTurnPiece (one in three has a chest)
//
CastleSmallCorridorRightTurnPiece::CastleSmallCorridorRightTurnPiece(int genDepth, Random* random, BoundingBox* box, int direction)
:	NetherBridgePiece(genDepth)
{
	orientation = direction;
	boundingBox = box;
	isNeedingChest = random->nextInt(3) == 0;
}

CREATE_PIECE(CastleSmallCorridorRightTurnPiece, -1, 0, 0)

void CastleSmallCorridorRightTurnPiece::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildRight((StartPiece*) startPiece, pieces, random, 0, 1, true);
}

bool CastleSmallCorridorRightTurnPiece::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 0, 0, 4, 1, 4, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 0, 2, 0, 4, 5, 4, 0, 0, false);

	// walls
	generateBox(level, chunkBB, 0, 2, 0, 0, 5, 4, NB, NB, false);
	generateBox(level, chunkBB, 0, 3, 1, 0, 4, 1, NF, NF, false);
	generateBox(level, chunkBB, 0, 3, 3, 0, 4, 3, NF, NF, false);

	generateBox(level, chunkBB, 4, 2, 0, 4, 5, 0, NB, NB, false);

	generateBox(level, chunkBB, 1, 2, 4, 4, 5, 4, NB, NB, false);
	generateBox(level, chunkBB, 1, 3, 4, 1, 4, 4, NF, NB, false);
	generateBox(level, chunkBB, 3, 3, 4, 3, 4, 4, NF, NB, false);

	if (isNeedingChest && chunkBB->isInside(getWorldX(1, 3), getWorldY(2), getWorldZ(1, 3))) {
		isNeedingChest = false;
		StructurePiece::LootEntry loot[16];
		int n = fortressLoot(loot);
		createChest(level, chunkBB, random, 1, 2, 3, loot, n, 2 + random->nextInt(4));
	}

	// roof
	generateBox(level, chunkBB, 0, 6, 0, 4, 6, 4, NB, NB, false);

	// supports
	for (int x = 0; x <= 4; x++)
		for (int z = 0; z <= 4; z++)
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);

	return true;
}

//
// CastleSmallCorridorLeftTurnPiece (one in three has a chest)
//
CastleSmallCorridorLeftTurnPiece::CastleSmallCorridorLeftTurnPiece(int genDepth, Random* random, BoundingBox* box, int direction)
:	NetherBridgePiece(genDepth)
{
	orientation = direction;
	boundingBox = box;
	isNeedingChest = random->nextInt(3) == 0;
}

CREATE_PIECE(CastleSmallCorridorLeftTurnPiece, -1, 0, 0)

void CastleSmallCorridorLeftTurnPiece::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildLeft((StartPiece*) startPiece, pieces, random, 0, 1, true);
}

bool CastleSmallCorridorLeftTurnPiece::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 0, 0, 4, 1, 4, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 0, 2, 0, 4, 5, 4, 0, 0, false);

	// walls
	generateBox(level, chunkBB, 4, 2, 0, 4, 5, 4, NB, NB, false);
	generateBox(level, chunkBB, 4, 3, 1, 4, 4, 1, NF, NF, false);
	generateBox(level, chunkBB, 4, 3, 3, 4, 4, 3, NF, NF, false);

	generateBox(level, chunkBB, 0, 2, 0, 0, 5, 0, NB, NB, false);

	generateBox(level, chunkBB, 0, 2, 4, 3, 5, 4, NB, NB, false);
	generateBox(level, chunkBB, 1, 3, 4, 1, 4, 4, NF, NB, false);
	generateBox(level, chunkBB, 3, 3, 4, 3, 4, 4, NF, NB, false);

	if (isNeedingChest && chunkBB->isInside(getWorldX(3, 3), getWorldY(2), getWorldZ(3, 3))) {
		isNeedingChest = false;
		StructurePiece::LootEntry loot[16];
		int n = fortressLoot(loot);
		createChest(level, chunkBB, random, 3, 2, 3, loot, n, 2 + random->nextInt(4));
	}

	// roof
	generateBox(level, chunkBB, 0, 6, 0, 4, 6, 4, NB, NB, false);

	// supports
	for (int x = 0; x <= 4; x++)
		for (int z = 0; z <= 4; z++)
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);

	return true;
}

//
// CastleCorridorStairsPiece
//
PIECE_CTOR(CastleCorridorStairsPiece)
CREATE_PIECE(CastleCorridorStairsPiece, -1, -7, 0)

void CastleCorridorStairsPiece::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	generateChildForward((StartPiece*) startPiece, pieces, random, 1, 0, true);
}

bool CastleCorridorStairsPiece::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	const int stairs = Tile::stairs_netherBricks->id;
	int stairsData = getOrientationData(stairs, 2);
	for (int step = 0; step <= 9; step++) {
		int floor = 7 - step; if (floor < 1) floor = 1;
		int roof = floor + 5; if (14 - step > roof) roof = 14 - step; if (roof > 13) roof = 13;
		int z = step;

		// floor
		generateBox(level, chunkBB, 0, 0, z, 4, floor, z, NB, NB, false);
		// room air
		generateBox(level, chunkBB, 1, floor + 1, z, 3, roof - 1, z, 0, 0, false);
		if (step <= 6) {
			placeBlock(level, stairs, stairsData, 1, floor + 1, z, chunkBB);
			placeBlock(level, stairs, stairsData, 2, floor + 1, z, chunkBB);
			placeBlock(level, stairs, stairsData, 3, floor + 1, z, chunkBB);
		}
		// roof
		generateBox(level, chunkBB, 0, roof, z, 4, roof, z, NB, NB, false);
		// walls
		generateBox(level, chunkBB, 0, floor + 1, z, 0, roof - 1, z, NB, NB, false);
		generateBox(level, chunkBB, 4, floor + 1, z, 4, roof - 1, z, NB, NB, false);
		if ((step & 1) == 0) {
			generateBox(level, chunkBB, 0, floor + 2, z, 0, floor + 3, z, NF, NF, false);
			generateBox(level, chunkBB, 4, floor + 2, z, 4, floor + 3, z, NF, NF, false);
		}

		// supports
		for (int x = 0; x <= 4; x++)
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);
	}

	return true;
}

//
// CastleCorridorTBalconyPiece
//
PIECE_CTOR(CastleCorridorTBalconyPiece)
CREATE_PIECE(CastleCorridorTBalconyPiece, -3, 0, 0)

void CastleCorridorTBalconyPiece::addChildren(StructurePiece* startPiece, PieceList* pieces, Random* random)
{
	int zOff = 1;
	// compensate for weird negative-facing behaviour
	if (orientation == Direction::WEST || orientation == Direction::NORTH)
		zOff = 5;

	generateChildLeft((StartPiece*) startPiece, pieces, random, 0, zOff, random->nextInt(8) > 0);
	generateChildRight((StartPiece*) startPiece, pieces, random, 0, zOff, random->nextInt(8) > 0);
}

bool CastleCorridorTBalconyPiece::postProcess(Level* level, Random* random, BoundingBox* chunkBB)
{
	// floor
	generateBox(level, chunkBB, 0, 0, 0, 8, 1, 8, NB, NB, false);
	// room air
	generateBox(level, chunkBB, 0, 2, 0, 8, 5, 8, 0, 0, false);
	// corridor roof
	generateBox(level, chunkBB, 0, 6, 0, 8, 6, 5, NB, NB, false);

	// inside walls
	generateBox(level, chunkBB, 0, 2, 0, 2, 5, 0, NB, NB, false);
	generateBox(level, chunkBB, 6, 2, 0, 8, 5, 0, NB, NB, false);
	generateBox(level, chunkBB, 1, 3, 0, 1, 4, 0, NF, NF, false);
	generateBox(level, chunkBB, 7, 3, 0, 7, 4, 0, NF, NF, false);

	// balcony floor
	generateBox(level, chunkBB, 0, 2, 4, 8, 2, 8, NB, NB, false);
	generateBox(level, chunkBB, 1, 1, 4, 2, 2, 4, 0, 0, false);
	generateBox(level, chunkBB, 6, 1, 4, 7, 2, 4, 0, 0, false);

	// hand rails
	generateBox(level, chunkBB, 0, 3, 8, 8, 3, 8, NF, NF, false);
	generateBox(level, chunkBB, 0, 3, 6, 0, 3, 7, NF, NF, false);
	generateBox(level, chunkBB, 8, 3, 6, 8, 3, 7, NF, NF, false);

	// balcony walls
	generateBox(level, chunkBB, 0, 3, 4, 0, 5, 5, NB, NB, false);
	generateBox(level, chunkBB, 8, 3, 4, 8, 5, 5, NB, NB, false);
	generateBox(level, chunkBB, 1, 3, 5, 2, 5, 5, NB, NB, false);
	generateBox(level, chunkBB, 6, 3, 5, 7, 5, 5, NB, NB, false);
	generateBox(level, chunkBB, 1, 4, 5, 1, 5, 5, NF, NF, false);
	generateBox(level, chunkBB, 7, 4, 5, 7, 5, 5, NF, NF, false);

	// supports
	for (int z = 0; z <= 5; z++)
		for (int x = 0; x <= 8; x++)
			fillColumnDown(level, NB, 0, x, -1, z, chunkBB);

	return true;
}

}
