#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__HellRandomLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__HellRandomLevelSource_H__

//package net.minecraft.world.level.levelgen;

class Biome;
class Level;
class LevelChunk;

#if 1 || USE_MAP
	#include <map>
	typedef std::map<int, LevelChunk*> ChunkMap;
#else
	#if defined(__APPLE__)
		#include <ext/hash_map>
		namespace std {
			using namespace __gnu_cxx;
		}
	#else
		#include <hash_map>
	#endif
	typedef std::hash_map<int, LevelChunk*> ChunkMap;
#endif


#include "../chunk/ChunkSource.h"
#include "LargeHellCaveFeature.h"
#include "synth/PerlinNoise.h"
#include "../../../SharedConstants.h"

// Generador del Nether. Espejo de RandomLevelSource adaptado de la version
// de MCPE 1.0.0 (HellRandomLevelSource):
//   - sin agua ni biomas: netherrack / lava / soul sand / gravel
//   - mar de lava por debajo de y=32
//   - techo y suelo de bedrock
//   - cuevas (LargeHellCaveFeature), glowstone y manantiales de lava

#include "structure/NetherFortress.h"

class HellRandomLevelSource: public ChunkSource
{
public:
    static const int CHUNK_HEIGHT = 8;
    static const int CHUNK_WIDTH = 4;
    static const int SEA_LEVEL = 32;

	HellRandomLevelSource(Level* level, long seed, int version, bool spawnMobs);
	~HellRandomLevelSource();

	bool hasChunk(int x, int y);
    LevelChunk* create(int x, int z);
	LevelChunk* getChunk(int xOffs, int zOffs);

	void prepareHeights(int xOffs, int zOffs, unsigned char* blocks);
    void buildSurfaces(int xOffs, int zOffs, unsigned char* blocks);
	void postProcess(ChunkSource* parent, int xt, int zt);

    bool tick();

    Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z);

	bool shouldSave();
    std::string gatherStats();

private:
	float* getHeights(float* buffer, int x, int y, int z, int xSize, int ySize, int zSize);

public:
	LargeHellCaveFeature caveFeature;
private:
	ChunkMap chunkMap;

	Random random;
	PerlinNoise lperlinNoise1;
    PerlinNoise lperlinNoise2;
    PerlinNoise perlinNoise1;
    PerlinNoise perlinNoise2;
    PerlinNoise perlinNoise3;
	PerlinNoise scaleNoise;
    PerlinNoise depthNoise;

	Level* level;
	bool spawnMobs;
	// The world's one fortress (see NetherFortress).
	NetherFortress fortress;

	float* buffer;
    float sandBuffer[16 * 16];
    float gravelBuffer[16 * 16];
    float depthBuffer[16 * 16];
	float* pnr;
	float* ar;
	float* br;
	long seed;
	/// LevelData's generator version: 1 and up has NetherBiomes.
	int version;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__HellRandomLevelSource_H__*/
