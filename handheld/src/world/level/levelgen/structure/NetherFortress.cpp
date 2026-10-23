#include "NetherFortress.h"
#include "NetherBridgePieces.h"
#include "../../LevelConstants.h"
#include "../../../entity/EntityTypes.h"
#include "../../../../util/Random.h"
#include "../../../../platform/log.h"

NetherFortress::NetherFortress(long seed)
:	seed(seed),
	built(false),
	boundingBox(NULL)
{
	// What spawns inside: the Java Edition's fortress list, with the wither skeleton
	// in place of the skeleton it turns most of them into.
	enemies.push_back(Biome::MobSpawnerData(MobTypes::Blaze, 10, 2, 3));
	enemies.push_back(Biome::MobSpawnerData(MobTypes::PigZombie, 5, 4, 4));
	enemies.push_back(Biome::MobSpawnerData(MobTypes::WitherSkeleton, 8, 2, 4));
	enemies.push_back(Biome::MobSpawnerData(MobTypes::MagmaCube, 3, 4, 4));
	// The bastions' garrison, moved in: piglins (always hostile here,
	// Piglin::inFortress), their brutes, and hoglins.
	enemies.push_back(Biome::MobSpawnerData(MobTypes::Piglin, 6, 2, 4));
	enemies.push_back(Biome::MobSpawnerData(MobTypes::PiglinBrute, 3, 1, 2));
	enemies.push_back(Biome::MobSpawnerData(MobTypes::Hoglin, 4, 2, 3));
}

NetherFortress::~NetherFortress()
{
	for (StructurePiece::PieceList::iterator it = pieces.begin(); it != pieces.end(); ++it)
		delete *it;
	delete boundingBox;
}

void NetherFortress::build()
{
	built = true;

	Random random(seed);
	random.nextInt();

	// One of the 7x7 chunks round the middle of the world, as the Console Edition
	// does in its own small Nether, so the fortress has room to grow on every side.
	const int middle = (LEVEL_WIDTH / 16) / 2;
	int chunk = random.nextInt(49);
	int chunkX = middle - 3 + chunk % 7;
	int chunkZ = middle - 3 + chunk / 7;

	NetherBridgePieces::StartPiece* start = new NetherBridgePieces::StartPiece(&random, (chunkX << 4) + 2, (chunkZ << 4) + 2);
	pieces.push_back(start);
	start->addChildren(start, &pieces, &random);

	// Children grow in a random order, so the fortress does not lean one way.
	std::vector<StructurePiece*>& pending = start->pendingChildren;
	while (!pending.empty()) {
		int pos = random.nextInt((int) pending.size());
		StructurePiece* piece = pending[pos];
		pending.erase(pending.begin() + pos);
		piece->addChildren(start, &pieces, &random);
	}

	boundingBox = BoundingBox::getUnknownBox();
	for (StructurePiece::PieceList::iterator it = pieces.begin(); it != pieces.end(); ++it)
		boundingBox->expand((*it)->getBoundingBox());

	// Put the whole thing between y 48 and 70.
	const int lowestAllowed = 48, highestAllowed = 70;
	int heightSpan = highestAllowed - lowestAllowed + 1 - boundingBox->getYSpan();
	int y0Pos = heightSpan > 1? lowestAllowed + random.nextInt(heightSpan) : lowestAllowed;
	int dy = y0Pos - boundingBox->y0;
	boundingBox->move(0, dy, 0);
	for (StructurePiece::PieceList::iterator it = pieces.begin(); it != pieces.end(); ++it)
		(*it)->getBoundingBox()->move(0, dy, 0);

	LOGI("Nether fortress: %d pieces, from %d,%d,%d to %d,%d,%d\n", (int) pieces.size(),
		boundingBox->x0, boundingBox->y0, boundingBox->z0, boundingBox->x1, boundingBox->y1, boundingBox->z1);
}

void NetherFortress::postProcess(Level* level, Random* random, int chunkX, int chunkZ)
{
	if (!built) build();

	int cx = chunkX << 4;
	int cz = chunkZ << 4;
	if (!boundingBox->intersects(cx, cz, cx + 15, cz + 15))
		return;

	// Each piece lays only the blocks inside this chunk.
	BoundingBox chunkBB(cx, cz, cx + 15, cz + 15);
	for (StructurePiece::PieceList::iterator it = pieces.begin(); it != pieces.end(); ++it) {
		StructurePiece* piece = *it;
		if (piece->getBoundingBox()->intersects(&chunkBB))
			piece->postProcess(level, random, &chunkBB);
	}
}

bool NetherFortress::isInside(int x, int y, int z)
{
	if (!built) build();

	if (!boundingBox->intersects(x, z, x, z))
		return false;
	for (StructurePiece::PieceList::iterator it = pieces.begin(); it != pieces.end(); ++it) {
		if ((*it)->getBoundingBox()->isInside(x, y, z))
			return true;
	}
	return false;
}
