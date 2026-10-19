#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MagmaCubeRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MagmaCubeRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../../model/MagmaCubeModel.h"

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
        // Size 2 of the Console Edition's three (1, 2, 4): a cube of 1.2 blocks.
        glScalef(2, 2, 2);
    }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MagmaCubeRenderer_H__*/
