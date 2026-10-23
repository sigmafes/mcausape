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
	const int rx = 3 + r->nextInt(5), rz = 3 + r->nextInt(5), rim = r->nextInt(3);
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


/** A small bastion remnant (phase 7). Java's are huge jigsaw structures of
    polished blackstone bricks; this is one tower-courtyard of blackstone and
    basalt, 15 across, on stilts down to the ground: open doorways on each
    side, a gallery round the inside four blocks up, a gold-trimmed treasure
    dais in the middle, chests, and a guard of piglin brutes. It is built
    entirely inside the chunk being decorated (x/z 8..22 of the decorator's
    window), so it never reaches into an ungenerated one. */
static const int BASTION_REGION = 6;	// chunks; one chance per 6x6

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

static bool isBastionChunk(long seed, int cx, int cz)
{
	const int rx = (cx >= 0 ? cx : cx - BASTION_REGION + 1) / BASTION_REGION;
	const int rz = (cz >= 0 ? cz : cz - BASTION_REGION + 1) / BASTION_REGION;
	Random h((long)(rx * 341873128712LL + rz * 132897987541LL) ^ seed ^ 30084232LL);
	if (h.nextInt(5) < 2) return false;	// about 3 regions in 5 have one
	return cx == rx * BASTION_REGION + 1 + h.nextInt(BASTION_REGION - 2)
		&& cz == rz * BASTION_REGION + 1 + h.nextInt(BASTION_REGION - 2);
}

static void bastion(Level* level, Random* r, int xo, int zo)
{
	const int S = 15, x0 = xo + 8, z0 = zo + 8, mid = S / 2;
	// The floor: the highest open floor under y 80 at the middle, kept off
	// the lava sea.
	int y0 = floorBelow(level, x0 + mid, 80, z0 + mid, 48);
	if (y0 < 0 || y0 < 34) y0 = 40;
	const int H = 9;
	const int bs = Tile::blackstone->id, ba = Tile::basalt->id;

	for (int dx = 0; dx < S; ++dx)
		for (int dz = 0; dz < S; ++dz) {
			const int x = x0 + dx, z = z0 + dz;
			const bool edge = dx == 0 || dz == 0 || dx == S - 1 || dz == S - 1;
			const bool corner = (dx == 0 || dx == S - 1) && (dz == 0 || dz == S - 1);
			const bool door = (dx >= mid - 1 && dx <= mid + 1) || (dz >= mid - 1 && dz <= mid + 1);
			// Floor, and stilts down to whatever is below.
			level->setTileNoUpdate(x, y0 - 1, z, (dx + dz) % 7 == 0 ? Tile::magmaBlock->id : bs);
			if (edge || (dx % 4 == 3 && dz % 4 == 3))
				for (int y = y0 - 2; y > y0 - 30 && y > 1 && !solidAt(level, x, y, z); --y)
					level->setTileNoUpdate(x, y, z, edge && !corner ? bs : ba);
			for (int y = y0; y < y0 + H + 2; ++y) {
				int t = 0;
				const int up = y - y0;
				if (edge) {
					if (up < H) t = corner ? ba : bs;
					else if (up == H) t = (dx + dz) % 2 == 0 ? bs : 0;	// battlements
					if (door && !corner && up < 4) t = 0;
					if (!corner && up == 6 && (dx + dz) % 3 == 0) t = 0;	// windows
				} else {
					// The gallery: a ring three wide, four up, with a gold trim.
					const bool ring = dx <= 3 || dz <= 3 || dx >= S - 4 || dz >= S - 4;
					const bool lip = dx == 3 || dz == 3 || dx == S - 4 || dz == S - 4;
					if (up == 4 && ring) t = lip && (dx * dz) % 5 == 0 ? Tile::goldBlock->id : bs;
				}
				level->setTileNoUpdate(x, y, z, t);
			}
		}
	// Columns holding the gallery up, and ladders of basalt steps to it.
	for (int i = 0; i < 4; ++i) {
		const int cx = x0 + (i & 1 ? S - 4 : 3), cz = z0 + (i & 2 ? S - 4 : 3);
		for (int y = y0; y < y0 + 4; ++y) level->setTileNoUpdate(cx, y, cz, ba);
	}
	for (int k = 0; k < 4; ++k) level->setTileNoUpdate(x0 + 4 + k, y0 + k, z0 + 1, bs);
	for (int k = 0; k < 4; ++k) {
		level->setTileNoUpdate(x0 + 4 + k, y0 + k + 1, z0 + 1, 0);
		level->setTileNoUpdate(x0 + 4 + k, y0 + k + 2, z0 + 1, 0);
	}

	// The treasure dais: blackstone, gold at the corners, the best chest.
	for (int dx = -1; dx <= 1; ++dx)
		for (int dz = -1; dz <= 1; ++dz)
			level->setTileNoUpdate(x0 + mid + dx, y0, z0 + mid + dz, (dx && dz) ? Tile::goldBlock->id : bs);

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
	fillChest(level, r, x0 + mid, y0 + 1, z0 + mid, treasure, 8, 5 + r->nextInt(3));
	fillChest(level, r, x0 + 1, y0 + 5, z0 + S - 2, common, 13, 5 + r->nextInt(4));
	fillChest(level, r, x0 + S - 2, y0 + 5, z0 + 1, common, 13, 5 + r->nextInt(4));
	fillChest(level, r, x0 + S - 2, y0, z0 + S - 2, common, 13, 4 + r->nextInt(4));

	// The guard: brutes on the floor, piglins in the gallery. Brutes are
	// never spawned anywhere else (as Java's).
	for (int i = 0; i < 5; ++i) {
		const bool brute = i < 2 + r->nextInt(2);
		Piglin* p = new Piglin(level, brute);
		const float px = x0 + 2.5f + r->nextInt(S - 4), pz = z0 + 2.5f + r->nextInt(S - 4);
		const float py = brute ? (float)y0 : (float)(y0 + 5);
		float gx = px, gz = pz;
		if (!brute) { gx = x0 + 1.5f + r->nextInt(2); gz = z0 + 2.5f + r->nextInt(S - 4); }
		p->moveTo(gx, py, gz, r->nextFloat() * 360, 0);
		level->addEntity(p);
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
		// Lakes first, then columns over them (Java's order); counts per
		// chunk follow Java's 1.16 biome: 40 deltas, 4 small and 2 large
		// column clusters, 16 springs - sampled down at several heights.
		for (int i = 0; i < 24; ++i)
			delta(level, r, xo + 8 + r->nextInt(16), 10 + r->nextInt(110), zo + 8 + r->nextInt(16));
		for (int i = 0; i < 6; ++i)
			basaltColumns(level, r, xo + 8 + r->nextInt(16), 10 + r->nextInt(110), zo + 8 + r->nextInt(16), false);
		for (int i = 0; i < 3; ++i)
			basaltColumns(level, r, xo + 8 + r->nextInt(16), 10 + r->nextInt(110), zo + 8 + r->nextInt(16), true);
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

	// Bastions: every biome but the basalt deltas, as Java's. Last, so
	// nothing grows through them.
	if (biome != NetherBiomes::BasaltDeltas && isBastionChunk(seed, xo >> 4, zo >> 4))
		bastion(level, r, xo, zo);
}

}
