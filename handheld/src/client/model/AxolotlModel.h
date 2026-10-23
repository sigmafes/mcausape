#ifndef NET_MINECRAFT_CLIENT_MODEL__AxolotlModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__AxolotlModel_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"

// Java's AxolotlModel (1.17) on its 64x64 texture. Java hangs the head,
// gills, legs and tail on the body; here each part is placed where that
// puts it, the body itself never turning. Swimming: the tail sweeps, the
// legs fold back and paddle, the gills flutter.
class AxolotlModel: public Model
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
	ModelPart body, fin, head, topGills, leftGills, rightGills, legRH, legLH, legRF, legLF, tail;

	AxolotlModel() {
		texWidth = texHeight = 64;
		part(this, body, 0, 11, -4, -2, -9, 8, 4, 10, 0, 20, 5);
		part(this, fin, 2, 17, 0, -3, -8, 0, 5, 9, 0, 20, 5);
		part(this, head, 0, 1, -4, -3, -5, 8, 5, 5, 0, 20, -4);
		part(this, topGills, 3, 37, -4, -3, 0, 8, 3, 0, 0, 17, -5);
		part(this, leftGills, 0, 40, -3, -5, 0, 3, 7, 0, -4, 20, -5);
		part(this, rightGills, 11, 40, 0, -5, 0, 3, 7, 0, 4, 20, -5);
		part(this, legRH, 2, 13, -1, 0, 0, 3, 5, 0, -3.5f, 21, 4);
		part(this, legLH, 2, 13, -2, 0, 0, 3, 5, 0, 3.5f, 21, 4);
		part(this, legRF, 2, 13, -2, 0, 0, 3, 5, 0, -3.5f, 21, -3);
		part(this, legLF, 2, 13, -1, 0, 0, 3, 5, 0, 3.5f, 21, -3);
		part(this, tail, 2, 19, 0, -3, 0, 0, 5, 12, 0, 20, 6);
	}

	void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale) {
		const float s = bob * 0.33f;
		tail.yRot = 0.5f * Mth::sin(s);
		const float paddle = 0.35f * Mth::sin(s * 0.8f);
		legRH.xRot = legLH.xRot = 1.88f + paddle;
		legRF.xRot = legLF.xRot = 1.88f - paddle;
		legRF.zRot = legRH.zRot = -0.1f;
		legLF.zRot = legLH.zRot = 0.1f;
		topGills.xRot = 0.2f * Mth::sin(bob * 0.15f) - 0.1f;
		leftGills.yRot = -0.3f + 0.15f * Mth::sin(bob * 0.2f);
		rightGills.yRot = 0.3f - 0.15f * Mth::sin(bob * 0.2f);
		body.render(scale); fin.render(scale); head.render(scale);
		topGills.render(scale); leftGills.render(scale); rightGills.render(scale);
		legRH.render(scale); legLH.render(scale); legRF.render(scale); legLF.render(scale);
		tail.render(scale);
	}
};

#endif
