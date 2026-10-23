#include "AquariumLevelSource.h"
#include "../Level.h"
#include "../ChunkPos.h"
#include "../chunk/LevelChunk.h"
#include "../tile/Tile.h"
#include "../LightLayer.h"
#include "../dimension/AquariumDimension.h"
#include "../../../util/Mth.h"
#include "../../../util/Random.h"
#include "synth/PerlinNoise.h"
#include <math.h>
#include <string.h>

AquariumLevelSource::AquariumLevelSource(Level* level, long seed)
:	level(level),
	seed(seed)
{
	random = new Random(seed);
	topNoise = new PerlinNoise(random, 4);
	bottomNoise = new PerlinNoise(random, 4);
	dripNoise = new PerlinNoise(random, 3);
	sandNoise = new PerlinNoise(random, 3);
	reefNoise = new PerlinNoise(random, 3);
}

AquariumLevelSource::~AquariumLevelSource()
{
	delete topNoise;
	delete bottomNoise;
	delete dripNoise;
	delete sandNoise;
	delete reefNoise;
	delete random;
}

static float unit(float v, int levels)
{
	// PerlinNoise::getValue halves the frequency and doubles the amplitude
	// each octave: n of them add up to about 2^n - 1 either way. Scaled back
	// to -1..1 (a little stretched: the extremes are rare).
	v = v / (float) ((1 << levels) - 1) * 1.7f;
	return v < -1 ? -1 : (v > 1 ? 1 : v);
}

void AquariumLevelSource::shelfAt(int x, int z, int* bottom, int* top) const
{
	// Top: hills between ~84 and ~104, like the Overworld's ground (octaves
	// from 8 to 64 blocks across).
	const float t = unit(topNoise->getValue(x / 8.0f, z / 8.0f), 4);
	*top = 94 + (int) (t * 10);
	// Underside: a slow swell between ~38 and ~62 (6 to 48 blocks)...
	const float b = unit(bottomNoise->getValue(x / 6.0f, z / 6.0f), 4);
	int bot = 50 + (int) (b * 12);
	// ...with drips hanging off it (2 to 10 blocks across), up to ~25 long.
	const float d = unit(dripNoise->getValue(x / 2.5f, z / 2.5f), 3);
	if (d > 0.1f) bot -= (int) ((d - 0.1f) * 28);
	// Never down to the sand: the cave under it stays open.
	if (bot < 18) bot = 18;
	*bottom = bot;
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
			int shelfBottom, shelfTop;
			shelfAt(cx * 16 + x, cz * 16 + z, &shelfBottom, &shelfTop);
			for (int y = shelfBottom; y <= shelfTop; ++y)
				blocks[column | y] = (unsigned char) Tile::prismarine->id;
			// Big patches of sand over the prismarine, two or three deep.
			if (shelfTop >= shelfBottom) {
				const float s = unit(sandNoise->getValue((cx * 16 + x) / 4.0f, (cz * 16 + z) / 4.0f), 3);
				if (s > 0.25f) {
					const int depth = s > 0.55f ? 3 : 2;
					for (int y = shelfTop; y > shelfTop - depth && y >= shelfBottom; --y)
						blocks[column | y] = (unsigned char) Tile::sand->id;
				}
			}
			blocks[column | CeilingY] = (unsigned char) Tile::invisible_bedrock->id;
		}
	}
	// Sea lanterns in the prismarine, like an ore: small clumps, plenty of
	// them, some coming out in the open. Inside this chunk only, so nothing
	// here reaches a neighbour.
	{
		Random r(seed ^ ((long) cx * 987234871L) ^ ((long) cz * 214537L) ^ 0x5ea1L);
		const int veins = 5 + r.nextInt(5);
		const int prismarine = Tile::prismarine->id, lantern = Tile::seaLantern->id;
		for (int v = 0; v < veins; ++v) {
			int x = r.nextInt(16), z = r.nextInt(16), y = 18 + r.nextInt(90);
			const int size = 2 + r.nextInt(4);
			for (int n = 0; n < size; ++n) {
				if (x >= 0 && x < 16 && z >= 0 && z < 16 && y > 0 && y < CeilingY) {
					unsigned char& b = blocks[x << 11 | z << 7 | y];
					if (b == prismarine) b = (unsigned char) lantern;
				}
				switch (r.nextInt(6)) {
					case 0: ++x; break; case 1: --x; break;
					case 2: ++z; break; case 3: --z; break;
					case 4: ++y; break; default: --y; break;
				}
			}
		}
	}
	LevelChunk* chunk = new LevelChunk(level, blocks, cx, cz);
	chunks.insert(std::make_pair(key, chunk));
	chunk->recalcHeightmap();
	return chunk;
}

