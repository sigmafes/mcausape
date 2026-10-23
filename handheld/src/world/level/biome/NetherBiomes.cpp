#include "NetherBiomes.h"
#include "../levelgen/synth/PerlinNoise.h"
#include "../../../util/Random.h"

namespace NetherBiomes
{

static float unit(float v, int octaves)
{
	v = v / (float) ((1 << octaves) - 1) * 1.7f;
	return v < -1 ? -1 : (v > 1 ? 1 : v);
}

int at(long seed, int x, int z)
{
	static long cachedSeed = 0;
	static bool made = false;
	static Random* random = 0;
	static PerlinNoise* temperature = 0;
	static PerlinNoise* humidity = 0;
	if (!made || seed != cachedSeed) {
		delete temperature; delete humidity; delete random;
		random = new Random(seed ^ 0x6e65746865724cL);
		temperature = new PerlinNoise(random, 3);
		humidity = new PerlinNoise(random, 3);
		cachedSeed = seed;
		made = true;
	}
	const float t = unit(temperature->getValue(x / 40.0f, z / 40.0f), 3);
	const float h = unit(humidity->getValue(x / 40.0f + 1000.0f, z / 40.0f - 1000.0f), 3);
	// Java 1.16's points (temperature, humidity): wastes at the middle.
	static const float points[Count][2] = {
		{ 0.0f, 0.0f },		// nether wastes
		{ 0.0f, -0.5f },	// soul sand valley
		{ 0.4f, 0.0f },		// crimson forest
		{ 0.0f, 0.5f },		// warped forest
		{ -0.5f, 0.0f },	// basalt deltas
	};
	int best = 0;
	float bestD = 1e9f;
	for (int i = 0; i < Count; ++i) {
		const float dt = t - points[i][0], dh = h - points[i][1];
		const float d = dt * dt + dh * dh;
		if (d < bestD) { bestD = d; best = i; }
	}
	return best;
}

int fogColor(int biome)
{
	switch (biome) {
		case SoulSandValley: return 0x1B4745;
		case CrimsonForest:  return 0x330303;
		case WarpedForest:   return 0x1A051A;
		case BasaltDeltas:   return 0x685F70;
		default:             return 0x330808;
	}
}

const char* name(int biome)
{
	switch (biome) {
		case SoulSandValley: return "Soul Sand Valley";
		case CrimsonForest:  return "Crimson Forest";
		case WarpedForest:   return "Warped Forest";
		case BasaltDeltas:   return "Basalt Deltas";
		default:             return "Nether Wastes";
	}
}

}
