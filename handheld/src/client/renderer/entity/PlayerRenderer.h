#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PlayerRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PlayerRenderer_H__

#include "HumanoidMobRenderer.h"

class Player;
class PlayerSkinModel;

class PlayerRenderer : public HumanoidMobRenderer
{
	typedef HumanoidMobRenderer super;
public:
	PlayerRenderer(HumanoidModel* humanoidModel, float shadow);
	~PlayerRenderer();

	virtual int prepareArmor(Mob* mob, int layer, float a);
	virtual int armorTag(Mob* mob, int layer);

	virtual void setupPosition(Entity* mob, float x, float y, float z);
	virtual void setupRotations(Entity* mob, float bob, float bodyRot, float a);

	virtual void render(Entity* mob, float x, float y, float z, float rot, float a);
	virtual void renderName(Mob* mob, float x, float y, float z);
	virtual void onGraphicsReset();

	/// The first-person arm, with the player's skin bound.
	void renderHandFor(Player* player);
protected:
	virtual void bindMobTexture(Mob* mob);
private:
	/// The PlayerSkins id this player is drawn with, 0 for mob/char.png.
	int skinFor(Player* player);
	HumanoidModel* modelFor(int skinId);

	HumanoidModel* armorParts1;
	HumanoidModel* armorParts2;
	// One per PlayerSkins::Model, made the first time a skin needs it.
	PlayerSkinModel* skinModels[3];
};


#endif /* NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PlayerRenderer_H__ */