#ifndef NET_MINECRAFT_CLIENT_MODEL__FishModels_H__
#define NET_MINECRAFT_CLIENT_MODEL__FishModels_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"
#include "../../world/entity/animal/Fish.h"

static inline void box(ModelPart& p, float x, float y, float z, int w, int h, int d) {
	p.addBox(x, y, z, w, h, d, 0.0f);
}

// Java's CodModel (1.13), on its 32x32 texture: a body, head and nose, two
// side fins, a top fin and the tail fin that waves.
class CodModel: public Model
{
	typedef Model super;
public:
	ModelPart body, head, nose, rightFin, leftFin, tailFin, topFin;

	CodModel()
	:	body(0, 0), head(11, 0), nose(0, 0), rightFin(22, 1), leftFin(22, 4), tailFin(22, 3), topFin(20, 0)
	{
		texWidth = texHeight = 32;
		body.setModel(this);     box(body, -1, -2, 0, 2, 4, 7);     body.setPos(0, 22, 0);
		head.setModel(this);     box(head, -1, -2, -3, 2, 4, 3);    head.setPos(0, 22, 0);
		nose.setModel(this);     box(nose, -1, -2, -1, 2, 3, 1);    nose.setPos(0, 22, -3);
		rightFin.setModel(this); box(rightFin, -2, 0, -1, 2, 0, 2); rightFin.setPos(-1, 23, 0);
		rightFin.zRot = -Mth::PI / 4;
		leftFin.setModel(this);  box(leftFin, 0, 0, -1, 2, 0, 2);   leftFin.setPos(1, 23, 0);
		leftFin.zRot = Mth::PI / 4;
		tailFin.setModel(this);  box(tailFin, 0, -2, 0, 0, 4, 4);   tailFin.setPos(0, 22, 7);
		topFin.setModel(this);   box(topFin, 0, -1, -1, 0, 1, 6);   topFin.setPos(0, 20, 0);
	}

	void render(Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale) {
		const float f = entity && ((Fish*) entity)->swimming() ? 1.0f : 1.5f;
		tailFin.yRot = -f * 0.45f * Mth::sin(0.6f * bob);
		body.render(scale); head.render(scale); nose.render(scale);
		rightFin.render(scale); leftFin.render(scale); tailFin.render(scale); topFin.render(scale);
	}
};

// Java's SalmonModel (1.13), 32x32: the front of the body, and the back half
// with its tail and fin that swings from the middle.
class SalmonModel: public Model
{
	typedef Model super;
public:
	ModelPart bodyFront, bodyBack, head, tail, topFrontFin, topBackFin, rightFin, leftFin;

	SalmonModel()
	:	bodyFront(0, 0), bodyBack(0, 13), head(22, 0), tail(20, 10), topFrontFin(2, 1), topBackFin(0, 2), rightFin(0, 0), leftFin(0, 0)
	{
		texWidth = texHeight = 32;
		bodyFront.setModel(this);   box(bodyFront, -1.5f, -2.5f, 0, 3, 5, 8);  bodyFront.setPos(0, 20, 0);
		bodyBack.setModel(this);    box(bodyBack, -1.5f, -2.5f, 0, 3, 5, 8);   bodyBack.setPos(0, 20, 8);
		head.setModel(this);        box(head, -1, -2, -3, 2, 4, 3);            head.setPos(0, 20, 0);
		// Children of the back half in Java; here the same pivot, offset boxes.
		tail.setModel(this);        box(tail, 0, -2.5f, 8, 0, 5, 6);           tail.setPos(0, 20, 8);
		topBackFin.setModel(this);  box(topBackFin, 0, -4.5f, -1, 0, 2, 4);    topBackFin.setPos(0, 20, 8);
		topFrontFin.setModel(this); box(topFrontFin, 0, -4.5f, 5, 0, 2, 3);    topFrontFin.setPos(0, 20, 0);
		rightFin.setModel(this);    box(rightFin, -2, 0, 0, 2, 0, 2);          rightFin.setPos(-1.5f, 21.5f, 0);
		rightFin.zRot = -Mth::PI / 4;
		leftFin.setModel(this);     box(leftFin, 0, 0, 0, 2, 0, 2);            leftFin.setPos(1.5f, 21.5f, 0);
		leftFin.zRot = Mth::PI / 4;
	}

	void render(Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale) {
		const float f = entity && ((Fish*) entity)->swimming() ? 1.0f : 1.3f;
		const float swing = -f * 0.25f * Mth::sin(0.6f * bob);
		bodyBack.yRot = tail.yRot = topBackFin.yRot = swing;
		bodyFront.render(scale); bodyBack.render(scale); head.render(scale); tail.render(scale);
		topFrontFin.render(scale); topBackFin.render(scale); rightFin.render(scale); leftFin.render(scale);
	}
};

#endif
