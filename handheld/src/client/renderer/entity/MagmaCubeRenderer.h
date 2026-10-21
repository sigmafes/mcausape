#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MagmaCubeRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MagmaCubeRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../../model/MagmaCubeModel.h"
#include "../../../world/entity/monster/MagmaCube.h"

class MagmaCubeRenderer: public MobRenderer
{
    typedef MobRenderer super;
public:
    MagmaCubeRenderer()
    :   super(new MagmaCubeModel(), 0.25f)
    {}

protected:
    /*@Override*/
    void scale(Mob* mob, float a) {
        // Size 2 of the Console Edition's three (1, 2, 4): a cube of 1.2 blocks,
        // stretched tall while it is in the air and squashed flat as it lands.
        MagmaCube* cube = (MagmaCube*) mob;
        const float size = (float) MagmaCube::SIZE;
        const float squish = (cube->oSquish + (cube->squish - cube->oSquish) * a) / (size * 0.5f + 1);
        const float w = 1 / (squish + 1);
        glScalef(w * size, 1 / w * size, w * size);
    }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MagmaCubeRenderer_H__*/
