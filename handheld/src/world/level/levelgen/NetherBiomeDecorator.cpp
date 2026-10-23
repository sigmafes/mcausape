#include "NetherBiomeDecorator.h"
#include "feature/OreFeature.h"
#include "../biome/NetherBiomes.h"
#include "../Level.h"
#include "../tile/Tile.h"
#include "../../../util/Random.h"
#include "../tile/entity/ChestTileEntity.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../../entity/monster/Piglin.h"

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

/** Java 1.16's HugeFungusFeature: a stem 4-13 tall (one in twelve twice
    that; a few thick, 3x3 less corners), and a hat over its top 5 + a third
    of its height: wart blocks two out (three at the base of tall hats, one
    more round a thick stem), one out for the top layer or two, hollow inside
    but for flecks, shroomlights studded under the shell. The crimson one
    trails weeping vines from the underside of its hat's rim. */
static void hugeFungus(Level* level, Random* r, int x, int y, int z, bool crimson)
{
	const int ground = level->getTile(x, y - 1, z);
	if (ground != (crimson ? Tile::crimsonNylium->id : Tile::warpedNylium->id)) return;
	int height = 4 + r->nextInt(10);
	if (r->nextInt(12) == 0) height *= 2;
	const bool thick = r->nextInt(16) == 0;
	// Room for the stem, or a shorter one; give up under a low ceiling.
	int room = 0;
	while (room < height + 2 && air(level, x, y + room, z)) ++room;
	if (room < height + 2) height = room - 2;
	if (height < 4) return;
	const int stem = crimson ? Tile::crimsonStem->id : Tile::warpedStem->id;
	const int wart = crimson ? Tile::netherWartBlock->id : Tile::warpedWartBlock->id;
	for (int j = 0; j < height; ++j)
		for (int dx = -thick; dx <= (int)thick; ++dx)
			for (int dz = -thick; dz <= (int)thick; ++dz)
				if (!(dx && dz)) {
					if (dx || dz) { if (air(level, x + dx, y + j, z + dz)) level->setTileNoUpdate(x + dx, y + j, z + dz, stem); }
					else level->setTileNoUpdate(x, y + j, z, stem);
				}
	int hatH = r->nextInt(1 + height / 3) + 5;
	if (hatH > height) hatH = height;
	const int top = y + height, start = top - hatH;
	for (int yy = start; yy <= top; ++yy) {
		int rad = yy < top - r->nextInt(3) ? 2 : 1;
		if (hatH > 8 && yy < start + 4) rad = 3;
		if (thick) ++rad;
		for (int dx = -rad; dx <= rad; ++dx)
			for (int dz = -rad; dz <= rad; ++dz) {
				const bool ex = dx == -rad || dx == rad, ez = dz == -rad || dz == rad;
				const bool corner = ex && ez, edge = ex || ez;
				const int bx = x + dx, bz = z + dz;
				if (yy == top) {	// the crown: solid but for the corners
					if (!corner || r->nextInt(3) == 0) put(level, bx, yy, bz, wart);
					continue;
				}
				if (corner && r->nextInt(10) != 0) continue;
				if (edge) {
					put(level, bx, yy, bz, wart);
					// Vines from the rim's lowest layers.
					if (crimson && yy <= start + 1 && r->nextInt(6) == 0) {
						const int len = 1 + r->nextInt(5);
						for (int k = 1; k <= len && air(level, bx, yy - k, bz); ++k)
							level->setTileNoUpdate(bx, yy - k, bz, Tile::weepingVines->id);
					}
				} else if (r->nextInt(10) == 0) {
					put(level, bx, yy, bz, r->nextInt(3) == 0 ? Tile::shroomlight->id : wart);
				} else if (r->nextInt(14) == 0 && air(level, bx, yy, bz)) {
					// Shroomlights hung just inside the shell.
					level->setTileNoUpdate(bx, yy, bz, Tile::shroomlight->id);
				}
			}
	}
}

/** The two forests, floor by floor and block by block across the chunk, as
    Java's features add up to: huge fungi everywhere on nylium (about one
    floor block in twenty, packed close enough that their hats touch), the
    floor between carpeted with roots - and in the warped forest sprouts and
    tall twisting vines - with the odd small fungus; and from the ceilings,
    in the crimson forest, clumps of wart block trailing long weeping vines. */
