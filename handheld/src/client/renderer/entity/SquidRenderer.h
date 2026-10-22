#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__SquidRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__SquidRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../../model/SquidModel.h"
#include "../../../world/entity/animal/Squid.h"

// The Console Edition's SquidRenderer: the body tilts and spins with the
// squid's swimming, and the tentacles open and close with its stroke.
class SquidRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
	SquidRenderer()
	:	super(new SquidModel(), 0.7f)
	{}

protected:
	void setupRotations(Entity* mob_, float bob, float bodyRot, float a) {
		Squid* mob = (Squid*) mob_;
		const float bodyXRot = mob->xBodyRotO + (mob->xBodyRot - mob->xBodyRotO) * a;
		const float bodyZRot = mob->zBodyRotO + (mob->zBodyRot - mob->zBodyRotO) * a;
		glTranslatef2(0, 0.5f, 0);
		glRotatef2(180 - bodyRot, 0, 1, 0);
		glRotatef2(bodyXRot, 1, 0, 0);
		glRotatef2(bodyZRot, 0, 1, 0);
		glTranslatef2(0, -1.2f, 0);
	}

	float getBob(Mob* mob_, float a) {
		Squid* mob = (Squid*) mob_;
		return mob->oldTentacleAngle + (mob->tentacleAngle - mob->oldTentacleAngle) * a;
	}
};

#endif
