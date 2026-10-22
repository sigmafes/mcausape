#ifndef NET_MINECRAFT_WORLD_LEVEL_DIMENSION__AquariumDimension_H__
#define NET_MINECRAFT_WORLD_LEVEL_DIMENSION__AquariumDimension_H__

#include "Dimension.h"

/** The Aquarium (docs/aquarium-plan.md). A world of water held as air: the
    dimension says its air is water (isAllWater), so swimming, drowning, the
    underwater view and putting out fire all follow, with nothing to flow or
    draw. Bedrock and a sand floor below, a barrier at the top, and one fixed
    portal in the middle. Light fades with depth (LevelChunk::getRawBrightness). */
class AquariumDimension: public Dimension {
public:
	AquariumDimension();

	virtual void init();
	virtual ChunkSource* createRandomLevelSource();

	virtual float getTimeOfDay(long time, float a);
	virtual float* getSunriseColor(float td, float a);
	virtual Vec3 getFogColor(float td, float a);
	virtual bool mayRespawn();
	virtual bool isAllWater() { return true; }

	/// The fixed portal, in the middle near the top: its four cells are
	/// HubX-1..HubX, HubZ-1..HubZ at HubY.
	static const int HubX = 256;
	static const int HubZ = 256;
	static const int HubY = 120;
};

#endif