static void forestField(Level* level, Random* r, int xo, int zo, bool crimson)
{
	const int nylium = crimson ? Tile::crimsonNylium->id : Tile::warpedNylium->id;
	const int roots = crimson ? Tile::crimsonRoots->id : Tile::warpedRoots->id;
	for (int dx = 0; dx < 16; ++dx)
		for (int dz = 0; dz < 16; ++dz) {
			const int x = xo + 8 + dx, z = zo + 8 + dz;
			for (int y = 122; y > 6; --y) {
				if (level->getTile(x, y, z) != 0) continue;
				const int under = level->getTile(x, y - 1, z), over = level->getTile(x, y + 1, z);
				if (under == nylium) {
					const int n = r->nextInt(100);
					if (n < 5) hugeFungus(level, r, x, y, z, crimson);
					else if (n < 6) level->setTileNoUpdate(x, y, z, crimson ? Tile::crimsonFungus->id : Tile::warpedFungus->id);
					else if (!crimson && n < 9) {
						const int len = 1 + r->nextInt(r->nextInt(3) == 0 ? 12 : 5);
						for (int k = 0; k < len && level->getTile(x, y + k, z) == 0; ++k)
							level->setTileNoUpdate(x, y + k, z, Tile::twistingVines->id);
					}
					else if (n < 30) level->setTileNoUpdate(x, y, z, roots);
					else if (!crimson && n < 45) level->setTileNoUpdate(x, y, z, Tile::netherSprouts->id);
				} else if (crimson && over == Tile::netherrack->id && r->nextInt(40) == 0) {
					// A clump of wart block let into the ceiling, vines below it.
					for (int ax = -1; ax <= 1; ++ax)
						for (int az = -1; az <= 1; ++az) {
							if (ax && az && r->nextInt(2)) continue;
							if (level->getTile(x + ax, y + 1, z + az) == Tile::netherrack->id)
								level->setTileNoUpdate(x + ax, y + 1, z + az, Tile::netherWartBlock->id);
							if (r->nextInt(2) == 0) {
								const int len = 1 + r->nextInt(9);
								for (int k = 0; k < len && level->getTile(x + ax, y - k, z + az) == 0 && y - k > 6; ++k)
									level->setTileNoUpdate(x + ax, y - k, z + az, Tile::weepingVines->id);
							}
						}
				}
			}
		}
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

static bool solidAt(Level* level, int x, int y, int z)
{
	const int t = level->getTile(x, y, z);
	return t > 0 && Tile::solid[t];
}

/** Java's BasaltColumnsFeature: a cluster of columns, tallest in the middle
    and falling off with distance, standing on the floor up to the ceiling or
    their height; where there is no floor (a lava lake) they hang from the
    ceiling instead. Small clusters: reach 1, 1-4 tall; large: reach 2-3,
    5-10 tall. This, more than anything, is what the deltas look like. */
static void basaltColumns(Level* level, Random* r, int cx, int cy, int cz, bool large)
{
	const int reach = large ? 2 + r->nextInt(2) : 1;
	const int height = large ? 5 + r->nextInt(6) : 1 + r->nextInt(4);
	int fy = floorBelow(level, cx, cy, cz, 40);
	bool hanging = false;
	if (fy < 0 || level->getMaterial(cx, fy - 1, cz) == Material::lava) {
		fy = ceilingAbove(level, cx, cy, cz, 40);
		if (fy < 0) return;
		hanging = true;
	}
	for (int dx = -reach; dx <= reach; ++dx)
		for (int dz = -reach; dz <= reach; ++dz) {
			const int d = (dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz);
			if (d > reach + 1) continue;
			const int h = height - d - r->nextInt(2) - (d > 0 ? r->nextInt(3) : 0);
			if (h <= 0) continue;
			const int x = cx + dx, z = cz + dz;
			// Each column settles on its own floor, a few blocks either way.
			const int y = hanging ? ceilingAbove(level, x, fy - 4, z, 8) : floorBelow(level, x, fy + 4, z, 8);
			if (y < 0) continue;
			if (!hanging && level->getMaterial(x, y - 1, z) == Material::lava) continue;
			for (int j = 0; j < h; ++j) {
				const int yy = hanging ? y - j : y + j;
				if (!air(level, x, yy, z)) break;
				level->setTileNoUpdate(x, yy, z, Tile::basalt->id);
			}
		}
}

/** Java's DeltaFeature: a flat, irregular lake of lava 3-7 across each way,
    let into the floor and rimmed with magma. A block becomes lava only when
    it is walled in on every side and below and open above, so a delta never
    spills over an edge - that is how the floors fill with dozens of small
    lakes without flooding the caves under them. */
static void delta(Level* level, Random* r, int cx, int cy, int cz)
{
	const int y0 = floorBelow(level, cx, cy, cz, 30);
	if (y0 < 0) return;
	const int y = y0 - 1;	// the floor block itself
	const int rx = 3 + r->nextInt(7), rz = 3 + r->nextInt(7), rim = r->nextInt(3);
	const int rr = (rx > rz ? rx : rz) + rim;
	for (int dx = -rr; dx <= rr; ++dx)
		for (int dz = -rr; dz <= rr; ++dz) {
			const float fx = dx / (float)rx, fz = dz / (float)rz;
			const float d = fx * fx + fz * fz + r->nextFloat() * 0.3f;
			const float fxr = dx / (float)(rx + rim), fzr = dz / (float)(rz + rim);
			const bool lake = d <= 1.0f, ring = rim > 0 && fxr * fxr + fzr * fzr <= 1.0f;
			if (!lake && !ring) continue;
			const int x = cx + dx, z = cz + dz;
			if (!solidAt(level, x, y, z) || level->getTile(x, y + 1, z) != 0) continue;
			if (!solidAt(level, x, y - 1, z)) continue;
			if (lake) {
				bool walled = true;
				for (int s = 0; s < 4 && walled; ++s) {
					const int nx = x + (s == 0) - (s == 1), nz = z + (s == 2) - (s == 3);
					const int t = level->getTile(nx, y, nz);
					walled = (t > 0 && Tile::solid[t]) || t == Tile::calmLava->id || t == Tile::lava->id;
				}
				if (walled) { level->setTileNoUpdate(x, y, z, Tile::calmLava->id); continue; }
			}
			level->setTileNoUpdate(x, y, z, Tile::magmaBlock->id);
		}
}

/// Smooth 0..1 value noise on a grid of `cell` blocks, from the seed.
static float valueNoise(long seed, int x, int z, int cell, int salt)
{
	const int gx = (x >= 0 ? x : x - cell + 1) / cell, gz = (z >= 0 ? z : z - cell + 1) / cell;
	const float fx = (x - gx * cell) / (float)cell, fz = (z - gz * cell) / (float)cell;
	float c[4];
	for (int i = 0; i < 4; ++i) {
		unsigned int h = (unsigned int)((gx + (i & 1)) * 73856093) ^ (unsigned int)((gz + (i >> 1)) * 19349663)
			^ (unsigned int)(seed * 83492791) ^ (unsigned int)(salt * 2654435761u);
		h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
		c[i] = (h & 0xffff) / 65535.0f;
	}
	const float sx = fx * fx * (3 - 2 * fx), sz = fz * fz * (3 - 2 * fz);
	return (c[0] * (1 - sx) + c[1] * sx) * (1 - sz) + (c[2] * (1 - sx) + c[3] * sx) * sz;
}

/** The deltas' forest of columns - the biome's look in Java, where its
    column features pile up until nearly every floor bristles with basalt
    pillars a block wide, ragged in height block to block, rising in tall
    clusters and dropping into gaps, with the flat lava lakes left open
    between them. Here: every floor of the chunk, one column per block, its
    height from two noises (clusters, and wide swells) plus a per-block
    jitter; a few hang from the ceilings too. Columns stop at what they meet. */
static void columnField(Level* level, Random* r, int xo, int zo, long seed)
{
	const int ba = Tile::basalt->id;
	for (int dx = 0; dx < 16; ++dx)
		for (int dz = 0; dz < 16; ++dz) {
			const int x = xo + 8 + dx, z = zo + 8 + dz;
			const float cluster = valueNoise(seed, x, z, 4, 1), swell = valueNoise(seed, x, z, 14, 2);
			for (int y = 122; y > 6; --y) {
				const int here = level->getTile(x, y, z);
				if (here != 0) continue;
				const int under = level->getTile(x, y - 1, z), over = level->getTile(x, y + 1, z);
				if (under > 0 && Tile::solid[under] && under != Tile::magmaBlock->id) {
					int h = (int)(cluster * 9 + swell * 10) - 5 + r->nextInt(4) - (r->nextInt(6) == 0 ? 4 : 0);
					for (int j = 0; j < h && level->getTile(x, y + j, z) == 0 && y + j < 126; ++j)
						level->setTileNoUpdate(x, y + j, z, ba);
				} else if (over > 0 && Tile::solid[over] && r->nextInt(3) == 0) {
					const int h = (int)(cluster * 6 + swell * 4) - 3 + r->nextInt(3);
					for (int j = 0; j < h && level->getTile(x, y - j, z) == 0 && y - j > 6; ++j)
						level->setTileNoUpdate(x, y - j, z, ba);
				}
			}
		}
}

/// Java's spring_delta: lava trickling out of the walls.
static void lavaSpring(Level* level, Random* r, int x, int y, int z)
{
	if (level->getTile(x, y, z) != Tile::basalt->id && level->getTile(x, y, z) != Tile::netherrack->id
		&& level->getTile(x, y, z) != Tile::blackstone->id) return;
	if (!solidAt(level, x, y + 1, z) || !solidAt(level, x, y - 1, z)) return;
	int open = 0;
	if (level->getTile(x + 1, y, z) == 0) ++open;
	if (level->getTile(x - 1, y, z) == 0) ++open;
	if (level->getTile(x, y, z + 1) == 0) ++open;
	if (level->getTile(x, y, z - 1) == 0) ++open;
	if (open != 1) return;
	// As HellSpringFeature: let it run its course now, during generation.
	level->setTile(x, y, z, Tile::lava->id);
	level->instaTick = true;
	Tile::lava->tick(level, x, y, z, r);
	level->instaTick = false;
}

/** The soul sand valley's floors, block by block: soul fire in clusters
    (where a coarse noise runs high, about one block in four there burns),
    as the blue fields of flame in Java's valleys. */
static void soulValleyField(Level* level, Random* r, int xo, int zo, long seed)
{
	for (int dx = 0; dx < 16; ++dx)
		for (int dz = 0; dz < 16; ++dz) {
			const int x = xo + 8 + dx, z = zo + 8 + dz;
			const float n = valueNoise(seed, x, z, 5, 7);
			if (n < 0.72f) continue;
			for (int y = 122; y > 6; --y) {
				if (level->getTile(x, y, z) != 0) continue;
				const int under = level->getTile(x, y - 1, z);
				if ((under == Tile::hellSand->id || under == Tile::soulSoil->id) && r->nextInt(4) == 0)
					level->setTileNoUpdate(x, y, z, Tile::soulFire->id);
			}
		}
}

/** Java's nether fossil, as the valley shows them: a great arch of bone
    block - two legs out of the soul sand, a span across, ribs hanging off
    it - something's spine left behind. */
static void fossilArch(Level* level, Random* r, int x, int y, int z)
{
	const int f = floorBelow(level, x, y, z, 24);
	if (f < 0) return;
	const bool alongX = r->nextInt(2) == 0;
	const int span = 3 + r->nextInt(4), legH = 3 + r->nextInt(3), bone = Tile::boneBlock->id;
	#define AT(i, yy, s) (alongX ? x + (i) : x + (s)), (yy), (alongX ? z + (s) : z + (i))
	for (int j = -1; j < legH; ++j) {	// the legs, sunk one into the floor
		level->setTileNoUpdate(AT(0, f + j, 0), bone);
		level->setTileNoUpdate(AT(span, f + j, 0), bone);
	}
	for (int i = 0; i <= span; ++i) {
		put(level, AT(i, f + legH, 0), bone);	// the span
		if (i > 0 && i < span && i % 2 == 0)	// ribs down either side
			for (int sd = -1; sd <= 1; sd += 2) {
				put(level, AT(i, f + legH, sd), bone);
				for (int k = 1; k <= 1 + r->nextInt(2); ++k) put(level, AT(i, f + legH - k, 2 * sd), bone);
			}
	}
	#undef AT
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


/** Bastion remnants, after Java 1.16's: a great keep of polished blackstone
    bricks 40 blocks square and three storeys tall, open in the middle. Its
    walls are cracked and broken off at uneven heights; inside, galleries run
    round an atrium at 7 and 14 up, reached by stairs along the walls; every
    floor is holed (two-block gaps over a long drop, or over lava on the
    ground floor), the upper ones with lava basins let into them; and in the
    middle a treasure dais of gold sits behind a lava moat crossed by four
    bridges a block wide. Brutes guard the dais, piglins - some with bows -
    the galleries.

    A bastion is far bigger than one chunk's window, so it is built the way
    a structure has to be here: every block is a pure function of where it is
    and of the bastion's seed (blockAt), and each chunk, when decorated,
    builds just the part that falls in its own window. Chests and mobs go in
    with the chunk that holds their spot. Order does not matter. */
static const int BASTION_REGION = 6 * 16;	// one chance per 96x96 blocks
static const int BW = 40, BH = 26;		// footprint, and the height carved out

struct Loot { int id, aux, lo, hi, w; };

static void fillChest(Level* level, Random* r, int x, int y, int z, const Loot* t, int n, int rolls)
{
	level->setTile(x, y, z, Tile::chest->id);
	TileEntity* te = level->getTileEntity(x, y, z);
	if (!TileEntity::isType(te, TileEntityType::Chest)) return;
	ChestTileEntity* chest = (ChestTileEntity*)te;
	int total = 0;
	for (int i = 0; i < n; ++i) total += t[i].w;
	for (int k = 0; k < rolls; ++k) {
		int pick = r->nextInt(total), i = 0;
		while ((pick -= t[i].w) >= 0) ++i;
		chest->setItem(r->nextInt(chest->getContainerSize()),
			new ItemInstance(t[i].id, t[i].lo + r->nextInt(t[i].hi - t[i].lo + 1), t[i].aux));
	}
}

static unsigned int bhash(unsigned int seed, int a, int b, int c)
{
	unsigned int h = seed ^ (unsigned int)(a * 73856093) ^ (unsigned int)(b * 19349663) ^ (unsigned int)(c * 83492791);
	h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
	return h;
}

struct Bastion { int x0, y0, z0; unsigned int seed; };

/// The bastion of the region holding (bx, bz), if it has one.
static bool bastionIn(long seed, int rx, int rz, Bastion& b)
{
	const unsigned int h = bhash((unsigned int)seed ^ 0x5a17b0u, rx, rz, 7);
	if (h % 5 < 2) return false;	// three regions in five
	b.x0 = rx * BASTION_REGION + 8 + (int)((h >> 4) % (BASTION_REGION - BW - 16));
	b.z0 = rz * BASTION_REGION + 8 + (int)((h >> 12) % (BASTION_REGION - BW - 16));
	b.y0 = 36 + (int)((h >> 20) % 10);	// clear of the lava sea at 31
	b.seed = h;
	// Every biome but the deltas, judged at its middle.
	return NetherBiomes::at(seed, b.x0 + BW / 2, b.z0 + BW / 2) != NetherBiomes::BasaltDeltas;
}

static int brick(unsigned int seed, int u, int h, int v)
{
	const unsigned int n = bhash(seed, u, h, v) % 100;
	if (n < 62) return Tile::polishedBlackstoneBricks->id;
	if (n < 84) return Tile::crackedPolishedBlackstoneBricks->id;
	if (n < 92) return Tile::blackstone->id;
	if (n < 96) return Tile::polishedBlackstone->id;
	if (n < 98) return Tile::gildedBlackstone->id;
	return Tile::chiseledPolishedBlackstone->id;
}

/// A two-block hole in the floor at storey `f`?
static bool hole(unsigned int seed, int u, int v, int f)
{
	return bhash(seed, u >> 1, v >> 1, 100 + f) % 100 < 13;
}

/** What the bastion has at (u, h, v), relative to its corner and floor:
    -1 leaves the terrain alone, 0 is air, anything else a tile. */
static int blockAt(const Bastion& b, int u, int h, int v)
{
	if (u < 0 || v < 0 || u >= BW || v >= BW || h >= BH) return -1;
	const unsigned int S = b.seed;
	const int c = BW / 2, du = u - c, dv = v - c;
	const int adu = du < 0 ? -du : du, adv = dv < 0 ? -dv : dv, ring = adu > adv ? adu : adv;
	const bool wall = u < 2 || v < 2 || u >= BW - 2 || v >= BW - 2;

	if (wall) {
		// Broken off at uneven heights, lowest towards the corners' gaps.
		const int top = 22 - (int)(bhash(S, u >> 1, 0, v >> 1) % 7) - (bhash(S, u >> 3, 1, v >> 3) % 3 == 0 ? 5 : 0);
		if (h > top) return 0;
		const bool mid = (adu <= 2 && (v < 2 || v >= BW - 2)) || (adv <= 2 && (u < 2 || u >= BW - 2));
		if (mid && h >= 1 && h <= 5) return 0;	// the gates
		if ((h == 9 || h == 10 || h == 16 || h == 17) && ((u + v) % 6 == 3)) return 0;	// windows
		return brick(S, u, h, v);
	}

	// Pillars of basalt up through the storeys, out of the atrium.
	if (ring > 9 && (u % 8) == 4 && (v % 8) == 4) return h <= 21 ? (h % 7 == 0 ? Tile::polishedBlackstone->id : Tile::polishedBasalt->id) : 0;

	// Stairs: ground floor to the first gallery along the north wall, the
	// first to the second along the south one. Steps of brick, headroom air.
	if (v >= 2 && v <= 3 && u >= 4 && u < 11) {
		const int step = u - 4 + 1;	// 1..7
		if (h < step) return h == 0 ? brick(S, u, h, v) : Tile::polishedBlackstone->id;
		if (h < step + 3) return 0;
	}
	if (v >= BW - 4 && v <= BW - 3 && u >= 4 && u < 11) {
		const int step = 7 + (u - 4) + 1;	// 8..14
		if (h >= 7 && h < step) return Tile::polishedBlackstone->id;
		if (h >= step && h < step + 3) return 0;
	}

	if (h == 0) {
		// The ground floor: the dais, its moat and bridges, holes elsewhere.
		if (ring <= 2) return brick(S, u, h, v);
		if (ring <= 6) return (du == 0 || dv == 0) ? Tile::polishedBlackstoneBricks->id : Tile::calmLava->id;
		if (hole(S, u, v, 0)) return 0;
		return brick(S, u, h, v);
	}
	if (ring <= 2 && h <= 2) {
		// The dais: two steps up, gold round its rim.
		if (h == 1) return ring == 2 ? ((adu == 2 && adv == 2) || bhash(S, u, h, v) % 3 == 0 ? Tile::goldBlock->id : Tile::polishedBlackstone->id) : Tile::blackstone->id;
		return ring <= 1 && !(du == 0 && dv == 0) ? Tile::goldBlock->id * (bhash(S, u, 2, v) % 2) : 0;
	}
	if (h == 7 || h == 14) {
		const int f = h / 7;
		if (ring <= 9) return 0;	// the atrium
		if (hole(S, u, v, f)) return 0;
		if (bhash(S, u >> 1, v >> 1, 200 + f) % 100 < 5) return Tile::calmLava->id;	// a basin
		return brick(S, u, h, v);
	}
	if ((h == 8 || h == 15) && ring == 10 && bhash(S, u, h, v) % 3 != 0)
		return Tile::polishedBlackstoneBricks->id;	// a broken railing round the atrium
	return 0;
}

static void fillChestAt(Level* level, Random* r, int x, int y, int z, bool isTreasure)
{
	static Loot common[] = {
		{0, 0, 2, 6, 12}, {0, 0, 4, 12, 12}, {0, 0, 1, 4, 8}, {0, 0, 1, 1, 6}, {0, 0, 1, 1, 6},
		{0, 0, 1, 1, 4}, {0, 0, 1, 1, 4}, {0, 0, 2, 6, 6}, {0, 0, 5, 12, 8}, {0, 0, 2, 5, 6},
		{0, 0, 1, 1, 2}, {0, 0, 1, 1, 2}, {0, 0, 2, 5, 6}
	};
	static Loot treasure[] = {
		{0, 0, 3, 9, 10}, {0, 0, 1, 1, 5}, {0, 0, 1, 2, 5}, {0, 0, 1, 1, 3}, {0, 0, 1, 1, 3},
		{0, 0, 1, 1, 4}, {0, 0, 1, 2, 6}, {0, 0, 1, 1, 2}
	};
	static bool init = false;
	if (!init) {
		init = true;
		const int c[13] = { Item::goldIngot->id, Item::goldNugget->id, Item::ironIngot->id, Item::sword_gold->id, Item::hatchet_gold->id,
			Item::helmet_gold->id, Item::boots_gold->id, Item::arrow->id, Item::string->id, Item::magmaCream->id,
			Tile::ancientDebris->id, Item::netheriteScrap->id, Tile::obsidian->id };
		for (int i = 0; i < 13; ++i) common[i].id = c[i];
		const int t[8] = { Item::goldIngot->id, Item::netheriteIngot->id, Item::netheriteScrap->id, Item::sword_netherite->id,
			Item::chestplate_diamond->id, Tile::ancientDebris->id, Item::emerald->id, Tile::goldBlock->id };
		for (int i = 0; i < 8; ++i) treasure[i].id = t[i];
	}
	if (isTreasure) fillChest(level, r, x, y, z, treasure, 8, 5 + r->nextInt(3));
	else fillChest(level, r, x, y, z, common, 13, 4 + r->nextInt(5));
}

/// Builds the part of the bastion b inside the window [wx, wx+16) x [wz, wz+16).
static void buildBastion(Level* level, Random* r, const Bastion& b, int wx, int wz)
{
	const int bs = Tile::polishedBlackstoneBricks->id;
	for (int x = wx; x < wx + 16; ++x)
		for (int z = wz; z < wz + 16; ++z) {
			const int u = x - b.x0, v = z - b.z0;
			if (u < 0 || v < 0 || u >= BW || v >= BW) continue;
			for (int h = 0; h < BH; ++h) {
				const int t = blockAt(b, u, h, v);
				if (t >= 0) level->setTileNoUpdate(x, b.y0 + h, z, t);
			}
			// Under the ground floor: lava under its holes (a pit two deep),
			// and the whole footprint founded down to whatever is below.
			int y = b.y0 - 1;
			if (blockAt(b, u, 0, v) == 0) {
				level->setTileNoUpdate(x, y--, z, Tile::calmLava->id);
				level->setTileNoUpdate(x, y--, z, Tile::calmLava->id);
			}
			for (int k = 0; k < 40 && y > 1; ++k, --y) {
				const int t = level->getTile(x, y, z);
				if (k > 0 && t > 0 && Tile::solid[t]) break;
				level->setTileNoUpdate(x, y, z, (u % 8 == 4 && v % 8 == 4) ? Tile::basalt->id : bs);
			}
		}

	// Chests and the guard, each with the chunk that holds its spot.
	#define INWIN(px, pz) ((px) >= wx && (px) < wx + 16 && (pz) >= wz && (pz) < wz + 16)
	const int cx = b.x0 + BW / 2, cz = b.z0 + BW / 2;
	if (INWIN(cx, cz)) fillChestAt(level, r, cx, b.y0 + 2, cz, true);
	static const int spots[6][3] = { {3, 1, 6}, {BW - 4, 1, BW - 7}, {3, 8, BW - 8}, {BW - 4, 8, 5}, {5, 15, 6}, {BW - 6, 15, BW - 5} };
	for (int i = 0; i < 6; ++i) {
		const int px = b.x0 + spots[i][0], pz = b.z0 + spots[i][2];
		if (INWIN(px, pz)) fillChestAt(level, r, px, b.y0 + spots[i][1], pz, false);
	}
	// Brutes by the dais' bridges; piglins on the floors and galleries.
	static const int guard[12][4] = {
		{0, 0, -4, 1}, {0, 0, 4, 1}, {-4, 0, 0, 1}, {4, 0, 0, 1},
		{-14, 1, -14, 0}, {14, 1, 14, 0}, {-14, 1, 12, 0}, {12, 1, -14, 0},
		{-15, 8, 0, 0}, {15, 8, 0, 0}, {0, 15, -15, 0}, {0, 15, 15, 0} };
	for (int i = 0; i < 12; ++i) {
		const int px = cx + guard[i][0], pz = cz + guard[i][2];
		if (!INWIN(px, pz)) continue;
		Piglin* p = new Piglin(level, guard[i][3] != 0);
		p->moveTo(px + 0.5f, (float)(b.y0 + guard[i][1] + (guard[i][3] ? 1 : 0)), pz + 0.5f, r->nextFloat() * 360, 0);
		level->addEntity(p);
	}
	#undef INWIN
}

/// Any bastion overlapping this chunk's window gets its part built.
static void bastions(Level* level, Random* r, int xo, int zo, long seed)
{
	const int wx = xo + 8, wz = zo + 8;
	const int rx0 = (wx - BW >= 0 ? wx - BW : wx - BW - BASTION_REGION + 1) / BASTION_REGION;
	const int rz0 = (wz - BW >= 0 ? wz - BW : wz - BW - BASTION_REGION + 1) / BASTION_REGION;
	for (int rx = rx0; rx <= rx0 + 1; ++rx)
		for (int rz = rz0; rz <= rz0 + 1; ++rz) {
			Bastion b;
			if (!bastionIn(seed, rx, rz, b)) continue;
			if (b.x0 >= wx + 16 || b.x0 + BW <= wx || b.z0 >= wz + 16 || b.z0 + BW <= wz) continue;
			buildBastion(level, r, b, wx, wz);
		}
}

void decorate(Level* level, Random* r, int xo, int zo, long seed)
{
	const int cx = xo + 16, cz = zo + 16;
	const int biome = NetherBiomes::at(seed, cx, cz);

	if (biome == NetherBiomes::CrimsonForest || biome == NetherBiomes::WarpedForest) {
		const bool crimson = biome == NetherBiomes::CrimsonForest;
		const int nylium = crimson ? Tile::crimsonNylium->id : Tile::warpedNylium->id;
		forestField(level, r, xo, zo, crimson);
	} else if (biome == NetherBiomes::BasaltDeltas) {
		// Lakes first, then columns over them (Java's order); counts per
		// chunk follow Java's 1.16 biome: 40 deltas, 4 small and 2 large
		// column clusters, 16 springs - sampled down at several heights.
		for (int i = 0; i < 24; ++i)
			delta(level, r, xo + 8 + r->nextInt(16), 10 + r->nextInt(110), zo + 8 + r->nextInt(16));
		for (int i = 0; i < 4; ++i)
			basaltColumns(level, r, xo + 8 + r->nextInt(16), 10 + r->nextInt(110), zo + 8 + r->nextInt(16), true);
		columnField(level, r, xo, zo, seed);
		for (int i = 0; i < 16; ++i)
			lavaSpring(level, r, xo + 8 + r->nextInt(16), 10 + r->nextInt(108), zo + 8 + r->nextInt(16));
		for (int i = 0; i < 4; ++i) {
			OreFeature blobs(Tile::blackstone->id, 20, Tile::basalt->id);
			blobs.place(level, r, xo + r->nextInt(16), 10 + r->nextInt(100), zo + r->nextInt(16));
		}
		for (int i = 0; i < 4; ++i) {
			OreFeature magma(Tile::magmaBlock->id, 10, Tile::netherrack->id);
			magma.place(level, r, xo + r->nextInt(16), 26 + r->nextInt(12), zo + r->nextInt(16));
		}
	} else if (biome == NetherBiomes::SoulSandValley) {
		soulValleyField(level, r, xo, zo, seed);
		// Java's nether fossils: one chunk in three or so.
		if (r->nextInt(3) == 0) fossilArch(level, r, xo + 10 + r->nextInt(10), 20 + r->nextInt(80), zo + 10 + r->nextInt(10));
		// Basalt pillars floor to ceiling, two a chunk, as Java's valleys.
		for (int i = 0; i < 2; ++i) {
			const int x = xo + 8 + r->nextInt(16), z = zo + 8 + r->nextInt(16);
			const int y = floorBelow(level, x, 10 + r->nextInt(100), z, 40);
			if (y < 0) continue;
			int top = y;
			while (top < 126 && level->getTile(x, top, z) == 0) ++top;
			if (top >= 126 || top - y > 60) continue;	// no ceiling over it
			for (int j = y; j < top; ++j) {
				level->setTileNoUpdate(x, j, z, Tile::basalt->id);
				// Ragged: a block or two out from it, here and there.
				if (r->nextInt(5) == 0) put(level, x + r->nextInt(3) - 1, j, z + r->nextInt(3) - 1, Tile::basalt->id);
			}
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

	// Bastions: every biome but the basalt deltas, as Java's. Last, so
	// nothing grows through them.
	bastions(level, r, xo, zo, seed);
}

}
