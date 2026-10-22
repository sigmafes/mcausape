#ifndef NET_MINECRAFT_CLIENT_MODEL__TurtleModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__TurtleModel_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"

// Java's TurtleModel (1.13) on its 128x64 texture: head, the shell (the
// body box laid flat), two back legs and two long front flippers that row.
class TurtleModel: public Model
{
	typedef Model super;
	static void part(Model* m, ModelPart& p, int u, int v, float x0, float y0, float z0,
			int w, int h, int d, float px, float py, float pz) {
		p = ModelPart(u, v);
		p.setModel(m);
		p.addBox(x0, y0, z0, w, h, d, 0.0f);
		p.setPos(px, py, pz);
	}
public:
	ModelPart head, body, leg0, leg1, leg2, leg3;

	TurtleModel() {
		texWidth = 128; texHeight = 64;
		part(this, head, 3, 0, -3, -1, -3, 6, 5, 6, 0, 19, -10);
		part(this, body, 7, 37, -9.5f, 3, -10, 19, 20, 6, 0, 11, -10);
		body.xRot = Mth::PI / 2;
		part(this, leg0, 1, 23, -2, 0, 0, 4, 1, 10, -3.5f, 22, 11);
		part(this, leg1, 1, 12, -2, 0, 0, 4, 1, 10, 3.5f, 22, 11);
		part(this, leg2, 27, 30, -13, 0, -2, 13, 1, 5, -5, 21, -4);
		part(this, leg3, 27, 24, 0, 0, -2, 13, 1, 5, 5, 21, -4);
	}

	void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale) {
		// Java's swimming stroke, always on: the Aquarium is all water.
		const float s = bob * 0.6662f * 0.3f;
		leg0.xRot = Mth::cos(s) * 0.5f;
		leg1.xRot = Mth::cos(s + Mth::PI) * 0.5f;
		leg2.zRot = Mth::cos(s + Mth::PI) * 0.5f;
		leg3.zRot = Mth::cos(s) * 0.5f;
		head.render(scale); body.render(scale);
		leg0.render(scale); leg1.render(scale); leg2.render(scale); leg3.render(scale);
	}
};

#endif
