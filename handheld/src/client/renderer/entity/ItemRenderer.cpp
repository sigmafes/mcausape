#include "ItemRenderer.h"
#include "EntityRenderDispatcher.h"
#include "../Tesselator.h"
#include "../TileRenderer.h"
#include "../Textures.h"
#include "../../gui/Font.h"
#include "../../../world/entity/item/ItemEntity.h"
#include "../../../world/item/ItemInstance.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../util/Mth.h"
#include "../../../util/Random.h"
#include "EntityRenderer.h"
#include "../ItemInHandRenderer.h"
#include "../../gui/Gui.h"
#include "../../../world/item/Item.h"

/*static*/
TileRenderer* ItemRenderer::tileRenderer = new TileRenderer();

ItemRenderer::ItemRenderer()
{
	shadowRadius = 0.15f;
	shadowStrength = 0.75f;
}

void ItemRenderer::teardown_static() {
	if (tileRenderer) {
		delete tileRenderer;
		tileRenderer = NULL;
	}
}

void ItemRenderer::render(Entity* itemEntity_, float x, float y, float z, float rot, float a) {
	ItemEntity* itemEntity = (ItemEntity*) itemEntity_;
	random.setSeed(187);
	ItemInstance* item = &itemEntity->item;

	glPushMatrix2();
	float bob = Mth::sin((itemEntity->age + a) / 10.0f + itemEntity->bobOffs) * 0.1f + 0.1f;
	float spin = ((itemEntity->age + a) / 20.0f + itemEntity->bobOffs) * Mth::RADDEG;

	int count = 1;
	if (item->count > 20) count = 4;
	else if (item->count > 5) count = 3;
	else if (item->count > 1) count = 2;

	glTranslatef2((float) x, (float) y + bob, (float) z);
	//glEnable2(GL_RESCALE_NORMAL);
	if (item->id < 256 && TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape())) {
		glRotatef2(spin, 0, 1, 0);

		float br = itemEntity->getBrightness(a);
		if (item->id == Tile::sand->id || item->id == Tile::sandStone->id) br *= 0.8f;
		glColor4f2(br, br, br, 1.0f);

		bindTexture("terrain.png");
		float s = 1 / 4.0f;
		//if (!Tile::tiles[item->id]->isCubeShaped() && item->id != Tile::stoneSlabHalf->id) {
		const int shape = Tile::tiles[item->id]->getRenderShape();
		if (shape == Tile::SHAPE_CROSS_TEXTURE || shape == Tile::SHAPE_TORCH)
			s = 0.5f;

		glScalef2(s, s, s);
		for (int i = 0; i < count; i++) {
			if (i > 0) {
				glPushMatrix2();
				float xo = (random.nextFloat() * 2 - 1) * 0.2f / s;
				float yo = (random.nextFloat() * 2 - 1) * 0.2f / s;
				float zo = (random.nextFloat() * 2 - 1) * 0.2f / s;
				glTranslatef2(xo, yo, zo);
			}
			//static Stopwatch w;
			//w.start();
			entityRenderDispatcher->itemInHandRenderer->renderItem(NULL, item);
			//tileRenderer->renderTile(Tile::tiles[item->id], item->getAuxValue());
			//w.stop();
			//w.printEvery(100, "render-item");
			if (i > 0) glPopMatrix2();
		}
	} else {
		glScalef2(1 / 2.0f, 1 / 2.0f, 1 / 2.0f);
		int icon = item->getIcon();
		if (item->id < 256) {
			bindTexture("terrain.png");
		} else {
			bindTexture("gui/items.png");
		}
		Tesselator& t = Tesselator::instance;

		float u0 = ((icon % 16) * 16 + 0) / 256.0f;
		float u1 = ((icon % 16) * 16 + 16) / 256.0f;
		float v0 = ((icon / 16) * 16 + 0) / 256.0f;
		float v1 = ((icon / 16) * 16 + 16) / 256.0f;

		float r = 1.0f;
		float xo = 0.5f;
		float yo = 0.25f;

		// glRotatef2(-playerRotX, 1, 0, 0);
		for (int i = 0; i < count; i++) {
			glPushMatrix2();
			if (i > 0) {
				float _xo = (random.nextFloat() * 2 - 1) * 0.3f;
				float _yo = (random.nextFloat() * 2 - 1) * 0.3f;
				float _zo = (random.nextFloat() * 2 - 1) * 0.3f;
				glTranslatef2(_xo, _yo, _zo);
			}
			glRotatef2(180 - entityRenderDispatcher->playerRotY, 0, 1, 0);
			t.begin();
			//t.normal(0, 1, 0);
			t.vertexUV(0 - xo, 0 - yo, 0, u0, v1);
			t.vertexUV(r - xo, 0 - yo, 0, u1, v1);
			t.vertexUV(r - xo, 1 - yo, 0, u1, v0);
			t.vertexUV(0 - xo, 1 - yo, 0, u0, v0);
			//t.end();
			t.draw();

			glPopMatrix2();
		}
	}
	//glDisable2(GL_RESCALE_NORMAL);
	glPopMatrix2();
}


/*static*/
void ItemRenderer::renderGuiItem(Font* font, Textures* textures, const ItemInstance* item, float x, float y, bool fancy) {
	renderGuiItem(font, textures, item, x, y, 16, 16, fancy);
}

