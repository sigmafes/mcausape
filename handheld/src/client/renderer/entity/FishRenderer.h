#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__FishRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__FishRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../../model/FishModels.h"
#include "../../../world/entity/animal/Fish.h"
#include "../../../util/Mth.h"

// Java's CodRenderer / SalmonRenderer: the whole fish sways side to side as
// it swims, and lies on its side out of water.
class FishRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
	FishRenderer(bool salmon)
	:	super(salmon ? (Model*) new SalmonModel() : (Model*) new CodModel(), salmon ? 0.4f : 0.3f),
		_salmon(salmon)
	{}

protected:
	void setupRotations(Entity* mob, float bob, float bodyRot, float a) {
		super::setupRotations(mob, bob, bodyRot, a);
		const float f = (_salmon && !((Fish*) mob)->swimming()) ? 1.3f : 1.0f;
		glRotatef2(f * 4.3f * Mth::sin(0.6f * bob), 0, 1, 0);
		if (_salmon) glTranslatef2(0, 0, -0.4f);
		if (!((Fish*) mob)->swimming()) {
			glTranslatef2(_salmon ? 0.2f : 0.1f, 0.1f, _salmon ? 0 : -0.1f);
			glRotatef2(90, 0, 0, 1);
		}
	}

	float getBob(Mob* mob, float a) { return mob->tickCount + a; }

private:
	bool _salmon;
};

#endif
