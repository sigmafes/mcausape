#ifndef NET_MINECRAFT_WORLD_LEVEL_BIOME__NetherBiomes_H__
#define NET_MINECRAFT_WORLD_LEVEL_BIOME__NetherBiomes_H__

/** The Nether's five biomes (Java 1.16), for worlds made since they were
    added (LevelData's generator version 1). Not Biome objects: what they
    decide -- the surface, the fog, later the features and mobs -- asks
    NetherBiomes::at, which is worked out from the seed alone, so a visitor
    gets the same answer as the host without being sent anything.

    Like Java's multi-noise source, two noise fields (temperature, humidity)
    pick the nearest of five points; scaled for a 256-block Nether, where
    Java's biomes would cover it whole. */
namespace NetherBiomes
{
	enum Id { Wastes = 0, SoulSandValley, CrimsonForest, WarpedForest, BasaltDeltas, Count };

	int at(long seed, int x, int z);
	/// Java's fog colour for the biome, 0xRRGGBB.
	int fogColor(int biome);
	const char* name(int biome);
}

#endif
