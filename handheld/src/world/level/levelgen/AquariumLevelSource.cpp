#include "AquariumLevelSource.h"
#include "../Level.h"
#include "../ChunkPos.h"
#include "../chunk/LevelChunk.h"
#include "../tile/Tile.h"
#include "../dimension/AquariumDimension.h"
#include "../../../util/Mth.h"
#include <math.h>
#include <string.h>

AquariumLevelSource::AquariumLevelSource(Level* level, long seed)
:	level(level),
	seed(seed)
{
}

AquariumLevelSource::~AquariumLevelSource()
{
}

int AquariumLevelSource::sandTop(int x, int z) const
{
	// Two long, slow swells at odd angles: dunes a couple of blocks high, no
	// noise tables needed. The seed only shifts their phase.
	const float p = (float) (seed % 1000) * 0.37f;
	const float a = sinf(x * 0.045f + p) * cosf(z * 0.038f - p * 0.5f);
	const float b = sinf((x + z) * 0.021f + p * 0.3f);
	return 4 + (int) Mth::floor(a * 1.6f + b * 1.2f + 0.5f);
}

LevelChunk* AquariumLevelSource::getChunk(int cx, int cz)
{
	const int key = ChunkPos::hashCode(cx, cz);
	std::map<int, LevelChunk*>::iterator it = chunks.find(key);
	if (it != chunks.end()) return it->second;

	unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
	memset(blocks, 0, LevelChunk::ChunkBlockCount);
	for (int x = 0; x < 16; ++x) {
		for (int z = 0; z < 16; ++z) {
			const int column = x << 11 | z << 7;
			blocks[column] = (unsigned char) Tile::unbreakable->id;
			int top = sandTop(cx * 16 + x, cz * 16 + z);
			if (top < 2) top = 2;
			for (int y = 1; y <= top; ++y)
				blocks[column | y] = (unsigned char) Tile::sand->id;
			blocks[column | CeilingY] = (unsigned char) Tile::invisible_bedrock->id;
		}
	}
	LevelChunk* chunk = new LevelChunk(level, blocks, cx, cz);
	chunks.insert(std::make_pair(key, chunk));
	chunk->recalcHeightmap();
	return chunk;
}

void AquariumLevelSource::postProcess(ChunkSource* parent, int cx, int cz)
{
	// The chunk holding the middle builds the fixed portal, once.
	if (cx == (AquariumDimension::HubX >> 4) && cz == (AquariumDimension::HubZ >> 4))
		buildHub(level);
}

void AquariumLevelSource::buildHub(Level* level)
{
	const int cx = AquariumDimension::HubX, cz = AquariumDimension::HubZ, y = AquariumDimension::HubY;
	if (level->getTile(cx, y, cz) == Tile::aquariumPortal->id) return;

	// A 10x10 prismarine platform under the frame.
	for (int x = cx - 5; x < cx + 5; ++x)
		for (int z = cz - 5; z < cz + 5; ++z)
			level->setTileNoUpdate(x, y - 1, z, Tile::prismarine->id);
	// The frame (4x4 without its corners), of the kind nobody can break, and
	// the portal in its middle: the 2x2 at HubX-1..HubX, HubZ-1..HubZ.
	for (int dx = -2; dx <= 1; ++dx) {
		for (int dz = -2; dz <= 1; ++dz) {
			const bool corner = (dx == -2 || dx == 1) && (dz == -2 || dz == 1);
			const bool middle = (dx == -1 || dx == 0) && (dz == -1 || dz == 0);
			if (corner) continue;
			level->setTileNoUpdate(cx + dx, y, cz + dz, middle ? Tile::aquariumPortal->id : Tile::oceanCrystalHub->id);
		}
	}
}
