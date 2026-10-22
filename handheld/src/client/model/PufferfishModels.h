#ifndef NET_MINECRAFT_CLIENT_MODEL__PufferfishModels_H__
#define NET_MINECRAFT_CLIENT_MODEL__PufferfishModels_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"

// Java's three pufferfish models (1.13), small, half and fully puffed, on
// the one 32x32 texture. The side fins paddle.

static inline void pufferPart(Model* m, ModelPart& p, int u, int v, float x0, float y0, float z0,
		int w, int h, int d, float px, float py, float pz, float xr = 0, float yr = 0, float zr = 0) {
	p = ModelPart(u, v);
	p.setModel(m);
	p.addBox(x0, y0, z0, w, h, d, 0.0f);
	p.setPos(px, py, pz);
	p.xRot = xr; p.yRot = yr; p.zRot = zr;
}

class PufferfishSmallModel: public Model
{
public:
	ModelPart body, rightEye, leftEye, backFin, rightFin, leftFin;
	PufferfishSmallModel() {
		texWidth = texHeight = 32;
		pufferPart(this, body, 0, 27, -1.5f, -2, -1.5f, 3, 2, 3, 0, 23, 0);
		pufferPart(this, rightEye, 24, 6, -1.5f, 0, -1.5f, 1, 1, 1, 0, 20, 0);
		pufferPart(this, leftEye, 28, 6, 0.5f, 0, -1.5f, 1, 1, 1, 0, 20, 0);
		pufferPart(this, backFin, -3, 0, -1.5f, 0, 0, 3, 0, 3, 0, 22, 1.5f);
		pufferPart(this, rightFin, 25, 0, -1, 0, 0, 1, 0, 2, -1.5f, 22, -1.5f);
		pufferPart(this, leftFin, 25, 0, 0, 0, 0, 1, 0, 2, 1.5f, 22, -1.5f);
	}
	void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale) {
		rightFin.zRot = -0.2f + 0.4f * Mth::sin(bob * 0.2f);
		leftFin.zRot = 0.2f - 0.4f * Mth::sin(bob * 0.2f);
		body.render(scale); rightEye.render(scale); leftEye.render(scale);
		backFin.render(scale); rightFin.render(scale); leftFin.render(scale);
	}
};

class PufferfishMidModel: public Model
{
public:
	static const int N = 11;
	ModelPart parts[N];
	PufferfishMidModel() {
		texWidth = texHeight = 32;
		const float q = Mth::PI / 4;
		pufferPart(this, parts[0], 12, 22, -2.5f, -5, -2.5f, 5, 5, 5, 0, 22, 0);
		pufferPart(this, parts[1], 24, 0, -2, 0, 0, 2, 0, 2, -2.5f, 17, -1.5f);	// right blue fin
		pufferPart(this, parts[2], 24, 3, 0, 0, 0, 2, 0, 2, 2.5f, 17, -1.5f);	// left blue fin
		pufferPart(this, parts[3], 15, 16, -2.5f, -1, 0, 5, 1, 1, 0, 17, -2.5f, q);
		pufferPart(this, parts[4], 10, 16, -2.5f, -1, -1, 5, 1, 1, 0, 17, 2.5f, -q);
		pufferPart(this, parts[5], 8, 16, -1, -5, 0, 1, 5, 1, -2.5f, 22, -2.5f, 0, -q);
		pufferPart(this, parts[6], 8, 16, 0, -5, 0, 1, 5, 1, 2.5f, 22, -2.5f, 0, q);
		pufferPart(this, parts[7], 4, 16, 0, -5, 0, 1, 5, 1, 2.5f, 22, 2.5f, 0, -q);
		pufferPart(this, parts[8], 8, 16, -1, -5, 0, 1, 5, 1, -2.5f, 22, 2.5f, 0, q);
		pufferPart(this, parts[9], 15, 20, -2.5f, 0, 0, 5, 1, 1, 0, 22, -2.5f, -q);
		pufferPart(this, parts[10], 15, 20, -2.5f, 0, 0, 5, 1, 1, 0, 22, 2.5f, q);
	}
	void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale) {
		parts[1].zRot = -0.2f + 0.4f * Mth::sin(bob * 0.2f);
		parts[2].zRot = 0.2f - 0.4f * Mth::sin(bob * 0.2f);
		for (int i = 0; i < N; ++i) parts[i].render(scale);
	}
};

class PufferfishBigModel: public Model
{
public:
	static const int N = 13;
	ModelPart parts[N];
	PufferfishBigModel() {
		texWidth = texHeight = 32;
		const float q = Mth::PI / 4;
		pufferPart(this, parts[0], 0, 0, -4, -8, -4, 8, 8, 8, 0, 22, 0);
		pufferPart(this, parts[1], 24, 0, -2, 0, -1, 2, 1, 2, -4, 15, -2);	// right blue fin
		pufferPart(this, parts[2], 24, 3, 0, 0, -1, 2, 1, 2, 4, 15, -2);	// left blue fin
		pufferPart(this, parts[3], 15, 17, -4, -1, 0, 8, 1, 0, 0, 14, -4, q);
		pufferPart(this, parts[4], 14, 16, -4, -1, 0, 8, 1, 1, 0, 14, 0);
		pufferPart(this, parts[5], 23, 18, -4, -1, 0, 8, 1, 0, 0, 14, 4, -q);
		pufferPart(this, parts[6], 5, 17, -1, -8, 0, 1, 8, 0, -4, 22, -4, 0, -q);
		pufferPart(this, parts[7], 1, 17, 0, -8, 0, 1, 8, 0, 4, 22, -4, 0, q);
		pufferPart(this, parts[8], 15, 20, -4, 0, 0, 8, 1, 0, 0, 22, -4, -q);
		pufferPart(this, parts[9], 15, 20, -4, 0, 0, 8, 1, 0, 0, 22, 0);
		pufferPart(this, parts[10], 15, 20, -4, 0, 0, 8, 1, 0, 0, 22, 4, q);
		pufferPart(this, parts[11], 9, 17, -1, -8, 0, 1, 8, 0, -4, 22, 4, 0, q);
		pufferPart(this, parts[12], 9, 17, 0, -8, 0, 1, 8, 0, 4, 22, 4, 0, -q);
	}
	void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale) {
		parts[1].zRot = -0.2f + 0.4f * Mth::sin(bob * 0.2f);
		parts[2].zRot = 0.2f - 0.4f * Mth::sin(bob * 0.2f);
		for (int i = 0; i < N; ++i) parts[i].render(scale);
	}
};

#endif
