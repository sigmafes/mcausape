#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MobRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MobRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "EntityRenderer.h"

#include <string>
#include <vector>

class Model;
class Entity;
class Mob;

class MobRenderer: public EntityRenderer
{
	static const int MAX_ARMOR_LAYERS = 4;
public:
	// @note: MobRenderer::model will be deleted automagically
	// Armor models (setArmor(Model*)) is to be deleted by derived classes.
    MobRenderer(Model* model, float shadow);
	~MobRenderer();

	virtual int prepareArmor(Mob* mob, int layer, float a);
	// The shaders' material tag for a worn armour layer (-1: none), as
	// TileRenderer tags blocks: 200 metal, 215 polished (PlayerRenderer).
	virtual int armorTag(Mob* mob, int layer) { return -1; }

	virtual void setupPosition(Entity* mob, float x, float y, float z);
    virtual void setupRotations(Entity* mob_, float bob, float bodyRot, float a);

    virtual float getAttackAnim(Mob* mob, float a);
    virtual float getBob(Mob* mob, float a);

    virtual float getFlipDegrees(Mob* mob);

    virtual int getOverlayColor(Mob* mob, float br, float a);

    virtual void scale(Mob* mob, float a);

	virtual void render(Entity* mob_, float x, float y, float z, float rot, float a);
	virtual void renderName(Mob* mob, float x, float y, float z);
	virtual void renderNameTag(Mob* mob, const std::string& name, float x, float y, float z, int maxDist);
	void drawNameTag(Mob* mob, const std::string& name, float x, float y, float z, int maxDist);
public:
	struct PendingTag { MobRenderer* renderer; Mob* mob; std::string name; float x, y, z; int maxDist; };
	static bool deferNameTags;
	static std::vector<PendingTag> pendingTags;
	static void flushNameTags();
protected:
	virtual void additionalRendering(Mob* mob, float a);

	virtual void onGraphicsReset();
protected:
	void setArmor(Model* armor);
	Model* getArmor();

	/// Binds the texture the mob is drawn with. PlayerRenderer overrides it
	/// for custom skins, which are not files Textures can load by name.
	virtual void bindMobTexture(Mob* mob);
	void setModel(Model* model) { this->model = model; }
	Model* getModel() { return model; }
private:
	Model* model;
	Model* armor;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MobRenderer_H__*/