void AquariumLevelSource::postProcess(ChunkSource* parent, int cx, int cz)
{
	decorate(cx, cz);
	// The sea lanterns getChunk set into raw blocks give no light until it is
	// worked out: the ones out in the open, which have somewhere to shine.
	const int lantern = Tile::seaLantern->id;
	for (int x = 0; x < 16; ++x)
		for (int z = 0; z < 16; ++z)
			for (int y = 18; y < 110; ++y) {
				const int wx = cx * 16 + x, wz = cz * 16 + z;
				if (level->getTile(wx, y, wz) != lantern) continue;
				if (level->getTile(wx, y + 1, wz) && level->getTile(wx, y - 1, wz)
						&& (x == 0 || level->getTile(wx - 1, y, wz)) && (x == 15 || level->getTile(wx + 1, y, wz))
						&& (z == 0 || level->getTile(wx, y, wz - 1)) && (z == 15 || level->getTile(wx, y, wz + 1)))
					continue;
				level->updateLight(LightLayer::Block, wx, y, wz, wx, y, wz);
			}
	// The chunk holding the middle builds the fixed portal, once.
	AquariumDimension::loadHub(level);
	if (!AquariumDimension::hubMoved && cx == (AquariumDimension::HubX >> 4) && cz == (AquariumDimension::HubZ >> 4))
		buildHub(level);
}

// The 8 cells of the frame round the 2x2 portal (HubX-1..HubX, HubZ-1..HubZ).
static const int FrameCells[8][2] = {
	{-1, -2}, {0, -2}, {-1, 1}, {0, 1}, {-2, -1}, {-2, 0}, {1, -1}, {1, 0}
};

void AquariumLevelSource::hubArrival(int pick, int* x, int* y, int* z)
{
	// On top of a frame block, not in the portal: standing in it would send
	// the player straight back.
	const int i = ((pick % 8) + 8) % 8;
	*x = AquariumDimension::HubX + FrameCells[i][0];
	*y = AquariumDimension::HubY + 1;
	*z = AquariumDimension::HubZ + FrameCells[i][1];
}

void AquariumLevelSource::buildHub(Level* level)
{
	AquariumDimension::loadHub(level);
	const int cx = AquariumDimension::HubX, cz = AquariumDimension::HubZ, y = AquariumDimension::HubY;
	if (level->getTile(cx, y, cz) == Tile::aquariumPortal->id) {
		// Built before the hub lost its platform: take the prismarine away, once.
		if (level->getTile(cx - 5, y - 1, cz - 5) == Tile::prismarine->id) {
			for (int x = cx - 5; x < cx + 5; ++x)
				for (int z = cz - 5; z < cz + 5; ++z)
					if (level->getTile(x, y - 1, z) == Tile::prismarine->id)
						level->setTile(x, y - 1, z, 0);
		}
		return;
	}

	// The frame, of the kind nobody can break, and the portal in its middle,
	// floating in the water.
	for (int i = 0; i < 8; ++i)
		level->setTileNoUpdate(cx + FrameCells[i][0], y, cz + FrameCells[i][1], Tile::oceanCrystalHub->id);
	for (int dx = -1; dx <= 0; ++dx)
		for (int dz = -1; dz <= 0; ++dz)
			level->setTileNoUpdate(cx + dx, y, cz + dz, Tile::aquariumPortal->id);
}

/** Coral, after Java's warm ocean: reefs of three shapes -- mushroom (a
    lumpy blob), tree (a stalk that splits into branches) and claw (arms
    reaching out and up) -- each of one coral, on the top of the shelf.
    Like any decoration they start in the middle of the chunk (+8) so they
    can spill into neighbours that already exist. Thick where reefNoise is
    high, a few elsewhere. */
