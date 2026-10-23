#ifndef NET_MINECRAFT_CLIENT_MODEL__HoglinModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__HoglinModel_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"

// Java's HoglinModel (1.16) on its 128x64 texture: a long body with a mane,
// a big head tilted down with ears and tusks hung off it, four legs.
// The zoglin is the same model on its own texture.
class HoglinModel: public Model
{
	typedef Model super;
	void box(ModelPart& p, int u, int v, float x0, float y0, float z0, int w, int h, int d, float px, float py, float pz) {
		p = ModelPart(u, v);
		p.setModel(this);
		p.addBox(x0, y0, z0, w, h, d, 0.0f);
		p.setPos(px, py, pz);
	}
public:
	ModelPart body, mane, head, earR, earL, hornR, hornL, legRF, legLF, legRH, legLH;

	HoglinModel() {
		texWidth = 128; texHeight = 64;
		box(body, 1, 1, -8, -7, -13, 16, 14, 26, 0, 7, 0);
		box(mane, 90, 33, 0, 0, -9, 0, 10, 19, 0, -14, -5);
		body.addChild(&mane);
		box(head, 61, 1, -7, -3, -19, 14, 6, 19, 0, 2, -12);
		head.xRot = 0.87266f;
		box(earR, 1, 1, -6, -1, -2, 6, 1, 4, -6, -2, -3);
		box(earL, 1, 6, 0, -1, -2, 6, 1, 4, 6, -2, -3);
		earR.zRot = -0.6981317f; earL.zRot = 0.6981317f;
		box(hornR, 10, 13, -1, -11, -1, 2, 11, 2, -7, 2, -12);
		box(hornL, 1, 13, -1, -11, -1, 2, 11, 2, 7, 2, -12);
		head.addChild(&earR); head.addChild(&earL); head.addChild(&hornR); head.addChild(&hornL);
		box(legRF, 66, 42, -3, 0, -3, 6, 14, 6, -4, 10, -8.5f);
		box(legLF, 41, 42, -3, 0, -3, 6, 14, 6, 4, 10, -8.5f);
		box(legRH, 21, 45, -2.5f, 0, -2.5f, 5, 11, 5, -5, 13, 10);
		box(legLH, 0, 45, -2.5f, 0, -2.5f, 5, 11, 5, 5, 13, 10);
	}

	void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale) {
		// Java's walk: the legs swing in pairs, the ears flap with it.
		head.yRot = yRot * Mth::DEGRAD;
		const float swing = Mth::cos(time) * 1.2f * r;
		legRF.xRot = swing;  legLH.xRot = swing;
		legLF.xRot = -swing; legRH.xRot = -swing;
		const float ear = 0.6981317f - r * Mth::sin(time) * 0.3f;
		earR.zRot = -ear; earL.zRot = ear;
		body.render(scale); head.render(scale);
		legRF.render(scale); legLF.render(scale); legRH.render(scale); legLH.render(scale);
	}
};

#endif
