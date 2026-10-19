#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ItemSpriteRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ItemSpriteRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "EntityRenderer.h"

class ItemSpriteRenderer: public EntityRenderer
{
public:
    ItemSpriteRenderer(int icon, float scale = 0.5f);

	void render(Entity* e, float x, float y, float z, float rot, float a);
private:
	int icon;
	float scale;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ItemSpriteRenderer_H__*/
