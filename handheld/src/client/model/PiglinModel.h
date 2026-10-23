#ifndef NET_MINECRAFT_CLIENT_MODEL__PiglinModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__PiglinModel_H__

#include "HumanoidModel.h"

// Java's PiglinModel (1.16): the player's 64x64 humanoid with a wide head
// (10 across), a snout and tusks, and floppy ears. Piglin, piglin brute and
// zombified piglin all wear it.
class PiglinModel: public HumanoidModel
{
	typedef HumanoidModel super;
	void rebuild(ModelPart& p, int u, int v, float x0, float y0, float z0, int w, int h, int d) {
		p.clear(); p.setModel(this); p.mirror = false; p.texOffs(u, v); p.addBox(x0, y0, z0, w, h, d, 0);
	}
	void child(ModelPart& c, ModelPart& parent, int u, int v, float x0, float y0, float z0, int w, int h, int d, float grow = 0) {
		c.setModel(this); c.texOffs(u, v); c.addBox(x0, y0, z0, w, h, d, grow); parent.addChild(&c);
	}
public:
	ModelPart snout, tuskL, tuskR, earL, earR, jacket, sleeve0, sleeve1, pants0, pants1;

	PiglinModel() : super(0, 0) {
		texHeight = 64;
		rebuild(head, 0, 0, -5, -8, -4, 10, 8, 8);
		child(snout, head, 31, 1, -2, -4, -5, 4, 4, 1);
		child(tuskL, head, 2, 4, 2, -2, -5, 1, 2, 1);
		child(tuskR, head, 2, 0, -3, -2, -5, 1, 2, 1);
		earL.setModel(this); earL.texOffs(51, 6); earL.addBox(0.0f, 0.0f, -2.0f, 1, 5, 4, 0.0f); earL.setPos(4.5f, -6, 0);
		earR.setModel(this); earR.texOffs(39, 6); earR.addBox(-1.0f, 0.0f, -2.0f, 1, 5, 4, 0.0f); earR.setPos(-4.5f, -6, 0);
		head.addChild(&earL); head.addChild(&earR);
		rebuild(body, 16, 16, -4, 0, -2, 8, 12, 4);
		rebuild(arm0, 40, 16, -3, -2, -2, 4, 12, 4);
		rebuild(arm1, 32, 48, -1, -2, -2, 4, 12, 4);
		rebuild(leg0, 0, 16, -2, 0, -2, 4, 12, 4);
		rebuild(leg1, 16, 48, -2, 0, -2, 4, 12, 4);
		child(jacket, body, 16, 32, -4, 0, -2, 8, 12, 4, 0.25f);
		child(sleeve0, arm0, 40, 32, -3, -2, -2, 4, 12, 4, 0.25f);
		child(sleeve1, arm1, 48, 48, -1, -2, -2, 4, 12, 4, 0.25f);
		child(pants0, leg0, 0, 32, -2, 0, -2, 4, 12, 4, 0.25f);
		child(pants1, leg1, 0, 48, -2, 0, -2, 4, 12, 4, 0.25f);
	}

	void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale) {
		// The ears swing a little, as Java's.
		earL.zRot = -0.5236f - Mth::cos(bob * 0.1f + time) * 0.25f * (1 + r);
		earR.zRot = 0.5236f + Mth::cos(bob * 0.12f + time) * 0.25f * (1 + r);
		super::render(e, time, r, bob, yRot, xRot, scale);
	}
};

#endif
