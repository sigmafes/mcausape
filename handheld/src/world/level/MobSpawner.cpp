#include "MobSpawner.h"

#include <algorithm>

#include "Level.h"
#include "LevelConstants.h"
#include "biome/Biome.h"
#include "material/Material.h"
#include "../entity/EntityTypes.h"
#include "../entity/MobFactory.h"
#include "../entity/MobCategory.h"
#include "../entity/player/Player.h"
#include "dimension/Dimension.h"

//#include "../entity/animal/Sheep.h"
//#include "tile/BedTile.h"

std::map<ChunkPos, bool> MobSpawner::chunksToPoll;

static int _bedEnemies[] = {
	MobTypes::Spider,
	MobTypes::Zombie,
	MobTypes::Skeleton,
	MobTypes::PigZombie
};

static const std::vector<int> bedEnemies(_bedEnemies, _bedEnemies + sizeof(_bedEnemies) / sizeof(_bedEnemies[0]));

/*static*/
int MobSpawner::tick(Level* level, bool spawnEnemies, bool spawnFriendlies) {

	//return 0;

	if (!spawnEnemies && !spawnFriendlies) {
        return 0;
    }
	// The Aquarium: fish only, in schools, anywhere in its water.
	if (level->dimension && level->dimension->isAllWater())
		return spawnFish(level);

    chunksToPoll.clear();
	// Add all chunks as a quick-and-dirty test
	// (The code above is the same as in the java version)
	// This code goes over the whole map one "row" at a time

	// Spawn friendlies == loop over whole map, and disable Monster spawning this tick
	if (spawnFriendlies) {
		spawnEnemies = false;
		for (int i = 0; i < CHUNK_CACHE_WIDTH * CHUNK_CACHE_WIDTH; ++i)
			chunksToPoll.insert( std::make_pair( ChunkPos(i / CHUNK_CACHE_WIDTH, i % CHUNK_CACHE_WIDTH), false) );

	} else {
		// Only spawn mobs, check around one player per tick (@todo: optimize the "count instances of"?)
		static unsigned int _pid = 0;
		if (++_pid >= level->players.size()) _pid = 0;
		if (level->players.size()) {
			Player* p = level->players[_pid];
			int xx = Mth::floor(p->x / 16);
			int zz = Mth::floor(p->z / 16);
			int r = 128 / 16;
			for (int x = -r; x <= r; x++)
			for (int z = -r; z <= r; z++) {
				const int cx = xx + x;
				const int cz = zz + z;
				if (cx >= 0 && cx < CHUNK_CACHE_WIDTH && cz >= 0 && cz < CHUNK_CACHE_WIDTH)
					chunksToPoll.insert(std::make_pair(ChunkPos(cx, cz), false ));
			}
		}
	}

    int count = 0;
    Pos spawnPos = level->getSharedSpawnPos();

	for (int i = 0; i < MobCategory::numValues; ++i) {
		const MobCategory& mobCategory = *MobCategory::values[i];

        if ((mobCategory.isFriendly() && !spawnFriendlies) || (!mobCategory.isFriendly() && !spawnEnemies))
            continue;

		int numMobs = level->countInstanceOfBaseType(mobCategory.getBaseClassId());
		// The Nether is allowed more monsters than the Overworld, as in Minecraft
		// (70 against 20 here). Zombie pigmen are 100 of the weights there and
		// come four at a time, so with the Overworld's cap of 20 they filled it
		// within seconds and nothing else -- ghasts, and the blazes and wither
		// skeletons of the fortress -- was ever allowed to spawn.
		const bool nether = level->dimension && level->dimension->id == Dimension::NETHER;
		const int maxMobs = (nether && !mobCategory.isFriendly()) ? 70 : mobCategory.getMaxInstancesPerLevel();
		if (numMobs > maxMobs)
		    continue;
		//LOGI("NumMobs: %d of Category: %d\n", numMobs, mobCategory.getBaseClassId());
		for(std::map<ChunkPos, bool>::iterator it = chunksToPoll.begin(); it != chunksToPoll.end(); ++it) {
			const ChunkPos& cp = it->first;
            TilePos start = getRandomPosWithin(level, cp.x * 16, cp.z * 16);
            int xStart = start.x;
            int yStart = start.y;
            int zStart = start.z;

			if (level->isSolidBlockingTile(xStart, yStart, zStart)) continue;

            if (level->getMaterial(xStart, yStart, zStart) != mobCategory.getSpawnPositionMaterial()) continue;

            int clusterSize = 0;

            for (int dd = 0; dd < 3; dd++) {
                int x = xStart;
                int y = yStart;
                int z = zStart;
                int ss = 6;

                Biome::MobSpawnerData currentMobType;
				int maxCreatureCount = 999;
				int currentCreatureCount = 0;

                for (int ll = 0; ll < 4; ll++) {
					if (currentCreatureCount > maxCreatureCount)
						break;

					x += level->random.nextInt(ss) - level->random.nextInt(ss);
                    y += level->random.nextInt(1) - level->random.nextInt(1);
                    z += level->random.nextInt(ss) - level->random.nextInt(ss);
                    // int y = heightMap[x + z * w] + 1;

                    if (isSpawnPositionOk(mobCategory, level, x, y, z)) {
                        float xx = (float)x + 0.5f;
                        float yy = (float)y;
                        float zz = (float)z + 0.5f;
                        if (level->getNearestPlayer(xx, yy, zz, (float)MIN_SPAWN_DISTANCE) != NULL) {
                            continue;
                        } else {
                            float xd = xx - spawnPos.x;
                            float yd = yy - spawnPos.y;
                            float zd = zz - spawnPos.z;
                            float sd = xd * xd + yd * yd + zd * zd;
                            if (sd < MIN_SPAWN_DISTANCE * MIN_SPAWN_DISTANCE) {
                                continue;
                            }
                        }

						static Stopwatch sw;
						sw.start();

						if (!currentMobType.isValid()) {
                            currentMobType = level->getRandomMobSpawnAt(mobCategory, x, y, z);
							if (!currentMobType.isValid())
                                break;

							// Don't allow monster to spawn (much) more than their defined
							// probability weight.
							if (&mobCategory == &MobCategory::monster) {
								int typeCount = level->countInstanceOfType(currentMobType.mobClassId);
								// A type's share of the cap follows its weight in the list
								// that applies *here* -- the fortress's, say, not the
								// Overworld's -- and is always room for one group.
								Biome::MobList here = level->getMobSpawnList(mobCategory, x, y, z);
								int totalWeight = 0;
								for (unsigned int w = 0; w < here.size(); ++w)
									totalWeight += here[w].randomWeight;
								if (totalWeight <= 0) totalWeight = Biome::defaultTotalEnemyWeight;
								int typeMax = (int)(1.5f * currentMobType.randomWeight * maxMobs) / totalWeight;
								if (typeMax < currentMobType.maxCount) typeMax = currentMobType.maxCount;
								//LOGI("Has %d (max %d) of type: %d\n", typeCount, typeMax, currentMobType.mobClassId);
								if (typeCount >= typeMax)
									break;
							}
							
							maxCreatureCount = currentMobType.minCount + level->random.nextInt(1 + currentMobType.maxCount - currentMobType.minCount);
                        }

						Mob* tmp = MobFactory::getStaticTestMob(currentMobType.mobClassId, level);
						if (!tmp) continue;

						tmp->moveTo(xx, yy, zz, 0, 0);
						if (!tmp->canSpawn()) continue;

						Mob* mob = MobFactory::CreateMob(currentMobType.mobClassId, level);
						if (!mob) continue;

						if (addMob(level, mob, xx, yy, zz, level->random.nextFloat() * 360, 0, false)) {
							++currentCreatureCount;
							// Java's "continue chunkLoop": on to the next chunk. This was a
							// goto to the label above the chunk loop, which starts it over
							// from the first chunk, without checking the cap again: every
							// full group restarted the whole pass. Land animals rarely
							// filled one; squid in the Overworld's water do, and the
							// spawner went on placing them forever on one frame.
							if (++clusterSize >= mob->getMaxSpawnClusterSize()) goto nextChunk;
						}
						else
							delete mob;

                        count += clusterSize;
                    }
                }
            }
nextChunk:
			;
        }
    }
    return count;
}

