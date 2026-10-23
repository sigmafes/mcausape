#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__HumanoidMobRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__HumanoidMobRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "MobRenderer.h"
#include "../../model/HumanoidModel.h"
class Mob;

class HumanoidMobRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
    HumanoidMobRenderer(HumanoidModel* humanoidModel, float shadow);

	void renderHand();
	void render(Entity* mob_, float x, float y, float z, float rot, float a);
protected:
	void setupRotations(Entity* mob, float bob, float bodyRot, float a);
    void additionalRendering(Mob* mob, float a);

	// Swaps the model drawn (and posed, and held items hung off) as one; see
	// PlayerRenderer, which draws custom skins with a different model.
	void setHumanoidModel(HumanoidModel* model) { humanoidModel = model; setModel(model); }
	HumanoidModel* getHumanoidModel() { return humanoidModel; }

private:
	HumanoidModel* humanoidModel;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__HumanoidMobRenderer_H__*/
