#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__AquaticBlaze_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__AquaticBlaze_H__

#include "Blaze.h"

/** The Aquarium's blaze: purple, grown over with coral, no smoke, rods
    turning at half speed, and instead of fireballs it fires water bolts
    (WaterBolt) that keep their speed under water and trail bubbles. */
class AquaticBlaze: public Blaze
{
	typedef Blaze super;
public:
	AquaticBlaze(Level* level);
	int getEntityTypeId() const;
	void aiStep();
protected:
	void checkHurtTarget(Entity* target, float d);
private:
	int shots;
	int shotCooldown;
};

#endif
