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
	// Any spot will do: the spawn is set to the fixed portal (Level::validateSpawn).
	virtual bool isValidSpawn(int x, int z) { return true; }

	/// The fixed portal, in the middle near the top: its four cells are
	/// HubX-1..HubX, HubZ-1..HubZ at HubY.
	/// 256/120/256 until somebody breaks it in creative and lights another
	/// in the Aquarium: that one is the way back from then on (loadHub/saveHub,
	/// aquarium_hub.txt in the world's folder).
	static int HubX;
	static int HubZ;
	static int HubY;
	static bool hubMoved;
	static void loadHub(Level* level);
	static void moveHub(Level* level, int x, int y, int z);
};

#endif