/** Draws one item in a menu: blocks as the small 3D cube (or stairs, fence...)
    the world renders, everything else as its 16x16 icon.

    This used to read pre-rendered pictures out of gui/gui_blocks.png so a whole
    hotbar or inventory could go in one batch. That atlas had to be redrawn by
    hand for every block and item added -- an item with no entry in it (the
    buckets) was drawn as a red square -- and it is why a retextured block
    looked right in the world and wrong in the inventory. Drawing from the same
    terrain.png / items.png the world uses has none of those problems; the price
    is one draw per item instead of one per screen, which a few dozen small
    quads do not notice.

    Callers batch with Tesselator::beginOverride(), which holds every vertex
    until the end; the item cannot join that, since it binds its own texture, so
    the batch is flushed first and reopened after. Nothing queued ahead of an
    item needs the texture bound at that moment, because only items are ever
    queued before one. */
void ItemRenderer::renderGuiItem(Font* font, Textures* textures, const ItemInstance* item, float x, float y, float w, float h, bool fancy) {
	if (item == NULL) {
		//LOGW("item is NULL @ ItemRenderer::renderGuiItem\n");
		return;
	}
	if (!Item::items[item->id])
		return;

	Tesselator& t = Tesselator::instance;
	const bool batching = t.isOverridden();
	if (batching)
		t.endOverrideAndDraw();

	renderGuiItemCorrect(font, textures, item, int(x), int(y));

	if (batching)
		t.beginOverride();
}

void ItemRenderer::renderGuiItemDecorations(const ItemInstance* item, float x, float y) {
	if (!item) return;
	if (item->count > 0 && item->isDamaged()) {
		float p = std::floor(13.5f - (float) item->getDamageValue() * 13.0f / (float) item->getMaxDamage());
		int cc = (int) std::floor(255.5f - (float) item->getDamageValue() * 255.0f / (float) item->getMaxDamage());
		//glDisable(GL_LIGHTING);
		//glDisable(GL_DEPTH_TEST);
		//glDisable(GL_TEXTURE_2D);

		Tesselator& t = Tesselator::instance;

		int ca = (255 - cc) << 16 | (cc) << 8;
		int cb = ((255 - cc) / 4) << 16 | (255 / 4) << 8;
		fillRect(t, x + 2, y + 13, 13, 1, 0x000000);
		fillRect(t, x + 2, y + 13, 12, 1, cb);
		fillRect(t, x + 2, y + 13, p, 1, ca);

		//glEnable(GL_TEXTURE_2D);
		//glEnable(GL_LIGHTING);
		//glEnable(GL_DEPTH_TEST);
		glColor4f2(1, 1, 1, 1);
	}
}

void ItemRenderer::fillRect(Tesselator& t, float x, float y, float w, float h, int c) {
	t.begin();
	t.color(c);
	t.vertex(x + 0, y + 0, 0);
	t.vertex(x + 0, y + h, 0);
	t.vertex(x + w, y + h, 0);
	t.vertex(x + w, y + 0, 0);
	t.draw();
}


void ItemRenderer::renderGuiItemCorrect(Font* font, Textures* textures, const ItemInstance* item, int x, int y) {
	if (item == NULL)
		return;

	//glDisable(GL_CULL_FACE);
	if (item->id < 256 && TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape()))
	{
		int paint = item->id;
		textures->loadAndBindTexture("terrain.png");

		static float ff = 0;// ff += 0.005f;
		static float gg = 0;// gg += 0.01f;

		Tile* tile = Tile::tiles[paint];
		glPushMatrix2();
		glTranslatef2((GLfloat)(x - 2), (GLfloat)(y + 3), -8);
		glScalef2(10.0f, 10.0f, 10.0f);
		glTranslatef2(1.0f, 0.5f, 0.0f);
		glRotatef2(ff + 180.0f + 30.0f, 1, 0, 0);
		glRotatef2(gg + 45.0f, 0, 1, 0);

		//glColor4f2(1, 1, 1, 1);
		glScalef2(1, 1, 1);
		tileRenderer->renderGuiTile(tile, item->getAuxValue());
		glPopMatrix2();
	}
	else if (item->getIcon() >= 0)
	{
		//if (item->id == Item::camera->id) {
		//	printf("item->id: %d, %d\n", item->id, item->getIcon());
		//}
		if (item->id < 256) {
			textures->loadAndBindTexture("terrain.png");
		} else {
			textures->loadAndBindTexture("gui/items.png");
		}
		//Tesselator& t = Tesselator::instance;
		//t.scale2d(Gui::InvGuiScale, Gui::InvGuiScale);
		blit((float)x, (float)y, (float)(item->getIcon() % 16 * 16), (float)(item->getIcon() / 16 * 16), 16, 16);
		//t.resetScale();
	}
	//glEnable(GL_CULL_FACE);
}

/*static*/
void ItemRenderer::blit(float x, float y, float sx, float sy, float w, float h) {
	float blitOffset = 0;
	const float us = 1 / 256.0f;
	const float vs = 1 / 256.0f;
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.vertexUV(x, y + h, blitOffset, sx * us, (sy + h) * vs);
	t.vertexUV(x + w, y + h, blitOffset, (sx + w) * us, (sy + h) * vs);
	t.vertexUV(x + w, y, blitOffset, (sx + w) * us, sy * vs);
	t.vertexUV(x, y, blitOffset, sx * us, sy * vs);
	//t.end();
	t.draw();
}
