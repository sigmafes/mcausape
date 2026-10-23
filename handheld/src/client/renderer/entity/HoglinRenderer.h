#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__HoglinRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__HoglinRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../../model/HoglinModel.h"
#include "../../../world/entity/monster/Hoglin.h"

// The hoglin and zoglin; a baby at half size.
class HoglinRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
	HoglinRenderer() : super(new HoglinModel(), 0.7f) {}
protected:
	void scale(Mob* mob, float a) {
		if (((Hoglin*) mob)->isBaby()) glScalef2(0.5f, 0.5f, 0.5f);
	}
};

#endif
