#include "AquariumDimension.h"
#include "../Level.h"
#include "../levelgen/AquariumLevelSource.h"
#include "../biome/HellBiomeSource.h"

AquariumDimension::AquariumDimension()
{
	id = Dimension::AQUARIUM;
	foggy = true;
	ultraWarm = false;
	hasCeiling = false;
}

void AquariumDimension::init()
{
	// One biome for everything; nothing here asks it for more than that.
	biomeSource = new HellBiomeSource(level);
}

ChunkSource* AquariumDimension::createRandomLevelSource()
{
	return new AquariumLevelSource(level, level->getSeed());
}

float AquariumDimension::getTimeOfDay(long time, float a)
{
	// Always noon: the light is the water's, not the sun's.
	return 0.0f;
}

float* AquariumDimension::getSunriseColor(float td, float a)
{
	return NULL;
}

Vec3 AquariumDimension::getFogColor(float td, float a)
{
	return Vec3(0.05f, 0.35f, 0.42f);
}

bool AquariumDimension::mayRespawn()
{
	return false;
}
