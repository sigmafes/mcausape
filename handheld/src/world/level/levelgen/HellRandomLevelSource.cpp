#include "HellRandomLevelSource.h"

#include "feature/FeatureInclude.h"
#include "feature/GlowStoneFeature.h"
#include "feature/HellSpringFeature.h"
#include "feature/OreFeature.h"
#include "feature/HellFireFeature.h"
#include "../../entity/MobCategory.h"
#include "../Level.h"
#include "../ChunkPos.h"
#include "../biome/Biome.h"
#include "../biome/BiomeSource.h"
#include "../chunk/LevelChunk.h"
#include "../material/Material.h"
#include "../tile/Tile.h"
#include "../tile/HeavyTile.h"
#include "../../../util/Mth.h"
#include "../../../util/Random.h"

static const int MAX_BUFFER_SIZE = 1024;

HellRandomLevelSource::HellRandomLevelSource(Level* level, long seed, int version, bool spawnMobs)
:	random(seed),
	level(level),
	lperlinNoise1(&random, 16),
	lperlinNoise2(&random, 16),
	perlinNoise1(&random, 8),
	perlinNoise2(&random, 4),
	perlinNoise3(&random, 4),
	scaleNoise(&random, 10),
	depthNoise(&random, 16),
	spawnMobs(spawnMobs),
	fortress(seed),
	pnr(NULL), ar(NULL), br(NULL)
{
	buffer = new float[MAX_BUFFER_SIZE];
}

HellRandomLevelSource::~HellRandomLevelSource() {
	delete[] buffer;
	delete[] pnr;
	delete[] ar;
	delete[] br;
}

/*public*/
void HellRandomLevelSource::prepareHeights(int xOffs, int zOffs, unsigned char* blocks) {

	int xChunks = 16 / CHUNK_WIDTH;

    int xSize = xChunks + 1;
    int ySize = 128 / CHUNK_HEIGHT + 1;
    int zSize = xChunks + 1;
    buffer = getHeights(buffer, xOffs * xChunks, 0, zOffs * xChunks, xSize, ySize, zSize);

    for (int xc = 0; xc < xChunks; xc++) {
        for (int zc = 0; zc < xChunks; zc++) {
            for (int yc = 0; yc < 128 / CHUNK_HEIGHT; yc++) {
                float yStep = 1 / (float) CHUNK_HEIGHT;
                float s0 = buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s1 = buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 0)];
                float s2 = buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s3 = buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 0)];

                float s0a = (buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 1)] - s0) * yStep;
                float s1a = (buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 1)] - s1) * yStep;
                float s2a = (buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 1)] - s2) * yStep;
                float s3a = (buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 1)] - s3) * yStep;

                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    float xStep = 1 / (float) CHUNK_WIDTH;

                    float _s0 = s0;
                    float _s1 = s1;
                    float _s0a = (s2 - s0) * xStep;
                    float _s1a = (s3 - s1) * xStep;

                    for (int x = 0; x < CHUNK_WIDTH; x++) {
                        int offs = (x + xc * CHUNK_WIDTH) << 11 | (0 + zc * CHUNK_WIDTH) << 7 | (yc * CHUNK_HEIGHT + y);
                        int step = 1 << 7;
                        float zStep = 1 / (float) CHUNK_WIDTH;

                        float val = _s0;
                        float vala = (_s1 - _s0) * zStep;
                        for (int z = 0; z < CHUNK_WIDTH; z++) {
                            int tileId = 0;
                            if (yc * CHUNK_HEIGHT + y < SEA_LEVEL) {
                                tileId = Tile::calmLava->id;
                            }
                            if (val > 0) {
                                tileId = Tile::netherrack->id;
                            }

                            blocks[offs] = (unsigned char) tileId;
                            offs += step;
                            val += vala;
                        }
                        _s0 += _s0a;
                        _s1 += _s1a;
                    }

                    s0 += s0a;
                    s1 += s1a;
                    s2 += s2a;
                    s3 += s3a;
                }
            }
        }
    }
}

