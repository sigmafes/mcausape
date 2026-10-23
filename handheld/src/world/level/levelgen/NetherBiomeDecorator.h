#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__NetherBiomeDecorator_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__NetherBiomeDecorator_H__

class Level;
class Random;

/** What Java 1.16's Nether biomes grow, for worlds that have them
    (HellRandomLevelSource::postProcess, generator version 1): the forests'
    huge fungi, plants and vines, the deltas' basalt columns and magma-edged
    lava pools, the valley's soul fire and fossils, and the ores every biome
    has (nether gold, ancient debris). Placed from the chunk's middle (+8)
    like any decoration, so it can reach into neighbours already made. */
namespace NetherBiomeDecorator
{
	void decorate(Level* level, Random* random, int xo, int zo, long seed);
}

#endif
