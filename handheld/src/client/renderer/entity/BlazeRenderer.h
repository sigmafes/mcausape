#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__BlazeRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__BlazeRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../TileRenderer.h"
#include "../../model/BlazeModel.h"
#include "../../model/AquaticBlazeModel.h"

class BlazeRenderer: public MobRenderer
{
    typedef MobRenderer super;
public:
    BlazeRenderer()
    :   super(new BlazeModel(), 0.5f)
    {}
    // Tagged as giving light (alpha 245, as TileRenderer tags glowing
    // blocks) while the shaders are on: no sun shading on it, and it blooms.
    void render(Entity* mob, float x, float y, float z, float rot, float a) {
        if (TileRenderer::shaderWater) glColor4f2(1, 1, 1, 245.0f / 255.0f);
        super::render(mob, x, y, z, rot, a);
        glColor4f2(1, 1, 1, 1);
    }
};

// The aquatic blaze: its rods turn and bob at half the blaze's pace.
class AquaticBlazeRenderer: public MobRenderer
{
public:
    AquaticBlazeRenderer() : MobRenderer(new AquaticBlazeModel(), 0.5f) {}
protected:
    float getBob(Mob* mob, float a) { return (mob->tickCount + a) * 0.5f; }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__BlazeRenderer_H__*/
