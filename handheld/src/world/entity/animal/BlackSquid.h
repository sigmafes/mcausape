#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__BlackSquid_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__BlackSquid_H__

#include "Squid.h"

/** The black squid of the Aquarium's deep: it swims at the nearest player,
    and on touching them does no harm but splashes ink over their view
    (the Ink effect, 10 seconds, drawn by Gui::renderInk), then swims off
    for those 10 seconds before coming back. */
class BlackSquid: public Squid
{
	typedef Squid super;
public:
	BlackSquid(Level* level);
	int getEntityTypeId() const;
	void aiStep();
protected:
	void updateAi();
private:
	int fleeTicks;
};

#endif
