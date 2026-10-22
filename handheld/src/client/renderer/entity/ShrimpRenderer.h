#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ShrimpRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ShrimpRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../../model/ShrimpModel.h"
#include "../../../world/entity/animal/Shrimp.h"

// The shrimp lies on its side out of water, as the fish do.
class ShrimpRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
	ShrimpRenderer() : super(new ShrimpModel(), 0.2f) {}

protected:
	void setupRotations(Entity* mob, float bob, float bodyRot, float a) {
		super::setupRotations(mob, bob, bodyRot, a);
		if (!((Shrimp*) mob)->swimming()) {
			glTranslatef2(0.1f, 0.05f, 0);
			glRotatef2(90, 0, 0, 1);
		}
	}
	float getBob(Mob* mob, float a) { return mob->tickCount + a; }
	// Half the model's size: a shrimp, not a lobster.
	void scale(Mob* mob, float a) { glScalef2(0.5f, 0.5f, 0.5f); }
};

#endif
