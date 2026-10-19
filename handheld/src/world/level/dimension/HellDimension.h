#ifndef NET_MINECRAFT_WORLD_LEVEL_DIMENSION__HellDimension_H__
#define NET_MINECRAFT_WORLD_LEVEL_DIMENSION__HellDimension_H__

//package net.minecraft.world.level.dimension;

#include "Dimension.h"

class HellDimension: public Dimension {
public:
	HellDimension();

	virtual void init();
	virtual ChunkSource* createRandomLevelSource();

	virtual float getTimeOfDay(long time, float a);
	virtual float* getSunriseColor(float td, float a);
	virtual Vec3 getFogColor(float td, float a);

	virtual bool mayRespawn();

protected:
	virtual void updateLightRamp();
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_DIMENSION__HellDimension_H__*/
