#ifndef NET_MINECRAFT_CLIENT_MODEL__AquaticBlazeModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__AquaticBlazeModel_H__

#include "BlazeModel.h"

// The aquatic blaze: the blaze's head and rods, with coral growing out of
// them in 3D -- each sprig two crossed flat planes, as a plant is drawn --
// three on the head (turning with it) and one on top of every rod. The
// sprigs' pictures are on the texture's free right half, five colours,
// an 8x14 patch each from (8,16): the first plane's two faces at its top,
// the second's at rows 25-29.
class AquaticBlazeModel: public BlazeModel
{
	typedef BlazeModel super;

	struct Sprig {
		ModelPart a, b;
		void make(Model* m, int colour, float ox, float oy, float oz) {
			const int u = 8 + colour * 8, v = 16;
			a = ModelPart(u, v); a.setModel(m); a.addBox(ox - 2, oy - 5, oz, 4, 5, 0, 0.0f);
			b = ModelPart(u, v + 5); b.setModel(m); b.addBox(ox, oy - 5, oz - 2, 0, 5, 4, 0.0f);
		}
		void place(float x, float y, float z, float xr, float yr) {
			a.setPos(x, y, z); b.setPos(x, y, z);
			a.xRot = b.xRot = xr; a.yRot = b.yRot = yr;
		}
		void render(float s) { a.render(s); b.render(s); }
	};

public:
	Sprig headSprigs[3];
	Sprig rodSprigs[ROD_COUNT];

	AquaticBlazeModel() {
		headSprigs[0].make(this, 1, -2, -4, -1);
		headSprigs[1].make(this, 3, 2, -4, 1);
		headSprigs[2].make(this, 0, 0, -4, 2);
		for (int i = 0; i < ROD_COUNT; ++i)
			rodSprigs[i].make(this, i % 5, 1, 0, 1);
	}

	void render(Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale) {
		super::render(entity, time, r, bob, yRot, xRot, scale);
		for (int i = 0; i < 3; ++i) {
			headSprigs[i].place(head.x, head.y, head.z, head.xRot, head.yRot);
			headSprigs[i].render(scale);
		}
		for (int i = 0; i < ROD_COUNT; ++i) {
			rodSprigs[i].place(rods[i].x, rods[i].y, rods[i].z, 0, i * 0.7f);
			rodSprigs[i].render(scale);
		}
	}
};

#endif
