#include "MobSpawnerTileEntity.h"
#include "../MobSpawnerTile.h"
#include "../../Level.h"
#include "../../MobSpawner.h"
#include "../../../entity/Entity.h"
#include "../../../entity/EntityTypes.h"
#include "../../../entity/MobFactory.h"
#include "../../../../util/Random.h"

static const float ACTIVATE_DISTANCE = 16.0f;   // a player this close wakes the spawner
static const float SPAWN_AREA = 8.0f;           // mobs of its kind within this many blocks count
static const int MAX_NEARBY = 6;
static const int MIN_DELAY = 100;               // ticks between spawns: 5 to 15 seconds
static const int DELAY_RANGE = 200;

MobSpawnerTileEntity::MobSpawnerTileEntity()
:	super(TileEntityType::MobSpawner),
	spawnDelay(20)
{
}

bool MobSpawnerTileEntity::shouldSave() {
	return true;
}

void MobSpawnerTileEntity::tick() {
	if (level == NULL || level->isClientSide)
		return;

	// Asleep until a player is near.
	if (!level->getNearestPlayer(x + 0.5f, y + 0.5f, z + 0.5f, ACTIVATE_DISTANCE))
		return;

	if (spawnDelay > 0) {
		--spawnDelay;
		return;
	}
	spawnDelay = MIN_DELAY + level->random.nextInt(DELAY_RANGE);

	trySpawn(MobSpawnerTile::mobTypeFor(getData()));
}

// Puts one mob on a free spot within four blocks, unless too many of its kind are
// already about.
bool MobSpawnerTileEntity::trySpawn(int mobType) {
	const float cx = x + 0.5f, cy = y + 0.5f, cz = z + 0.5f;

	int nearby = 0;
	const EntityList& entities = level->getAllEntities();
	for (unsigned int i = 0; i < entities.size(); ++i) {
		Entity* e = entities[i];
		if (e->getEntityTypeId() != mobType)
			continue;
		float dx = e->x - cx, dy = e->y - cy, dz = e->z - cz;
		if (dx * dx + dy * dy + dz * dz < SPAWN_AREA * SPAWN_AREA)
			++nearby;
	}
	if (nearby >= MAX_NEARBY)
		return false;

	for (int attempt = 0; attempt < 12; ++attempt) {
		int bx = x + level->random.nextInt(9) - 4;
		int by = y + level->random.nextInt(3) - 1;
		int bz = z + level->random.nextInt(9) - 4;
		if (!level->isEmptyTile(bx, by, bz) || !level->isEmptyTile(bx, by + 1, bz))
			continue;

		Mob* mob = MobFactory::CreateMob(mobType, level);
		if (!mob)
			return false;
		if (MobSpawner::addMob(level, mob, bx + 0.5f, (float) by, bz + 0.5f, level->random.nextFloat() * 360.0f, 0, true))
			return true;
		delete mob;
		return false;
	}
	return false;
}
