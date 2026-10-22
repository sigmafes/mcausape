#include "AquariumDimension.h"
#include "../Level.h"
#include "../levelgen/AquariumLevelSource.h"
#include "../biome/HellBiomeSource.h"
#include "../storage/LevelStorage.h"
#include <fstream>

int AquariumDimension::HubX = 256;
int AquariumDimension::HubZ = 256;
int AquariumDimension::HubY = 120;
bool AquariumDimension::hubMoved = false;

static std::string hubFile(Level* level)
{
	LevelStorage* storage = level ? level->getLevelStorage() : NULL;
	return storage ? storage->getFolder() + "/aquarium_hub.txt" : std::string();
}

void AquariumDimension::loadHub(Level* level)
{
	static std::string loaded;
	const std::string file = hubFile(level);
	if (file.empty() || file == loaded) return;
	loaded = file;
	HubX = 256; HubY = 120; HubZ = 256; hubMoved = false;
	std::ifstream in(file.c_str());
	int x, y, z;
	if (in >> x >> y >> z) { HubX = x; HubY = y; HubZ = z; hubMoved = true; }
}

void AquariumDimension::moveHub(Level* level, int x, int y, int z)
{
	HubX = x; HubY = y; HubZ = z; hubMoved = true;
	const std::string file = hubFile(level);
	if (file.empty()) return;
	std::ofstream out(file.c_str(), std::ios::trunc);
	out << x << " " << y << " " << z << "\n";
}

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
