#ifndef NET_MINECRAFT_CLIENT_MODEL__ShrimpModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__ShrimpModel_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"

// The shrimp (ours, after the picture it was asked from): a body with the
// head at the front, two black eyes, a tail of two segments and a flat fan
// that curls as it swims, long antennae that sway, and three pairs of legs
// paddling underneath. 32x32 texture (mob/shrimp.png).
class ShrimpModel: public Model
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
	ModelPart body, eyeL, eyeR, abdomen, abdomen2, fan, antennaL, antennaR;
	ModelPart legs[6];

	ShrimpModel() {
		texWidth = texHeight = 32;
		part(this, body, 0, 0, -1.5f, -3, -4, 3, 3, 6, 0, 23, 0);
		part(this, eyeL, 26, 9, 1.0f, -3, -4, 1, 1, 1, 0, 23, 0);
		part(this, eyeR, 26, 9, -2.0f, -3, -4, 1, 1, 1, 0, 23, 0);
		// The tail's three pieces turn about one pivot, where it joins the body.
		part(this, abdomen, 0, 9, -1.5f, -3, 0, 3, 3, 3, 0, 23, 2);
		part(this, abdomen2, 12, 9, -1, -2.5f, 3, 2, 2, 3, 0, 23, 2);
		part(this, fan, 0, 15, -2, -1.5f, 6, 4, 0, 3, 0, 23, 2);
		part(this, antennaL, 14, 18, -0.5f, -0.5f, -8, 1, 1, 8, 0.8f, 21, -4);
		part(this, antennaR, 14, 18, -0.5f, -0.5f, -8, 1, 1, 8, -0.8f, 21, -4);
		for (int i = 0; i < 6; ++i)
			part(this, legs[i], 22, 9, -0.5f, 0, -0.5f, 1, 2, 1, (i & 1) ? 1.0f : -1.0f, 22.5f, -2.5f + (i / 2) * 1.8f);
	}

	void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale) {
		const float curl = 0.25f * Mth::sin(bob * 0.35f);
		abdomen.xRot = abdomen2.xRot = fan.xRot = curl;
		antennaL.xRot = antennaR.xRot = -0.45f + 0.1f * Mth::sin(bob * 0.15f);
		antennaL.yRot = 0.35f + 0.08f * Mth::cos(bob * 0.13f);
		antennaR.yRot = -0.35f - 0.08f * Mth::cos(bob * 0.13f);
		for (int i = 0; i < 6; ++i)
			legs[i].xRot = 0.5f * Mth::sin(bob * 0.6f + i * 0.9f);
		body.render(scale); eyeL.render(scale); eyeR.render(scale);
		abdomen.render(scale); abdomen2.render(scale); fan.render(scale);
		antennaL.render(scale); antennaR.render(scale);
		for (int i = 0; i < 6; ++i) legs[i].render(scale);
	}
};

#endif
