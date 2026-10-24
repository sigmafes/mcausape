#include "MobSpawnerTile.h"
#include "entity/MobSpawnerTileEntity.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../entity/EntityTypes.h"

const std::string MobSpawnerTile::KIND_NAMES[] = {
	"zombie", "skeleton", "spider", "pigZombie", "witherSkeleton", "blaze", "magmaCube", "ghast"
};

MobSpawnerTile::MobSpawnerTile(int id, int tex)
:	super(id, tex, Material::stone)
{
	// The random tick only heals a spawner placed before it had a tile entity.
	setTicking(true);
}

int MobSpawnerTile::mobTypeFor(int data)
{
	switch (data) {
	case SKELETON:        return MobTypes::Skeleton;
	case SPIDER:          return MobTypes::Spider;
	case PIG_ZOMBIE:      return MobTypes::PigZombie;
	case WITHER_SKELETON: return MobTypes::WitherSkeleton;
	case BLAZE:           return MobTypes::Blaze;
	case MAGMA_CUBE:      return MobTypes::MagmaCube;
	case GHAST:           return MobTypes::Ghast;
	default:              return MobTypes::Zombie;
	}
}

TileEntity* MobSpawnerTile::newTileEntity()
{
	return new MobSpawnerTileEntity();
}

// One cage picture per kind, tinted differently (see tools/make_fire_and_spawner_assets.py):
// kind 0 in cell (8,5) of the terrain atlas, kinds 1 to 7 in row 11, columns 4 to 10.
int MobSpawnerTile::getTexture(int face, int data)
{
	int kind = data & 7;
	if (kind == 0) return 264;	// the zombie cage (8 + 5 * 16 is the crops)
	return (3 + kind) + 11 * 16;
}

void MobSpawnerTile::tick(Level* level, int x, int y, int z, Random* random)
{
	// Asking for the tile entity makes one if the block has none.
	level->getTileEntity(x, y, z);
}

// A spawner gives nothing when broken: in the game it is taken with a pickaxe and
// leaves an empty block behind.
int MobSpawnerTile::getResource(int data, Random* random)
{
	return -1;
}

int MobSpawnerTile::getResourceCount(Random* random)
{
	return 0;
}
