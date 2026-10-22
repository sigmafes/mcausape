#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TurtleRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TurtleRenderer_H__

#include "MobRenderer.h"
#include "../../model/TurtleModel.h"

class TurtleRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
	TurtleRenderer() : super(new TurtleModel(), 0.7f) {}
protected:
	float getBob(Mob* mob, float a) { return mob->tickCount + a; }
};

#endif
