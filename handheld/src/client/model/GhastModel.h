#ifndef NET_MINECRAFT_CLIENT_MODEL__GhastModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__GhastModel_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"

// The ghast, ported from the Console Edition's GhastModel: a 16-unit cube with nine
// tentacles hanging from under it, swaying. All its boxes read the same corner of the
// texture. (+y points down in model space, so the body sits at y = 8 and the tentacles
// start at y = 15.)
class GhastModel: public Model
{
    typedef Model super;
public:
    static const int TENTACLE_COUNT = 9;

    ModelPart body;
    ModelPart tentacles[TENTACLE_COUNT];

    GhastModel()
    :   body(0, 0)
    {
        body.setModel(this);
        body.addBox(-8.0f, -8.0f, -8.0f, 16, 16, 16, 0.0f);
        body.setPos(0, 8, 0);

        // Lengths between 8 and 14, fixed so every ghast looks the same.
        static const int lengths[TENTACLE_COUNT] = { 12, 9, 13, 10, 14, 8, 11, 13, 9 };
        for (int i = 0; i < TENTACLE_COUNT; ++i) {
            tentacles[i] = ModelPart(0, 0);
            tentacles[i].setModel(this);

            float xo = ((i % 3 - (i / 3 % 2) * 0.5f + 0.25f) / 2.0f * 2 - 1) * 5;
            float zo = ((i / 3) / 2.0f * 2 - 1) * 5;
            tentacles[i].addBox(-1.0f, 0.0f, -1.0f, 2, lengths[i], 2, 0.0f);
            tentacles[i].setPos(xo, 15, zo);
        }
    }

    /*@Override*/
    void render(Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale) {
        setupAnim(time, r, bob, yRot, xRot, scale);

        glPushMatrix();
        glTranslatef(0, 0.6f, 0);

        body.render(scale);
        for (int i = 0; i < TENTACLE_COUNT; ++i)
            tentacles[i].render(scale);

        glPopMatrix();
    }

    void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) {
        for (int i = 0; i < TENTACLE_COUNT; ++i)
            tentacles[i].xRot = 0.2f * Mth::sin(bob * 0.3f + i) + 0.4f;
    }
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__GhastModel_H__*/
