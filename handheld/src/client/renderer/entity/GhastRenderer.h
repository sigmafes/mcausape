#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__GhastRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__GhastRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../../model/GhastModel.h"
#include "../../../world/entity/monster/Ghast.h"

class GhastRenderer: public MobRenderer
{
    typedef MobRenderer super;
public:
    GhastRenderer()
    :   super(new GhastModel(), 0.5f)
    {}

protected:
    /*@Override*/
    void scale(Mob* mob, float a) {
        // The Console Edition's squash while charging: it gets a little squatter and
        // wider as the shot builds up.
        Ghast* ghast = (Ghast*) mob;
        float ss = ghast->charge / 20.0f;
        if (ss < 0) ss = 0;
        if (ss > 1) ss = 1;
        ss = 1 / (ss * ss * ss * ss * ss * 2 + 1);
        float s = (8 + ss) / 2;
        float hs = (8 + 1 / ss) / 2;
        glScalef(hs, s, hs);
        glColor4f2(1, 1, 1, 1);
    }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__GhastRenderer_H__*/