/*public*/
void HellRandomLevelSource::buildSurfaces(int xOffs, int zOffs, unsigned char* blocks) {
    int waterHeight = Level::DEPTH - 64;

    float s = 1 / 32.0f;
    perlinNoise2.getRegion(sandBuffer, (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s, s, 1);
    perlinNoise2.getRegion(gravelBuffer, (float)(xOffs * 16), 109.01340f, (float)(zOffs * 16), 16, 1, 16, s, 1, s);
    perlinNoise3.getRegion(depthBuffer, (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s * 2, s * 2, s * 2);

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            bool sand = (sandBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 0;
            bool gravel = (gravelBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 0;
            int runDepth = (int) (depthBuffer[x + z * 16] / 3 + 3 + random.nextFloat() * 0.25f);

            int run = -1;

            int top = Tile::netherrack->id;
            int material = Tile::netherrack->id;

            for (int y = 127; y >= 0; y--) {
                int offs = (x << 11) | (z << 7) | y;

                int randUpperHeight = 127 - random.nextInt(5);
                int randLowerHeight = random.nextInt(5);
                if (y >= randUpperHeight || y <= randLowerHeight) {
                    blocks[offs] = (unsigned char) Tile::unbreakable->id;
                } else {
                    int old = blocks[offs];

                    if (old == 0) {
                        run = -1;
                    } else if (old == Tile::netherrack->id) {
                        if (run == -1) {
                            if (runDepth <= 0) {
                                top = 0;
                                material = Tile::netherrack->id;
                            } else if (y >= waterHeight - 4 && y <= waterHeight + 1) {
                                top = Tile::netherrack->id;
                                material = Tile::netherrack->id;

                                if (gravel) {
                                    top = Tile::gravel->id;
                                }
                                if (sand) {
                                    top = Tile::hellSand->id;
                                    material = Tile::hellSand->id;
                                }
                            }

                            if (y < waterHeight && top == 0) {
                                top = Tile::calmLava->id;
                            }

                            run = runDepth;
                            if (y >= waterHeight - 1) blocks[offs] = (unsigned char) top;
                            else blocks[offs] = (unsigned char) material;
                        } else if (run > 0) {
                            run--;
                            blocks[offs] = (unsigned char) material;
                        }
                    }
                }
            }
        }
    }
}

/*public*/
void HellRandomLevelSource::postProcess(ChunkSource* parent, int xt, int zt) {

	level->isGeneratingTerrain = true;

    HeavyTile::instaFall = true;
    int xo = xt * 16;
    int zo = zt * 16;

    random.setSeed(level->getSeed());
    int xScale = random.nextInt() / 2 * 2 + 1;
    int zScale = random.nextInt() / 2 * 2 + 1;
    random.setSeed(((xt * xScale) + (zt * zScale)) ^ level->getSeed());

    // la fortaleza: solo los bloques que caen en este chunk
    fortress.postProcess(level, &random, xt, zt);

    // manantiales de lava (MCPE 1.0: HellSpringFeature, 8 intentos)
    for (int i = 0; i < 8; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128 - 8) + 4;
        int z = zo + random.nextInt(16) + 8;
        HellSpringFeature feature(Tile::lava->id);
        feature.place(level, &random, x, y, z);
    }

    // fuego sobre la netherrack (HellFireFeature de la Console Edition)
    {
        int fires = random.nextInt(random.nextInt(10) + 1) + 1;
        for (int i = 0; i < fires; i++) {
            int x = xo + random.nextInt(16) + 8;
            int y = random.nextInt(128 - 8) + 4;
            int z = zo + random.nextInt(16) + 8;
            HellFireFeature feature;
            feature.place(level, &random, x, y, z);
        }
    }

    // glowstone colgando del techo / en las paredes (2 pases como en 1.0)
    int randc = random.nextInt(10) + 1;
    int count = random.nextInt(randc);
    for (int i = 0; i < count; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128 - 8) + 4;
        int z = zo + random.nextInt(16) + 8;
        GlowStoneFeature feature;
        feature.place(level, &random, x, y, z);
    }
    for (int i = 0; i < 10; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        GlowStoneFeature feature;
        feature.place(level, &random, x, y, z);
    }

    // mena de cuarzo: 16 vetas de 14 bloques por chunk dentro de la netherrack
    // (HellRandomLevelSource de la Console Edition)
    for (int i = 0; i < 16; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(128 - 20) + 10;
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::quartzOre->id, 14, Tile::netherrack->id);
        feature.place(level, &random, x, y, z);
    }

    // setas (sobre netherrack, a oscuras)
    if (random.nextInt(1) == 0) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        FlowerFeature feature(Tile::mushroom1->id);
        feature.place(level, &random, x, y, z);
    }
    if (random.nextInt(1) == 0) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        FlowerFeature feature(Tile::mushroom2->id);
        feature.place(level, &random, x, y, z);
    }

    HeavyTile::instaFall = false;

	level->isGeneratingTerrain = false;
}

