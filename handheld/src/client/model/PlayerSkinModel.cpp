#include "PlayerSkinModel.h"
#include "../skin/PlayerSkins.h"
#include "../renderer/gles.h"

PlayerSkinModel::PlayerSkinModel(int skinModel)
:	super(0, 0)
{
	if (skinModel == PlayerSkins::MODEL_CLASSIC) {
		// The base parts are already the classic layout; only the hat is new.
		addLayer(hat, head, 32, 0, -4, -8, -4, 8, 8, 8, 0.5f);
		return;
	}

	texHeight = 64;
	const bool slim = (skinModel == PlayerSkins::MODEL_SLIM);
	const int armW = slim ? 3 : 4;

	rebuild(head, 0, 0, -4, -8, -4, 8, 8, 8);
	rebuild(body, 16, 16, -4, 0, -2, 8, 12, 4);
	// Right arm hangs off its outer edge, so a narrower arm moves its box, not
	// its pivot; slim arms also sit half a pixel lower, as in Minecraft.
	rebuild(arm0, 40, 16, slim ? -2.0f : -3.0f, -2, -2, armW, 12, 4);
	rebuild(arm1, 32, 48, -1, -2, -2, armW, 12, 4);
	rebuild(leg0, 0, 16, -2, 0, -2, 4, 12, 4);
	rebuild(leg1, 16, 48, -2, 0, -2, 4, 12, 4);
	if (slim) {
		arm0.setPos(-5, 2.5f, 0);
		arm1.setPos(5, 2.5f, 0);
	}

	addLayer(hat, head, 32, 0, -4, -8, -4, 8, 8, 8, 0.5f);
	addLayer(jacket, body, 16, 32, -4, 0, -2, 8, 12, 4, 0.25f);
	addLayer(sleeve0, arm0, 40, 32, slim ? -2.0f : -3.0f, -2, -2, armW, 12, 4, 0.25f);
	addLayer(sleeve1, arm1, 48, 48, -1, -2, -2, armW, 12, 4, 0.25f);
	addLayer(pants0, leg0, 0, 32, -2, 0, -2, 4, 12, 4, 0.25f);
	addLayer(pants1, leg1, 0, 48, -2, 0, -2, 4, 12, 4, 0.25f);
}

void PlayerSkinModel::rebuild(ModelPart& part, int u, int v, float x0, float y0, float z0, int w, int h, int d)
{
	// clear() drops the cubes and the model link; setModel() brings the link
	// back and, with it, this model's texture size -- which the cube reads when
	// it works out its UVs, so it has to be right before addBox().
	part.clear();
	part.setModel(this);
	part.mirror = false;
	part.texOffs(u, v);
	part.addBox(x0, y0, z0, w, h, d, 0);
}

void PlayerSkinModel::addLayer(ModelPart& layer, ModelPart& parent, int u, int v, float x0, float y0, float z0, int w, int h, int d, float grow)
{
	layer.setModel(this);
	layer.texOffs(u, v);
	layer.addBox(x0, y0, z0, w, h, d, grow);
	parent.addChild(&layer);
}

void PlayerSkinModel::render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale)
{
	// The outer layer is mostly transparent pixels, which have to be cut out
	// rather than drawn black. NinecraftApp sets the alpha func once at start.
	glEnable2(GL_ALPHA_TEST);
	super::render(e, time, r, bob, yRot, xRot, scale);
}

void PlayerSkinModel::onGraphicsReset()
{
	super::onGraphicsReset();
	hat.onGraphicsReset();
	jacket.onGraphicsReset();
	sleeve0.onGraphicsReset();
	sleeve1.onGraphicsReset();
	pants0.onGraphicsReset();
	pants1.onGraphicsReset();
}
