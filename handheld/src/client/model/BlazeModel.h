#ifndef NET_MINECRAFT_CLIENT_MODEL__BlazeModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__BlazeModel_H__

#include "Model.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"

// The blaze, ported from the Console Edition's BlazeModel: a head and twelve rods in
// three rings that turn about it at different speeds and bob up and down.
class BlazeModel: public Model
{
    typedef Model super;
public:
    static const int ROD_COUNT = 12;

    ModelPart head;
    ModelPart rods[ROD_COUNT];

    BlazeModel()
    :   head(0, 0)
    {
        for (int i = 0; i < ROD_COUNT; ++i) {
            rods[i] = ModelPart(0, 16);
            rods[i].setModel(this);
            rods[i].addBox(0.0f, 0.0f, 0.0f, 2, 8, 2, 0.0f);
        }

        head.setModel(this);
        head.addBox(-4.0f, -4.0f, -4.0f, 8, 8, 8, 0.0f);
    }

    /*@Override*/
    void render(Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale) {
        setupAnim(time, r, bob, yRot, xRot, scale);

        head.render(scale);
        for (int i = 0; i < ROD_COUNT; ++i)
            rods[i].render(scale);
    }

    void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) {
        float angle = bob * Mth::PI * -0.1f;
        for (int i = 0; i < 4; ++i) {
            rods[i].setPos(Mth::cos(angle) * 9.0f, -2 + Mth::cos((i * 2 + bob) * 0.25f), Mth::sin(angle) * 9.0f);
            angle += Mth::PI * 0.5f;
        }
        angle = 0.25f * Mth::PI + bob * Mth::PI * 0.03f;
        for (int i = 4; i < 8; ++i) {
            rods[i].setPos(Mth::cos(angle) * 7.0f, 2 + Mth::cos((i * 2 + bob) * 0.25f), Mth::sin(angle) * 7.0f);
            angle += Mth::PI * 0.5f;
        }

        angle = 0.15f * Mth::PI + bob * Mth::PI * -0.05f;
        for (int i = 8; i < 12; ++i) {
            rods[i].setPos(Mth::cos(angle) * 5.0f, 11 + Mth::cos((i * 1.5f + bob) * 0.5f), Mth::sin(angle) * 5.0f);
            angle += Mth::PI * 0.5f;
        }

        head.yRot = yRot / (float) (180 / Mth::PI);
        head.xRot = xRot / (float) (180 / Mth::PI);
    }
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__BlazeModel_H__*/
