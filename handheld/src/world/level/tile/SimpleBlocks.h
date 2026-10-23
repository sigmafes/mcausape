#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__SimpleBlocks_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__SimpleBlocks_H__

#include "Tile.h"
#include "../material/Material.h"
#include "../../item/Item.h"

/// Java's bone block: the sides at tex, the top and bottom at tex + 1.
class BoneBlockTile: public Tile
{
public:
	BoneBlockTile(int id, int tex) : Tile(id, tex, Material::stone) {}
	int getTexture(int face) { return face <= 1 ? tex + 1 : tex; }
	int getTexture(int face, int data) { return getTexture(face); }
};

/// The sea lantern: full light; broken it gives back the four ocean shards
/// it is made of.
class SeaLanternTile: public Tile
{
public:
	SeaLanternTile(int id, int tex) : Tile(id, tex, Material::glass) {}
	/// Lit by itself, as glowstone looks, even deep in the dark.
	float getBrightness(LevelSource* level, int x, int y, int z) { return 1.0f; }
	int getResource(int data, Random* random) { return Item::oceanShard->id; }
	int getResourceCount(Random* random) { return 4; }
};

/// Prismarine: drops itself.
class PrismarineTile: public Tile
{
public:
	PrismarineTile(int id, int tex) : Tile(id, tex, Material::stone) {}
};

#endif
