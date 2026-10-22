#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__AquariumLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__AquariumLevelSource_H__

#include <map>
#include "../chunk/ChunkSource.h"

class Level;
class LevelChunk;

/** The Aquarium's terrain: bedrock at y=0, sand above it about four deep with
    a gentle swell, then nothing (which is water there) up to a barrier of
    invisible bedrock at the top. The fixed portal is built when the middle
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
	/// The fixed portal: a prismarine platform with the frame and portal on it.
	static void buildHub(Level* level);

	static const int CeilingY = 127;

private:
	Level* level;
	long seed;
	std::map<int, LevelChunk*> chunks;
};

#endif
