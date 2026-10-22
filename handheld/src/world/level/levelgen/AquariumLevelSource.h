#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__AquariumLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__AquariumLevelSource_H__

#include <map>
#include "../chunk/ChunkSource.h"

class Level;
class PerlinNoise;
class Random;
class LevelChunk;

/** The Aquarium's terrain: bedrock at y=0, sand above it about four deep with
    a gentle swell, a vast open cave of water, then a thick shelf of
    prismarine hanging in the middle of the world -- rolling hills on top
    like the Overworld's ground, a ragged underside with drips -- and water
    again up to a barrier of invisible bedrock at the top. The fixed portal is built when the middle
    chunks are made (AquariumLevelSource::buildHub). */
class AquariumLevelSource: public ChunkSource
{
public:
	AquariumLevelSource(Level* level, long seed);
	~AquariumLevelSource();

	bool hasChunk(int x, int z) { return true; }
	LevelChunk* create(int x, int z) { return getChunk(x, z); }
	LevelChunk* getChunk(int x, int z);
	void postProcess(ChunkSource* parent, int x, int z);
	bool tick() { return false; }
	bool shouldSave() { return true; }
	Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) { return Biome::MobList(); }
	std::string gatherStats() { return "AquariumLevelSource"; }

	/// Top of the sand at a column (its height is the floor's swell).
	int sandTop(int x, int z) const;
	/// The fixed portal: the frame and portal, floating at the top.
	static void buildHub(Level* level);
	/// Where an arrival stands: on top of one of the frame's 8 blocks (`pick` chooses).
	static void hubArrival(int pick, int* x, int* y, int* z);

	static const int CeilingY = 127;

private:
	Level* level;
	long seed;
	// The prismarine shelf: its rolling top, its underside, and the drips
	// hanging off that.
	Random* random;
	PerlinNoise* topNoise;
	PerlinNoise* bottomNoise;
	PerlinNoise* dripNoise;
	// Sand patches on the shelf, and where the reefs are thick or thin.
	PerlinNoise* sandNoise;
	PerlinNoise* reefNoise;
	/// Coral reefs on the shelf of chunk (cx, cz), Java-style.
	void decorate(int cx, int cz);
	void placeCoral(int x, int y, int z, int tile);
	void coralMushroom(Random& r, int x, int y, int z, int tile);
	void coralTree(Random& r, int x, int y, int z, int tile);
	void coralClaw(Random& r, int x, int y, int z, int tile);
	/// Where the shelf is in a column: [bottom, top], empty if bottom > top.
	void shelfAt(int x, int z, int* bottom, int* top) const;
	std::map<int, LevelChunk*> chunks;
};

#endif
