#ifndef NET_MINECRAFT_CLIENT_MODEL__SquidModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__SquidModel_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"

// The squid, ported from the Console Edition's SquidModel: a 12x16x12 body
// with eight tentacles in a ring under it. The tentacles' angle is the "bob"
// SquidRenderer hands in (the squid's own tentacle stroke).
class SquidModel: public Model
{
	typedef Model super;
public:
	static const int TENTACLES = 8;

	ModelPart body;
	ModelPart tentacles[TENTACLES];

	SquidModel()
	:	body(0, 0)
	{
		const int yoffs = -16;
		body.setModel(this);
		body.addBox(-6, -8, -6, 12, 16, 12, 0.0f);
		body.y += (8 + 16) + yoffs;

		for (int i = 0; i < TENTACLES; ++i) {
			tentacles[i] = ModelPart(48, 0);
			tentacles[i].setModel(this);
			float angle = i * Mth::PI * 2.0f / TENTACLES;
			tentacles[i].addBox(-1, 0, -1, 2, 18, 2, 0.0f);
			tentacles[i].x = Mth::cos(angle) * 5;
			tentacles[i].z = Mth::sin(angle) * 5;
			tentacles[i].y = (float) (31 + yoffs);
			angle = i * Mth::PI * -2.0f / TENTACLES + Mth::PI * 0.5f;
			tentacles[i].yRot = angle;
		}
	}

	/*@Override*/
	void render(Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale) {
		for (int i = 0; i < TENTACLES; ++i)
			tentacles[i].xRot = bob;
		body.render(scale);
		for (int i = 0; i < TENTACLES; ++i)
			tentacles[i].render(scale);
	}
};

#endif
