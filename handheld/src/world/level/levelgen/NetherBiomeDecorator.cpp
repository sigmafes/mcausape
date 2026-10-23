#include "NetherBiomeDecorator.h"
#include "feature/OreFeature.h"
#include "../biome/NetherBiomes.h"
#include "../Level.h"
#include "../tile/Tile.h"
#include "../../../util/Random.h"

namespace NetherBiomeDecorator
{

static bool air(Level* level, int x, int y, int z)
{
	if (y <= 0 || y >= 127) return false;
	const int t = level->getTile(x, y, z);
	return t == 0 || (Tile::tiles[t] && Tile::tiles[t]->material == Material::replaceable_plant);
}

static void put(Level* level, int x, int y, int z, int id)
{
	if (air(level, x, y, z)) level->setTileNoUpdate(x, y, z, id);
}

/// The first floor at or below y (air over something solid), or -1.
static int floorBelow(Level* level, int x, int y, int z, int span)
{
	for (int i = 0; i < span && y > 2; ++i, --y) {
		const int under = level->getTile(x, y - 1, z);
		if (level->getTile(x, y, z) == 0 && under > 0 && Tile::solid[under]) return y;
	}
	return -1;
}

/// The first ceiling at or above y (air under something solid), or -1.
static int ceilingAbove(Level* level, int x, int y, int z, int span)
{
	for (int i = 0; i < span && y < 125; ++i, ++y) {
		const int over = level->getTile(x, y + 1, z);
		if (level->getTile(x, y, z) == 0 && over > 0 && Tile::solid[over]) return y;
	}
	return -1;
}

/** Java's HugeFungusFeature, a little simplified: a stem 4-13 tall on nylium
    and a hat of wart block two blocks out, narrowing to one at the top,
    hollow but for some flecks, with shroomlights in it; the crimson one
    trails weeping vines from its rim. */
static void hugeFungus(Level* level, Random* r, int x, int y, int z, bool crimson)
{
	const int ground = level->getTile(x, y - 1, z);
	if (ground != (crimson ? Tile::crimsonNylium->id : Tile::warpedNylium->id)) return;
	const int height = 4 + r->nextInt(10);
	for (int j = 0; j <= height; ++j) if (!air(level, x, y + j, z)) return;
	const int stem = crimson ? Tile::crimsonStem->id : Tile::warpedStem->id;
	const int wart = crimson ? Tile::netherWartBlock->id : Tile::warpedWartBlock->id;
	for (int j = 0; j < height; ++j) level->setTileNoUpdate(x, y + j, z, stem);
	const int top = y + height;
	const int capH = 2 + height / 3;
	for (int yy = top - capH; yy <= top; ++yy) {
		const int rad = yy >= top - 1 ? 1 : 2;
		for (int dx = -rad; dx <= rad; ++dx)
			for (int dz = -rad; dz <= rad; ++dz) {
				const bool edge = dx == -rad || dx == rad || dz == -rad || dz == rad;
				if ((dx == -rad || dx == rad) && (dz == -rad || dz == rad) && rad > 1) continue;
				if (dx == 0 && dz == 0 && yy < top) continue;
				if (!edge && yy < top && r->nextInt(10) != 0) continue;
				put(level, x + dx, yy, z + dz, r->nextInt(12) == 0 ? Tile::shroomlight->id : wart);
				if (crimson && edge && yy == top - capH && r->nextInt(5) == 0) {
					const int len = 1 + r->nextInt(4);
					for (int k = 1; k <= len; ++k) put(level, x + dx, yy - k, z + dz, Tile::weepingVines->id);
				}
			}
	}
	put(level, x, top + 1, z, wart);
}

/// A patch of plants on the biome's ground: tries round a point.
static void plants(Level* level, Random* r, int cx, int cy, int cz, int tile, int ground, int tries)
{
	for (int i = 0; i < tries; ++i) {
		const int x = cx + r->nextInt(8) - r->nextInt(8);
		const int z = cz + r->nextInt(8) - r->nextInt(8);
		const int y = floorBelow(level, x, cy + 4, z, 8);
		if (y > 0 && level->getTile(x, y - 1, z) == ground) level->setTileNoUpdate(x, y, z, tile);
	}
}

/// Vines from ceilings (weeping) or floors (twisting), 1-8 long.
static void vines(Level* level, Random* r, int x, int y, int z, bool weeping)
{
	const int start = weeping ? ceilingAbove(level, x, y, z, 24) : floorBelow(level, x, y, z, 24);
	if (start < 0) return;
	const int len = 1 + r->nextInt(8);
	const int id = weeping ? Tile::weepingVines->id : Tile::twistingVines->id;
	for (int k = 0; k < len; ++k) {
		const int yy = weeping ? start - k : start + k;
		if (level->getTile(x, yy, z) != 0) break;
		level->setTileNoUpdate(x, yy, z, id);
	}
}

/// The deltas' basalt columns: a cluster of pillars 1-5 tall off the floor.
static void basaltColumns(Level* level, Random* r, int cx, int cy, int cz)
{
	const int n = 3 + r->nextInt(6);
	for (int i = 0; i < n; ++i) {
		const int x = cx + r->nextInt(5) - 2, z = cz + r->nextInt(5) - 2;
		const int y = floorBelow(level, x, cy + 4, z, 10);
		if (y < 0) continue;
		const int h = 1 + r->nextInt(5);
		for (int j = 0; j < h; ++j) put(level, x, y + j, z, Tile::basalt->id);
	}
}

/// A delta: a shallow pool of lava sunk into the floor, rimmed with magma.
static void delta(Level* level, Random* r, int cx, int cy, int cz)
{
	const int y = floorBelow(level, cx, cy, cz, 16);
	if (y < 0) return;
	const int rad = 2 + r->nextInt(3);
	for (int dx = -rad - 1; dx <= rad + 1; ++dx)
		for (int dz = -rad - 1; dz <= rad + 1; ++dz) {
			const int d2 = dx * dx + dz * dz;
			const int x = cx + dx, z = cz + dz;
			if (level->getTile(x, y, z) != 0) continue;
			const int under = level->getTile(x, y - 1, z);
			if (under != Tile::basalt->id && under != Tile::blackstone->id && under != Tile::netherrack->id) continue;
			if (d2 <= rad * rad) level->setTileNoUpdate(x, y - 1, z, Tile::calmLava->id);
			else if (d2 <= (rad + 1) * (rad + 1)) level->setTileNoUpdate(x, y - 1, z, Tile::magmaBlock->id);
		}
}

/// A small fossil of bone blocks, half sunk in the valley's soul sand.
static void fossil(Level* level, Random* r, int x, int y, int z)
{
	const int f = floorBelow(level, x, y, z, 20);
	if (f < 0) return;
	const bool alongX = r->nextInt(2) == 0;
	const int len = 4 + r->nextInt(4);
	for (int i = 0; i < len; ++i) {
		const int bx = alongX ? x + i : x, bz = alongX ? z : z + i;
		level->setTileNoUpdate(bx, f - 1, bz, Tile::boneBlock->id);
		if (i % 2 == 1)
			for (int s = -1; s <= 1; s += 2) {
				put(level, alongX ? bx : bx + s, f, alongX ? bz + s : bz, Tile::boneBlock->id);
				put(level, alongX ? bx : bx + 2 * s, f + 1, alongX ? bz + 2 * s : bz, Tile::boneBlock->id);
			}
	}
}

void decorate(Level* level, Random* r, int xo, int zo, long seed)
{
	const int cx = xo + 16, cz = zo + 16;
	const int biome = NetherBiomes::at(seed, cx, cz);

	if (biome == NetherBiomes::CrimsonForest || biome == NetherBiomes::WarpedForest) {
		const bool crimson = biome == NetherBiomes::CrimsonForest;
		const int nylium = crimson ? Tile::crimsonNylium->id : Tile::warpedNylium->id;
		for (int i = 0; i < 6; ++i) {
			const int x = xo + 8 + r->nextInt(16), z = zo + 8 + r->nextInt(16);
			const int y = floorBelow(level, x, 10 + r->nextInt(100), z, 30);
			if (y > 0) hugeFungus(level, r, x, y, z, crimson);
		}
		for (int i = 0; i < 4; ++i) {
			const int x = xo + 8 + r->nextInt(16), z = zo + 8 + r->nextInt(16);
			const int y = 10 + r->nextInt(100);
			plants(level, r, x, y, z, crimson ? Tile::crimsonRoots->id : Tile::warpedRoots->id, nylium, 24);
			plants(level, r, x, y, z, crimson ? Tile::crimsonFungus->id : Tile::warpedFungus->id, nylium, 8);
			if (!crimson) plants(level, r, x, y, z, Tile::netherSprouts->id, nylium, 16);
		}
		for (int i = 0; i < 10; ++i)
			vines(level, r, xo + 8 + r->nextInt(16), 10 + r->nextInt(110), zo + 8 + r->nextInt(16), crimson);
	} else if (biome == NetherBiomes::BasaltDeltas) {
		for (int i = 0; i < 3; ++i)
			basaltColumns(level, r, xo + 8 + r->nextInt(16), 10 + r->nextInt(100), zo + 8 + r->nextInt(16));
		for (int i = 0; i < 2; ++i)
			delta(level, r, xo + 8 + r->nextInt(16), 20 + r->nextInt(90), zo + 8 + r->nextInt(16));
		for (int i = 0; i < 4; ++i) {
			OreFeature blobs(Tile::blackstone->id, 20, Tile::basalt->id);
			blobs.place(level, r, xo + r->nextInt(16), 10 + r->nextInt(100), zo + r->nextInt(16));
		}
		for (int i = 0; i < 4; ++i) {
			OreFeature magma(Tile::magmaBlock->id, 10, Tile::netherrack->id);
			magma.place(level, r, xo + r->nextInt(16), 26 + r->nextInt(12), zo + r->nextInt(16));
		}
	} else if (biome == NetherBiomes::SoulSandValley) {
		for (int i = 0; i < 5; ++i) {
			const int x = xo + 8 + r->nextInt(16), z = zo + 8 + r->nextInt(16);
			const int y = floorBelow(level, x, 10 + r->nextInt(100), z, 30);
			if (y > 0) {
				const int under = level->getTile(x, y - 1, z);
				if (under == Tile::hellSand->id || under == Tile::soulSoil->id) level->setTileNoUpdate(x, y, z, Tile::soulFire->id);
			}
		}
		if (r->nextInt(6) == 0) fossil(level, r, xo + 8 + r->nextInt(12), 20 + r->nextInt(80), zo + 8 + r->nextInt(12));
		if (r->nextInt(3) == 0) {
			// A basalt pillar from floor to ceiling, as Java's valleys have.
			const int x = xo + 8 + r->nextInt(16), z = zo + 8 + r->nextInt(16);
			const int y = floorBelow(level, x, 10 + r->nextInt(100), z, 30);
			if (y > 0) for (int j = 0; j < 40 && level->getTile(x, y + j, z) == 0; ++j) level->setTileNoUpdate(x, y + j, z, Tile::basalt->id);
		}
	}

	// Every biome: nether gold ore, and ancient debris -- one big vein low
	// down (8-24) and one small one anywhere, as Java's.
	for (int i = 0; i < 10; ++i) {
		OreFeature gold(Tile::netherGoldOre->id, 10, Tile::netherrack->id);
		gold.place(level, r, xo + r->nextInt(16), 10 + r->nextInt(108), zo + r->nextInt(16));
	}
	{
		OreFeature big(Tile::ancientDebris->id, 3, Tile::netherrack->id);
		big.place(level, r, xo + r->nextInt(16), 8 + r->nextInt(16), zo + r->nextInt(16));
		OreFeature small(Tile::ancientDebris->id, 2, Tile::netherrack->id);
		small.place(level, r, xo + r->nextInt(16), 8 + r->nextInt(112), zo + r->nextInt(16));
	}
}

}
