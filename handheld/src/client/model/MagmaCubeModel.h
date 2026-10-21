#ifndef NET_MINECRAFT_CLIENT_MODEL__MagmaCubeModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__MagmaCubeModel_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../world/entity/monster/MagmaCube.h"

// The magma cube, ported from the Console Edition's LavaSlimeModel: eight one-unit
// slices stacked into an 8x8x8 cube round a 4x4x4 core, each slice reading its own
// strip of the texture.
class MagmaCubeModel: public Model
{
    typedef Model super;
public:
    static const int SLICE_COUNT = 8;

    ModelPart slices[SLICE_COUNT];
    ModelPart core;

    MagmaCubeModel()
    :   core(0, 16)
    {
        for (int i = 0; i < SLICE_COUNT; ++i) {
            int u = 0, v = i;
            if (i == 2) { u = 24; v = 10; }
            else if (i == 3) { u = 24; v = 19; }
            slices[i] = ModelPart(u, v);
            slices[i].setModel(this);
            slices[i].addBox(-4.0f, 16.0f + i, -4.0f, 8, 1, 8, 0.0f);
        }

        core.setModel(this);
        core.addBox(-2.0f, 18.0f, -2.0f, 4, 4, 4, 0.0f);
    }

    /** The slices spread apart while the cube is squashed, which is what makes
        it read as a blob rather than a block. */
    /*@Override*/
    void prepareMobModel(Mob* mob, float time, float r, float a) {
        MagmaCube* cube = (MagmaCube*) mob;
        float squish = cube->oSquish + (cube->squish - cube->oSquish) * a;
        if (squish < 0)
            squish = 0;
        for (int i = 0; i < SLICE_COUNT; ++i)
            slices[i].y = -(4 - i) * squish * 1.7f;
    }

    /*@Override*/
    void render(Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale) {
        core.render(scale);
        for (int i = 0; i < SLICE_COUNT; ++i)
            slices[i].render(scale);
    }
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__MagmaCubeModel_H__*/
