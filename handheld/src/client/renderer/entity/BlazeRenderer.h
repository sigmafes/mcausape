#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__BlazeRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__BlazeRenderer_H__

#include "MobRenderer.h"
#include "../gles.h"
#include "../../model/BlazeModel.h"

class BlazeRenderer: public MobRenderer
{
    typedef MobRenderer super;
public:
    BlazeRenderer()
    :   super(new BlazeModel(), 0.5f)
    {}
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__BlazeRenderer_H__*/
