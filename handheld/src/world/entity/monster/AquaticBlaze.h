#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__AquaticBlaze_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__AquaticBlaze_H__

#include "Blaze.h"

/** The Aquarium's blaze: purple, grown over with coral, giving off bubbles,
    rods turning at half speed. It hunts the nearest player within 48 blocks:
    level with them and following across about six blocks off, firing the
    blaze's three-shot volleys as water bolts (WaterBolt) from within 32. */
class AquaticBlaze: public Blaze
{
	typedef Blaze super;
public:
	AquaticBlaze(Level* level);
	int getEntityTypeId() const;
	void aiStep();
	void travel(float xa, float ya);
	bool canBreatheUnderwater() { return true; }
protected:
	void updateAi();
	void checkHurtTarget(Entity* target, float d) {}
private:
	int shots;
};

#endif