/*static*/
void MobSpawner::postProcessSpawnMobs(Level* level, Biome* biome, int xo, int zo, int cellWidth, int cellHeight, Random* random) {

	//return;

    Biome::MobList mobs = biome->getMobs(MobCategory::creature);
    if (mobs.empty()) {
        return;
    }

    while (random->nextFloat() < biome->getCreatureProbability()) {

        Biome::MobSpawnerData* type = (Biome::MobSpawnerData*) WeighedRandom::getRandomItem(&level->random, mobs);
        int count = type->minCount + random->nextInt(1 + type->maxCount - type->minCount);

        int x = xo + random->nextInt(cellWidth);
        int z = zo + random->nextInt(cellHeight);
        int startX = x, startZ = z;

        for (int c = 0; c < count; c++) {
            bool success = false;
            for (int attempts = 0; !success && attempts < 4; attempts++) {
                // these mobs always spawn at the topmost position
                int y = level->getTopSolidBlock(x, z);
                if (isSpawnPositionOk(MobCategory::creature, level, x, y, z)) {

                    float xx = (float)x + 0.5f;
                    float yy = (float)y;
                    float zz = (float)z + 0.5f;

                    Mob* mob = MobFactory::CreateMob(type->mobClassId, level);
					if (!mob) continue;

                    // System.out.println("Placing night mob");
                    mob->moveTo(xx, yy, zz, random->nextFloat() * 360, 0);

                    level->addEntity(mob);
                    finalizeMobSettings(mob, level, xx, yy, zz);
                    success = true;
                }

                x += random->nextInt(5) - random->nextInt(5);
                z += random->nextInt(5) - random->nextInt(5);
                while (x < xo || x >= (xo + cellWidth) || z < zo || z >= (zo + cellWidth)) {
                    x = startX + random->nextInt(5) - random->nextInt(5);
                    z = startZ + random->nextInt(5) - random->nextInt(5);
                }
            }
        }
    }
}

