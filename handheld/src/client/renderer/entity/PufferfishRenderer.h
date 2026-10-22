#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PufferfishRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PufferfishRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../../model/PufferfishModels.h"
#include "../../../world/entity/animal/Pufferfish.h"
#include "../../../util/Mth.h"

// Java's PufferfishRenderer: the model for its puff, bobbing gently, on its
// side out of water.
class PufferfishRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
	PufferfishRenderer()
	:	super(new PufferfishSmallModel(), 0.2f),
		_small(getModel()), _mid(new PufferfishMidModel()), _big(new PufferfishBigModel())
	{}

	void render(Entity* e, float x, float y, float z, float rot, float a) {
		const int puff = ((Pufferfish*) e)->puffState;
		setModel(puff == 0 ? _small : (puff == 1 ? _mid : _big));
		shadowRadius = 0.1f + 0.1f * puff;
		super::render(e, x, y, z, rot, a);
	}

protected:
	void setupRotations(Entity* mob, float bob, float bodyRot, float a) {
		glTranslatef2(0, Mth::cos(bob * 0.05f) * 0.08f, 0);
		super::setupRotations(mob, bob, bodyRot, a);
		if (!((Pufferfish*) mob)->swimming()) {
			glTranslatef2(0, 0.1f, 0);
			glRotatef2(90, 0, 0, 1);
		}
	}
	float getBob(Mob* mob, float a) { return mob->tickCount + a; }

private:
	Model* _small;
	Model* _mid;
	Model* _big;
};

#endif
