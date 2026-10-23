#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__AxolotlRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__AxolotlRenderer_H__

#include "MobRenderer.h"
#include "../../model/AxolotlModel.h"

class AxolotlRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
	AxolotlRenderer() : super(new AxolotlModel(), 0.4f) {}
protected:
	float getBob(Mob* mob, float a) { return mob->tickCount + a; }
};

#endif