/*static*/
TilePos MobSpawner::getRandomPosWithin(Level* level, int xo, int zo) {
    int x = xo + level->random.nextInt(16);
	int y = level->random.nextInt(Level::DEPTH); //@note: level->depth);
    int z = zo + level->random.nextInt(16);

    return TilePos(x, y, z);
}

/*static*/
bool MobSpawner::isSpawnPositionOk(const MobCategory& category, Level* level, int x, int y, int z) {
    if (category.getSpawnPositionMaterial() == Material::water) {
        return level->getMaterial(x, y, z)->isLiquid() && !level->isSolidBlockingTile(x, y + 1, z);
    } else {
        return level->isSolidBlockingTile(x, y - 1, z) && !level->isSolidBlockingTile(x, y, z) && !level->getMaterial(x, y, z)->isLiquid() && !level->isSolidBlockingTile(x, y + 1, z);
    }
}

/*static*/
void MobSpawner::finalizeMobSettings(Mob* mob, Level* level, float xx, float yy, float zz) {
	// @todo
//         if (mob instanceof Spider && level->random->nextInt(100) == 0) {
//             Skeleton skeleton = /*new*/ Skeleton(level);
//             skeleton.moveTo(xx, yy, zz, mob.yRot, 0);
//             level->addEntity(skeleton);
//             skeleton.ride(mob);
//         } else if (mob instanceof Sheep) {
//             ((Sheep) mob).setColor(Sheep.getSheepColor(level->random));
//         }

	if (mob->getEntityTypeId() == MobTypes::Sheep) {
		((Sheep*) mob)->setColor(Sheep::getSheepColor(&level->random));
	}

	makeBabyMob(mob, 0.5f);
}

/*static*/
bool MobSpawner::addMob(Level* level, Mob* mob, float xx, float yy, float zz, float yRot, float xRot, bool force)
{
	mob->moveTo(xx, yy, zz, yRot, xRot);

	if (force || mob->canSpawn()) {
		if (mob->getEntityTypeId() == MobTypes::PigZombie)
			LOGI("MobSpawner: pigman at %.0f, %.0f, %.0f\n", xx, yy, zz);
		level->addEntity(mob);
		finalizeMobSettings(mob, level, xx, yy, zz);
		return true;
	} else {
		//LOGI("Couldn't add the entity\n");
		return false;
	}	
}

void MobSpawner::makeBabyMob( Mob* mob, float probability ) {
	static Random babyRandom(98495119L);
	if (MobTypes::BaseCreature == mob->getCreatureBaseType()) {
		if (babyRandom.nextFloat() < probability)
			((Animal*)mob)->setAge(-20 * 60 * SharedConstants::TicksPerSecond);
	}
}

/** The Aquarium's own spawner: its biome is the Nether's (nothing asks it
    for more) and its water is air, which the usual spawner's material check
    never matches. A school of cod or salmon now and then near a player, up
    to the water creatures' cap. */
int MobSpawner::spawnFish(Level* level)
{
	if (level->players.empty() || level->random.nextInt(20) != 0) return 0;
	if (level->countInstanceOfBaseType(MobTypes::BaseWaterCreature) >= MobCategory::waterCreature.getMaxInstancesPerLevel())
		return 0;
	Player* p = level->players[level->random.nextInt((int) level->players.size())];
	const int x = Mth::floor(p->x) + level->random.nextInt(65) - 32;
	const int z = Mth::floor(p->z) + level->random.nextInt(65) - 32;
	const int y = 8 + level->random.nextInt(110);
	if (x < 0 || z < 0 || x >= LEVEL_WIDTH || z >= LEVEL_DEPTH || !level->allWaterAt(x, y, z)) return 0;
	if (level->getNearestPlayer(x + 0.5f, (float) y, z + 0.5f, (float) MIN_SPAWN_DISTANCE) != NULL) return 0;
	// Pufferfish keep to the deep, under the shelf (its underside is 38-62);
	// shrimp anywhere, in schools like the fish.
	const bool deep = y < 50;
	const int roll = level->random.nextInt(8);
	int type;
	if (roll < 3) type = MobTypes::Cod;
	else if (roll < 5) type = MobTypes::Shrimp;
	else if (roll < 7 && deep) type = MobTypes::Pufferfish;
	else type = MobTypes::Salmon;
	const int count = (type == MobTypes::Cod || type == MobTypes::Shrimp) ? 3 + level->random.nextInt(4) : 1 + level->random.nextInt(3);
	int made = 0;
	for (int i = 0; i < count; ++i) {
		const float xx = x + 0.5f + level->random.nextInt(5) - 2;
		const float yy = (float) y + level->random.nextInt(3) - 1;
		const float zz = z + 0.5f + level->random.nextInt(5) - 2;
		if (!level->allWaterAt(Mth::floor(xx), Mth::floor(yy), Mth::floor(zz))) continue;
		Mob* mob = MobFactory::CreateMob(type, level);
		if (!mob) continue;
		if (addMob(level, mob, xx, yy, zz, level->random.nextFloat() * 360, 0, false)) ++made;
		else delete mob;
	}
	return made;
}