void AquariumLevelSource::decorate(int cx, int cz)
{
	Random r(seed ^ ((long) cx * 341873128712L) ^ ((long) cz * 132897987541L));
	const int x0 = cx * 16 + 8, z0 = cz * 16 + 8;
	const float reef = unit(reefNoise->getValue(x0 / 6.0f, z0 / 6.0f), 3);
	const int count = reef > 0.2f ? 10 + r.nextInt(8) : r.nextInt(3);
	_placed.clear();
	for (int i = 0; i < count; ++i) {
		const int x = x0 + r.nextInt(16), z = z0 + r.nextInt(16);
		// The shelf's top: the first block from the top down, if it is shelf.
		int y = 110;
		while (y > 60 && level->getTile(x, y, z) == 0) --y;
		const int ground = level->getTile(x, y, z);
		if (y <= 60 || (ground != Tile::prismarine->id && ground != Tile::sand->id)) continue;
		const int tile = Tile::coral[r.nextInt(5)]->id;
		switch (r.nextInt(3)) {
			case 0: coralMushroom(r, x, y + 1, z, tile); break;
			case 1: coralTree(r, x, y + 1, z, tile); break;
			default: coralClaw(r, x, y + 1, z, tile); break;
		}
	}
	kelp(r, x0, z0);
	if (r.nextInt(8) == 0) fossil(r, x0, z0);
	// Coral plants of the block's own colour on the reefs' open tops, as
	// often as not. Only on blocks just placed: their chunks exist.
	// A copy: a neighbour generated by a getTile below decorates too, and
	// that clears and refills _placed under this loop.
	const std::vector<TilePos> placed = _placed;
	for (unsigned int i = 0; i < placed.size(); ++i) {
		const int x = placed[i].x, y = placed[i].y + 1, z = placed[i].z;
		const int below = level->getTile(x, y - 1, z);
		if (y >= CeilingY || level->getTile(x, y, z) != 0 || r.nextInt(2)) continue;
		for (int c = 0; c < 5; ++c)
			if (below == Tile::coral[c]->id) level->setTileNoUpdate(x, y, z, Tile::coralPlant[c]->id);
	}
}

void AquariumLevelSource::placeCoral(int x, int y, int z, int tile)
{
	if (y > 0 && y < CeilingY && level->getTile(x, y, z) == 0)
	{
		level->setTileNoUpdate(x, y, z, tile);
		_placed.push_back(TilePos(x, y, z));
	}
}

void AquariumLevelSource::coralMushroom(Random& r, int x, int y, int z, int tile)
{
	// Java's CoralMushroomFeature: a box 3-5 a side, its edges trimmed,
	// a few blocks missing, sitting a block or so up on a short stem.
	const int w = r.nextInt(3) + 3, h = r.nextInt(3) + 3, d = r.nextInt(3) + 3;
	const int lift = r.nextInt(2);
	placeCoral(x, y, z, tile);
	for (int i = 0; i < w; ++i)
		for (int j = 0; j < h; ++j)
			for (int k = 0; k < d; ++k) {
				const int edges = (i == 0 || i == w - 1) + (j == 0 || j == h - 1) + (k == 0 || k == d - 1);
				if (edges >= 2 || r.nextInt(6) == 0) continue;
				placeCoral(x + i - w / 2, y + lift + j, z + k - d / 2, tile);
			}
}