LevelChunk* HellRandomLevelSource::create(int x, int z) {
    return getChunk(x, z);
}

LevelChunk* HellRandomLevelSource::getChunk(int xOffs, int zOffs) {
	int hashedPos = ChunkPos::hashCode(xOffs, zOffs);

	ChunkMap::iterator it = chunkMap.find(hashedPos);
	if (it != chunkMap.end())
		return it->second;

    random.setSeed((long)(xOffs * 341872712l + zOffs * 132899541l)); //@fix

    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
    LevelChunk* levelChunk = new LevelChunk(level, blocks, xOffs, zOffs);
	chunkMap.insert(std::make_pair(hashedPos, levelChunk));

    prepareHeights(xOffs, zOffs, blocks);
    buildSurfaces(xOffs, zOffs, blocks);
    caveFeature.apply(this, level, xOffs, zOffs, blocks, LevelChunk::ChunkBlockCount);

    levelChunk->recalcHeightmap();

    return levelChunk;
}

/*private*/
float* HellRandomLevelSource::getHeights(float* buffer, int x, int y, int z, int xSize, int ySize, int zSize) {
	const int size = xSize * ySize * zSize;
	if (size > MAX_BUFFER_SIZE) {
		LOGI("HellRandomLevelSource::getHeights: TOO LARGE BUFFER REQUESTED: %d (max %d)\n", size, MAX_BUFFER_SIZE);
	}

    float s = 1 * 684.412f;
    float hs = 1 * 684.412f * 3;

    pnr = perlinNoise1.getRegion(pnr, (float)x, (float)y, (float)z, xSize, ySize, zSize, s / 80.0f, hs / 60.0f, s / 80.0f);
    ar  = lperlinNoise1.getRegion(ar, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);
    br  = lperlinNoise2.getRegion(br, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);

    int p = 0;
    float yoffs[32];

    for (int yy = 0; yy < ySize; yy++) {
        yoffs[yy] = cos(yy * Mth::PI * 6 / (float) ySize) * 2;

        float dd = (float) yy;
        if (yy > ySize / 2) {
            dd = (float)(ySize - 1) - yy;
        }
        if (dd < 4) {
            dd = 4 - dd;
            yoffs[yy] -= dd * dd * dd * 10;
        }
    }

    for (int xx = 0; xx < xSize; xx++) {
        for (int zz = 0; zz < zSize; zz++) {
            for (int yy = 0; yy < ySize; yy++) {
                float val = 0;

                float yOffs = yoffs[yy];

                float bb = ar[p] / 512;
                float cc = br[p] / 512;

                float v = (pnr[p] / 10 + 1) / 2;
                if (v < 0) {
                    val = bb;
                } else if (v > 1) {
                    val = cc;
                } else {
                    val = bb + (cc - bb) * v;
                }
                val -= yOffs;

                if (yy > ySize - 4) {
                    float slide = (yy - (ySize - 4)) / (4 - 1.0f);
                    val = val * (1 - slide) + -10 * slide;
                }

                buffer[p] = val;
                p++;
            }
        }
    }
    return buffer;
}

bool HellRandomLevelSource::hasChunk(int x, int y) {
	return true;
}

bool HellRandomLevelSource::tick() {
	return false;
}

bool HellRandomLevelSource::shouldSave() {
	return true;
}

std::string HellRandomLevelSource::gatherStats() {
	return "HellRandomLevelSource";
}

Biome::MobList HellRandomLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
    // Inside the fortress, its own monsters.
    if (&mobCategory == &MobCategory::monster && fortress.isInside(x, y, z))
        return fortress.getEnemies();

    BiomeSource* biomeSource = level->getBiomeSource();
    if (biomeSource == NULL) {
        return Biome::MobList();
    }
    Biome* biome = biomeSource->getBiome(x, z);
    if (biome == NULL) {
        return Biome::MobList();
    }
    return biome->getMobs(mobCategory);
}
