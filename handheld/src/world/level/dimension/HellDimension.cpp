#include "HellDimension.h"
#include "../Level.h"
#include "../levelgen/HellRandomLevelSource.h"
#include "../biome/HellBiomeSource.h"

// El Nether. Valores tomados de la referencia MCPE 1.0.0 (HellDimension)
// adaptados al motor 0.6.1:
//  - foggy/ultraWarm/hasCeiling
//  - rampa de brillo con luz ambiente 0.10
//  - tiempo fijo (sin ciclo de dia/noche), niebla rojo oscuro

HellDimension::HellDimension()
{
	id = Dimension::NETHER;
	foggy = true;
	ultraWarm = true;
	hasCeiling = true;
}

void HellDimension::init()
{
	// Todo el Nether es un unico bioma (como la FixedBiomeSource de 1.0)
	biomeSource = new HellBiomeSource(level);
}

ChunkSource* HellDimension::createRandomLevelSource()
{
	// Generador del Nether (netherrack, mar de lava, soul sand, cuevas,
	// glowstone y manantiales de lava). Ver HellRandomLevelSource.
	return new HellRandomLevelSource(
		level,
		level->getSeed(),
		level->getLevelData()->getGeneratorVersion(),
		!level->isClientSide);
}

float HellDimension::getTimeOfDay(long time, float a)
{
	return 0.5f;
}

float* HellDimension::getSunriseColor(float td, float a)
{
	return NULL;
}

Vec3 HellDimension::getFogColor(float td, float a)
{
	// Color de niebla del Nether (1.0: 0.2, 0.03, 0.03)
	return Vec3(0.2f, 0.03f, 0.03f);
}

bool HellDimension::mayRespawn()
{
	return false;
}

void HellDimension::updateLightRamp()
{
	float ambientLight = 0.25f;
	for (int i = 0; i <= 15; i++) {
		float v = (1 - i / (float) (16));
		brightnessRamp[i] = ((1 - v) / (v * 3 + 1)) * (1 - ambientLight) + ambientLight;
	}
}