void AquariumLevelSource::coralTree(Random& r, int x, int y, int z, int tile)
{
	// Java's CoralTreeFeature: a stalk 1-3 high, then 2-4 branches that step
	// out one block and climb, now and then stepping aside again.
	const int stalk = r.nextInt(3) + 1;
	for (int j = 0; j < stalk; ++j) placeCoral(x, y + j, z, tile);
	const int top = y + stalk;
	static const int dirs[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	const int branches = r.nextInt(3) + 2;
	const int first = r.nextInt(4);
	for (int b = 0; b < branches; ++b) {
		const int* dir = dirs[(first + b) % 4];
		int bx = x + dir[0], bz = z + dir[1], by = top;
		const int len = r.nextInt(5) + 2;
		for (int j = 0; j < len; ++j) {
			placeCoral(bx, by, bz, tile);
			++by;
			if (r.nextInt(4) == 0) { bx += dir[0]; bz += dir[1]; }
		}
	}
}

void AquariumLevelSource::coralClaw(Random& r, int x, int y, int z, int tile)
{
	// Java's CoralClawFeature: a base block, and 2-3 arms reaching out
	// diagonally for a while, then straight up.
	placeCoral(x, y, z, tile);
	static const int dirs[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	const int arms = r.nextInt(2) + 2;
	const int first = r.nextInt(4);
	for (int a = 0; a < arms; ++a) {
		const int* dir = dirs[(first + a) % 4];
		int ax = x, ay = y, az = z;
		const int out = r.nextInt(2) + 1, up = r.nextInt(3) + 2;
		for (int j = 0; j < out; ++j) {
			ax += dir[0]; az += dir[1]; ++ay;
			placeCoral(ax, ay, az, tile);
			placeCoral(ax, ay - 1, az, tile);
		}
		for (int j = 0; j < up; ++j) {
			++ay;
			placeCoral(ax, ay, az, tile);
		}
	}
}

/** Kelp, after Java's KelpFeature: columns up from the ground, the last block
    the tip. In beds -- many to a chunk where reefNoise is low, so reefs and
    kelp forests take turns -- on the shelf's top and on
    the sand of the cave under it. */
void AquariumLevelSource::kelp(Random& r, int x0, int z0)
{
	const float reef = unit(reefNoise->getValue(x0 / 6.0f, z0 / 6.0f), 3);
	const int count = reef < -0.1f ? 12 + r.nextInt(8) : 2 + r.nextInt(3);
	const int id = Tile::kelp->id;
	for (int i = 0; i < count; ++i) {
		const int x = x0 + r.nextInt(16), z = z0 + r.nextInt(16);
		// Half on the shelf, half on the sea floor below it.
		int y;
		if (r.nextInt(2) == 0) {
			y = 110;
			while (y > 60 && level->getTile(x, y, z) == 0) --y;
		} else {
			y = 1;
			while (y < 40 && level->getTile(x, y + 1, z) != 0) ++y;
		}
		const int ground = level->getTile(x, y, z);
		if (ground != Tile::prismarine->id && ground != Tile::sand->id) continue;
		if (level->getTile(x, y + 1, z) != 0) continue;
		// Java: up to ~25 tall, most a good deal less.
		const int height = 1 + r.nextInt(r.nextInt(12) + 3);
		for (int j = 1; j <= height; ++j) {
			if (y + j >= CeilingY - 1 || level->getTile(x, y + j, z) != 0) break;
			level->setTileNoUpdate(x, y + j, z, id);
		}
	}
}

/** A fossil on the sea floor, after Java's: a spine of bone blocks half sunk
    in the sand, a rib arching over it every other block, a skull at one end,
    a few blocks worn away. Small enough to stay within the next chunk. */
void AquariumLevelSource::fossil(Random& r, int x0, int z0)
{
	const int bone = Tile::boneBlock->id, sand = Tile::sand->id;
	const bool alongX = r.nextInt(2) == 0;
	const int len = 5 + r.nextInt(4);
	const int sx = x0 + 3 + r.nextInt(4), sz = z0 + 3 + r.nextInt(4);
	// The sand's top under the spine's start.
	int y = 1;
	while (y < 40 && level->getTile(sx, y + 1, sz) != 0) ++y;
	if (level->getTile(sx, y, sz) != sand || y >= 40) return;

	struct Put {
		static void at(Level* level, Random& r, bool alongX, int sx, int sz, int i, int side, int y, int id) {
			if (r.nextInt(7) == 0) return;
			const int x = alongX ? sx + i : sx + side, z = alongX ? sz + side : sz + i;
			const int t = level->getTile(x, y, z);
			if (t == 0 || t == Tile::sand->id) level->setTileNoUpdate(x, y, z, id);
		}
	};
	for (int i = 0; i < len; ++i) {
		Put::at(level, r, alongX, sx, sz, i, 0, y, bone);
		if (i % 2 == 1 && i < len - 1) {
			for (int s = -1; s <= 1; s += 2) {
				Put::at(level, r, alongX, sx, sz, i, s, y + 1, bone);
				Put::at(level, r, alongX, sx, sz, i, 2 * s, y + 2, bone);
				Put::at(level, r, alongX, sx, sz, i, 2 * s, y + 3, bone);
				Put::at(level, r, alongX, sx, sz, i, s, y + 4, bone);
			}
		}
	}
	// The skull: a 2x2x2 at the end, a block up.
	for (int i = len; i < len + 2; ++i)
		for (int s = 0; s <= 1; ++s)
			for (int h = 1; h <= 2; ++h)
				Put::at(level, r, alongX, sx, sz, i, s, y + h - 1, bone);
}
