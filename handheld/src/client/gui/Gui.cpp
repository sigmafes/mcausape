#include "../../world/effect/MobEffect.h"
#include <cstdio>
#include "Gui.h"
#include "../../platform/time.h"
#include "Font.h"
#include "screens/IngameBlockSelectionScreen.h"
#include "../Minecraft.h"
#include "../player/LocalPlayer.h"
#include "../renderer/Tesselator.h"
#include "../renderer/TileRenderer.h"
#include "../renderer/LevelRenderer.h"
#include "../renderer/GameRenderer.h"
#include "../renderer/entity/ItemRenderer.h"
#include "../player/input/IInputHolder.h"
#include "../gamemode/GameMode.h"
#include "../gamemode/CreativeMode.h"
#include "../renderer/Textures.h"
#include "../../AppConstants.h"
#include "../../world/entity/player/Inventory.h"
#include "../../world/level/material/Material.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../../platform/input/Mouse.h"
#include "../../world/level/Level.h"
#include "../../world/PosTranslator.h"
#include "../../platform/input/Keyboard.h"
#include <algorithm>
#include <sstream>

float Gui::InvGuiScale = 1.0f / 3.0f;
float Gui::GuiScale = 1.0f / Gui::InvGuiScale;
const float Gui::DropTicks = 40.0f;

//#include <android/log.h>

Gui::Gui(Minecraft* minecraft)
:	minecraft(minecraft),
	tickCount(0),
	progress(0),
	overlayMessageTime(0),
	animateOverlayMessageColor(false),
	tbr(1),
	_inventoryNeedsUpdate(true),
	_flashSlotId(-1),
	_flashSlotStartTime(-1),
	_slotFont(NULL),
	_numSlots(4),
	_currentDropTicks(-1),
	_currentDropSlot(-1),
	MAX_MESSAGE_WIDTH(240),
	itemNameOverlayTime(2),
	lastSelectedSlot(-1),
	lastSelectedId(-1)
{
	glGenBuffers2(1, &_inventoryRc.vboId);
	glGenBuffers2(1, &rcFeedbackInner.vboId);
	glGenBuffers2(1, &rcFeedbackOuter.vboId);
	//Gui::InvGuiScale = 1.0f / (int) (3 * Minecraft::width / 854);
}

Gui::~Gui()
{
	if (_slotFont)
		delete _slotFont;

	glDeleteBuffers(1, &_inventoryRc.vboId);
}

void Gui::render(float a, bool mouseFree, int xMouse, int yMouse) {

	if (!minecraft->level || !minecraft->player)
		return;

	//minecraft->gameRenderer->setupGuiScreen();
	Font* font = minecraft->font;

	const bool isTouchInterface = minecraft->useTouchscreen();
	const int screenWidth = (int)(minecraft->width * InvGuiScale);
	const int screenHeight = (int)(minecraft->height * InvGuiScale);
	blitOffset = -90;
	renderProgressIndicator(isTouchInterface, screenWidth, screenHeight, a);

	// /fps: frames counted over each second, shown in the top left.
	{
		static int frames = 0, shown = 0, since = 0;
		const int now = getTimeMs();
		++frames;
		if (since == 0) since = now;
		if (now - since >= 1000) { shown = frames * 1000 / (now - since); frames = 0; since = now; }
		if (minecraft->showFps) {
			char buf[32];
			snprintf(buf, sizeof buf, "FPS: %d", shown);
			font->drawShadow(buf, 2, 2, 0xffffffff);
		}
	}
	renderInk(screenWidth, screenHeight);

	// A phone has no T: this opens the chat (TouchscreenInput handles the tap).
	if (isTouchInterface && !minecraft->screen && !minecraft->options.hideGui) {
		RectangleArea r = getChatButtonArea();
		const int x0 = (int)(r._x0 * InvGuiScale), y0 = (int)(r._y0 * InvGuiScale);
		const int x1 = (int)(r._x1 * InvGuiScale), y1 = (int)(r._y1 * InvGuiScale);
		fill(x0, y0, x1, y1, 0x80000000);
		drawCenteredString(font, "Chat", (x0 + x1) / 2, (y0 + y1) / 2 - 4, 0xffe0e0e0);
	}

	glColor4f2(1, 1, 1, 1);

	// H: 4
    // T: 7
    // L: 6
    // F: 3
	int ySlot = screenHeight - 16 - 3;

	if (minecraft->gameMode->canHurtPlayer()) {
		minecraft->textures->loadAndBindTexture("gui/icons.png");
		Tesselator& t = Tesselator::instance;
		t.beginOverride();
		t.colorABGR(0xffffffff);
		renderHearts();
		renderBubbles();
		t.endOverrideAndDraw();
	}

	if(minecraft->player->getSleepTimer() > 0) {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);

		renderSleepAnimation(screenWidth, screenHeight);

		glEnable(GL_ALPHA_TEST);
		glEnable(GL_DEPTH_TEST);
	}

	renderToolBar(a, ySlot, screenWidth);


	//font->drawShadow(APP_NAME, 2, 2, 0xffffffff);
	//font->drawShadow("This is a demo, not the finished product", 2, 10 + 2, 0xffffffff);
	#ifdef APPLE_DEMO_PROMOTION
		font->drawShadow("Demo version", 2, 0 + 2, 0xffffffff);
	#endif /*APPLE_DEMO_PROMOTION*/
	glEnable(GL_BLEND);
	unsigned int max = 10;
    bool isChatting = false;
	renderChatMessages(screenHeight, max, isChatting, font);
	renderPlayerList(screenWidth, font);
	if (!minecraft->options.hideGui) renderEffects(screenWidth, font, isTouchInterface);
#if !defined(RPI)
	renderOnSelectItemNameText(screenWidth, font, ySlot);
#endif
#if defined(RPI)
	renderDebugInfo();
#endif

//        glPopMatrix2();
//
//        glEnable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
	glEnable2(GL_ALPHA_TEST);
}

int Gui::getSlotIdAt(int x, int y) {
	int screenWidth = (int)(minecraft->width * InvGuiScale);
	int screenHeight = (int)(minecraft->height * InvGuiScale);
	x = (int)(x * InvGuiScale);
	y = (int)(y * InvGuiScale);

	if (y < (screenHeight - 16 - 3) || y > screenHeight)
		return -1;

	int xBase = 2 + screenWidth / 2 - getNumSlots() * 10;
	int xRel  = (x - xBase);
	if (xRel < 0)
		return -1;

	int slot = xRel / 20;
	return (slot >= 0 && slot < getNumSlots())? slot : -1;
}

bool Gui::isInside(int x, int y) {
	return getSlotIdAt(x, y) != -1;
}

int Gui::getNumSlots() {
	return _numSlots;
}

void Gui::flashSlot(int slotId) {
	_flashSlotId = slotId;
	_flashSlotStartTime = getTimeS();
}

void Gui::getSlotPos(int slot, int& posX, int& posY) {
	int screenWidth = (int)(minecraft->width * InvGuiScale);
	int screenHeight = (int)(minecraft->height * InvGuiScale);
	posX = screenWidth / 2 - getNumSlots() * 10 + slot * 20, 
	posY = screenHeight - 22;
}

RectangleArea Gui::getRectangleArea(int extendSide) {
	const int Spacing = 3;
	const float pCenterX   = 2.0f + (float)(minecraft->width / 2);
	const float pHalfWidth = (1.0f + (getNumSlots() * 10 + Spacing)) * Gui::GuiScale;
	const float pHeight    = (22 + Spacing) * Gui::GuiScale;

	if (extendSide < 0)
		return RectangleArea(0, (float)minecraft->height-pHeight, pCenterX+pHalfWidth+2, (float)minecraft->height);
	if (extendSide > 0)
		return RectangleArea(pCenterX-pHalfWidth, (float)minecraft->height-pHeight, (float)minecraft->width, (float)minecraft->height);
	
	return RectangleArea(pCenterX-pHalfWidth, (float)minecraft->height-pHeight, pCenterX+pHalfWidth+2, (float)minecraft->height);
}

RectangleArea Gui::getChatButtonArea() {
	const float w = 36 * Gui::GuiScale, h = 14 * Gui::GuiScale;
	const float x0 = (float)(minecraft->width / 2) - w / 2;
	return RectangleArea(x0, 4, x0 + w, 4 + h);
}

void Gui::handleClick(int button, int x, int y) {
	if (button != MouseAction::ACTION_LEFT)	return;

	int slot = getSlotIdAt(x, y);
	if (slot != -1)
	{
		if (slot == (getNumSlots()-1))
		{
			minecraft->screenChooser.setScreen(SCREEN_BLOCKSELECTION);
		}
		else
		{
			minecraft->player->inventory->selectSlot(slot);
			itemNameOverlayTime = 0;
		}
	}
}

void Gui::handleKeyPressed(int key)
{
	if (key == 99)
	{
		if (minecraft->player->inventory->selected > 0)
		{
			minecraft->player->inventory->selected--;
		}
	}
	else if (key == 4)
	{
		if (minecraft->player->inventory->selected < (getNumSlots() - 2))
		{
			minecraft->player->inventory->selected++;
		}
	}
	else if (key == 100)
	{
		minecraft->screenChooser.setScreen(SCREEN_BLOCKSELECTION);
	}
}

void Gui::tick() {
	if (overlayMessageTime > 0) overlayMessageTime--;
	tickCount++;
	if(itemNameOverlayTime < 2)
		itemNameOverlayTime += 1.0f / SharedConstants::TicksPerSecond;
	// The name over the hotbar was only shown for a tap on it (touch); on a
	// PC the hotbar changes by key or wheel, so watch the selection instead.
	if (minecraft->player && minecraft->player->inventory) {
		const int slot = minecraft->player->inventory->selected;
		ItemInstance* item = minecraft->player->inventory->getSelected();
		const int id = item ? item->id : 0;
		if (slot != lastSelectedSlot || id != lastSelectedId) {
			if (lastSelectedSlot != -1) itemNameOverlayTime = 0;
			lastSelectedSlot = slot;
			lastSelectedId = id;
		}
	}
	for (unsigned int i = 0; i < guiMessages.size(); i++) {
	    guiMessages.at(i).ticks++;
	}

    if (!minecraft->isCreativeMode())
        tickItemDrop();
}

void Gui::addMessage(const std::string& _string, int color) {
	if (!minecraft->font)
		return;

	std::string string = _string;
	while (minecraft->font->width(string) > MAX_MESSAGE_WIDTH) {
		unsigned int i = 1;
		while (i < string.length() && minecraft->font->width(string.substr(0, i + 1)) <= MAX_MESSAGE_WIDTH) {
			i++;
		}
		addMessage(string.substr(0, i), color);
		string = string.substr(i);
	}
	GuiMessage message;
	message.message = string;
	message.ticks = 0;
	message.color = color;
	guiMessages.insert(guiMessages.begin(), message);
	while (guiMessages.size() > 30) {
		guiMessages.pop_back();
	}
}

void Gui::setNowPlaying(const std::string& string) {
	overlayMessageString = "Now playing: " + string;
	overlayMessageTime = 20 * 3;
	animateOverlayMessageColor = true;
}

void Gui::displayClientMessage(const std::string& messageId) {
	//Language language = Language.getInstance();
	//std::string languageString = language.getElement(messageId);
	// Chat lines arrive here too, already in "<name> message" form, so no label.
	addMessage(messageId);
}

/// The black squid's ink, over the world and under the HUD: big blots
/// (gui/ink.png) that fade in their last two seconds.
void Gui::renderInk(int w, int h) {
	const std::vector<MobEffectInstance>& effects = minecraft->player->activeEffects;
	int left = 0;
	for (unsigned int i = 0; i < effects.size(); ++i)
		if (effects[i].id == MobEffects::Ink) left = effects[i].duration;
	if (left <= 0) return;
	const float alpha = left >= 40 ? 1.0f : left / 40.0f;
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f2(1, 1, 1, alpha);
	minecraft->textures->loadAndBindTexture("gui/ink.png");
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.vertexUV(0, (float)h, -90, 0, 1);
	t.vertexUV((float)w, (float)h, -90, 1, 1);
	t.vertexUV((float)w, 0, -90, 1, 0);
	t.vertexUV(0, 0, -90, 0, 0);
	t.draw();
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
	glColor4f2(1, 1, 1, 1);
}

void Gui::renderVignette(float br, int w, int h) {
	br = 1 - br;
	if (br < 0) br = 0;
	if (br > 1) br = 1;
	tbr += (br - tbr) * 0.01f;

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glBlendFunc2(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glColor4f2(tbr, tbr, tbr, 1);
	minecraft->textures->loadAndBindTexture("misc/vignette.png");

	Tesselator& t = Tesselator::instance;
	t.begin();
	t.vertexUV(0, (float)h, -90, 0, 1);
	t.vertexUV((float)w, (float)h, -90, 1, 1);
	t.vertexUV((float)w, 0, -90, 1, 0);
	t.vertexUV(0, 0, -90, 0, 0);
	t.draw();
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
	glColor4f2(1, 1, 1, 1);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Gui::renderSlot(int slot, int x, int y, float a) {
	ItemInstance* item = minecraft->player->inventory->getItem(slot);
	if (!item) {
		//LOGW("Warning: item @ Gui::renderSlot is NULL\n");
		return;
	}

	const bool fancy = true;
	ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, item, (float)x, (float)y, fancy);
}

void Gui::renderSlotText( const ItemInstance* item, float x, float y, bool hasFinite, bool shadow )
{
	//if (!item || item->getItem()->getMaxStackSize() <= 1) {
	if (item->count <= 1) {
		return;
	}

	int c = item->count;

	char buffer[4] = {0,0,0,0};
	if (hasFinite)
		itemCountItoa(buffer, c);
	else
		buffer[0] = (char)157;

	//LOGI("slot: %d - %s\n", slot, buffer);
	if (shadow)
		minecraft->font->drawShadow(buffer, x, y, item->count>0?0xffcccccc:0x60cccccc);
	else
		minecraft->font->draw(buffer, x, y, item->count>0?0xffcccccc:0x60cccccc);
}

void Gui::inventoryUpdated() {
	_inventoryNeedsUpdate = true;
}

void Gui::onGraphicsReset() {
    inventoryUpdated();
}

void Gui::texturesLoaded( Textures* textures ) {
	//_slotFont = new Font(&minecraft->options, "gui/gui_blocks.png", textures, 0, 504, 10, 1, '0');
}

void Gui::onConfigChanged( const Config& c ) {
	Tesselator& t = Tesselator::instance;
	t.begin();

	//
	// Create outer feedback circle
	//
#ifdef ANDROID
	const float mm = 12;
#else
	const float mm = 12;
#endif
	const float maxRadius = minecraft->pixelCalcUi.millimetersToPixels(mm);
	const float radius = Mth::Min(80.0f/2, maxRadius);
	//LOGI("radius, maxradius: %f, %f\n", radius, maxRadius);
	const float radiusInner = radius * 0.95f;

	const int steps = 24;
	const float fstep = Mth::TWO_PI / steps;
	for (int i = 0; i < steps; ++i) {
		float a = i * fstep;;
		float b = a + fstep;

		float aCos = Mth::cos(a);
		float bCos = Mth::cos(b);
		float aSin = Mth::sin(a);
		float bSin = Mth::sin(b);
		float x00 = radius * aCos;
		float x01 = radiusInner * aCos;
		float x10 = radius * bCos;
		float x11 = radiusInner * bCos;
		float y00 = radius * aSin;
		float y01 = radiusInner * aSin;
		float y10 = radius * bSin;
		float y11 = radiusInner * bSin;

		t.vertexUV(x01, y01, 0, 0, 1);
		t.vertexUV(x11, y11, 0, 1, 1);
		t.vertexUV(x10, y10, 0, 1, 0);
		t.vertexUV(x00, y00, 0, 0, 0);
	}
	rcFeedbackOuter = t.end(true, rcFeedbackOuter.vboId);

	//
	// Create the inner feedback ring
	//
	t.begin(GL_TRIANGLE_FAN);
	t.vertex(0, 0, 0);
	for (int i = 0; i < steps + 1; ++i) {
		float a = -i * fstep;
		float xx = radiusInner * Mth::cos(a);
		float yy = radiusInner * Mth::sin(a);
		t.vertex(xx, yy, 0);
		//LOGI("x,y: %f, %f\n", xx, yy);
	}
	rcFeedbackInner = t.end(true, rcFeedbackInner.vboId);

	if (c.minecraft->useTouchscreen()) {
		// I'll bump this up to 6.
		int num = 6; // without "..." dots
		if (!c.minecraft->options.isJoyTouchArea && c.width > 480) {
			while (num < Inventory::MAX_SELECTION_SIZE - 1) {
				int x0, x1, y;
				getSlotPos(0, x0, y);
				getSlotPos(num, x1, y);
				int width = x1 - x0;
				float leftoverPixels = c.width - c.guiScale*width;
				if (c.pixelCalc.pixelsToMillimeters(leftoverPixels) < 80)
					break;
				num++;
			}
		}
		_numSlots = num;
#if defined(__APPLE__)
		_numSlots = Mth::Min(7, _numSlots);
#endif
	} else {
		_numSlots = Inventory::MAX_SELECTION_SIZE; // Xperia Play
	}
	MAX_MESSAGE_WIDTH = c.guiWidth;
}

float Gui::floorAlignToScreenPixel(float v) {
	return (int)(v * Gui::GuiScale) * Gui::InvGuiScale;
}

int Gui::itemCountItoa( char* buffer, int count )
{
	if (count < 0)
		return 0;

	if (count < 10) { // 1 digit
		buffer[0] = '0' + count;
		buffer[1] = 0;
		return 1;
	} else if (count < 100) { // 2 digits
		int digit = count/10;
		buffer[0] = '0' + digit;
		buffer[1] = '0' + count - digit*10;
		buffer[2] = 0;
	} else { // 3 digits -> "99+"
		buffer[0] = buffer[1] = '9';
		buffer[2] = '+';
		buffer[3] = 0;
		return 3;
	}
	return 2;
}

void Gui::tickItemDrop()
{
	// Handle item drop
	static bool isCurrentlyActive = false;
	isCurrentlyActive = false;
	// While the mouse is grabbed the cursor is hidden but parked where it was
	// last seen, and holding the button to dig over the hotbar's spot dropped
	// the slot under it. A grabbed cursor is the middle of the screen: no
	// hotbar there. (Touch builds never grab.)
	if (minecraft->mouseGrabbed) {
		_currentDropSlot = -1;
		_currentDropTicks = -1;
		return;
	}
	if (Mouse::isButtonDown(MouseAction::ACTION_LEFT)) {
		int slot = getSlotIdAt(Mouse::getX(), Mouse::getY());
		if (slot >= 0 && slot < getNumSlots()-1) {
			if (slot != _currentDropSlot) {
				_currentDropTicks = 0;
				_currentDropSlot = slot;
			}
			isCurrentlyActive = true;
			if ((_currentDropTicks += 1.0f) >= DropTicks) {
				minecraft->player->inventory->dropSlot(slot, false);
				minecraft->level->playSound(minecraft->player, "random.pop", 0.3f, 1);
				isCurrentlyActive = false;
			}
		}
	}
	if (!isCurrentlyActive) {
		_currentDropSlot = -1;
		_currentDropTicks = -1;
	}
}

void Gui::postError( int errCode )
{
	static std::set<int> posted;
	if (posted.find(errCode) != posted.end())
		return;

	posted.insert(errCode);

	std::stringstream s;
	s << "Something went wrong! (errcode " << errCode << ")\n";
	addMessage(s.str());
}

void Gui::setScissorRect( const IntRectangle& bbox )
{
	GLuint x = (GLuint)(GuiScale * bbox.x);
	GLuint y = minecraft->height - (GLuint)(GuiScale * (bbox.y + bbox.h));
	GLuint w = (GLuint)(GuiScale * bbox.w);
	GLuint h = (GLuint)(GuiScale * bbox.h);
	glScissor(x, y, w, h);
}

float Gui::cubeSmoothStep(float percentage, float min, float max) {
	//percentage = percentage * percentage;
	//return (min * percentage) + (max * (1 - percentage));
	return (percentage) * (percentage) * (3 - 2 * (percentage));
}

void Gui::renderProgressIndicator( const bool isTouchInterface, const int screenWidth, const int screenHeight, float a ) {
	ItemInstance* currentItem = minecraft->player->inventory->getSelected();
	bool bowEquipped = currentItem != NULL ? currentItem->getItem() == Item::bow : false;
	bool itemInUse = currentItem != NULL ? currentItem->getItem() == minecraft->player->getUseItem()->getItem() : false;
	if (!isTouchInterface || minecraft->options.isJoyTouchArea || (bowEquipped && itemInUse)) {
		minecraft->textures->loadAndBindTexture("gui/icons.png");
		glEnable(GL_BLEND);
		glBlendFunc2(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
		blit(screenWidth/2 - 8, screenHeight/2 - 8, 0, 0, 16, 16);
		glDisable(GL_BLEND);
	} else if(!bowEquipped) {
		const float tprogress = minecraft->gameMode->destroyProgress;
		const float alpha = Mth::clamp(minecraft->inputHolder->alpha, 0.0f, 1.0f);
		//LOGI("alpha: %f\n", alpha);

		if (tprogress <= 0 && minecraft->inputHolder->alpha >= 0) {
			glDisable2(GL_TEXTURE_2D);
			glEnable2(GL_BLEND);
			glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			if (minecraft->hitResult.isHit())
				glColor4f2(1, 1, 1, 0.8f * alpha);
			else
				glColor4f2(1, 1, 1, Mth::Min(0.4f, alpha*0.4f));

			//LOGI("alpha2: %f\n", alpha);
			const float x = InvGuiScale * minecraft->inputHolder->mousex;
			const float y = InvGuiScale * minecraft->inputHolder->mousey;
			glTranslatef2(x, y, 0);
			drawArrayVT(rcFeedbackOuter.vboId, rcFeedbackOuter.vertexCount, 24);
			glTranslatef2(-x, -y, 0);

			glEnable2(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
		} else if (tprogress > 0) {
			const float oProgress = minecraft->gameMode->oDestroyProgress;
			const float progress = 0.5f * (oProgress + (tprogress - oProgress) * a);

			//static Stopwatch w;
			//w.start();

			glDisable2(GL_TEXTURE_2D);
			glColor4f2(1, 1, 1, 0.8f * alpha);
			glEnable(GL_BLEND);
			glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			const float x = InvGuiScale * minecraft->inputHolder->mousex;
			const float y = InvGuiScale * minecraft->inputHolder->mousey;
			glPushMatrix2();
			glTranslatef2(x, y, 0);
			drawArrayVT(rcFeedbackOuter.vboId, rcFeedbackOuter.vertexCount, 24);
			glScalef2(0.5f + progress, 0.5f + progress, 1);
			//glDisable2(GL_CULL_FACE);
			glColor4f2(1, 1, 1, 1);
			glBlendFunc2(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
			drawArrayVT(rcFeedbackInner.vboId, rcFeedbackInner.vertexCount, 24, GL_TRIANGLE_FAN);
			glPopMatrix2();

			glDisable(GL_BLEND);
			glEnable2(GL_TEXTURE_2D);

			//w.stop();
			//w.printEvery(100, "feedback-r ");
		}
	}
}

void Gui::renderHearts() {
	bool blink = (minecraft->player->invulnerableTime / 3) % 2 == 1;
	if (minecraft->player->invulnerableTime < 10) blink = false;
	int h = minecraft->player->health;
	int oh = minecraft->player->lastHealth;
	random.setSeed(tickCount * 312871);

	// Centred over the hotbar, on its top edge, instead of the screen's corner.
	// The armour row goes above them rather than beside them: beside, the pair
	// would no longer fit the hotbar's width.
	int slotX, slotY;
	getSlotPos(0, slotX, slotY);
	const int screenWidth = (int)(minecraft->width * InvGuiScale);
	const int heartsY = slotY - 11;
	int xx = screenWidth / 2 - (Player::MAX_HEALTH / 2 * 8 + 1) / 2;

	int armor = minecraft->player->getArmorValue();
	// Poisoned: the hearts go dark yellow (icons 81/90 on row 18). Inked by a
	// black squid: every heart is a black one with a "?" (99), so the health
	// cannot be read while it lasts.
	const bool poisoned = minecraft->player->hasEffect(MobEffects::Poison);
	const bool inked = minecraft->player->hasEffect(MobEffects::Ink);
	for (int i = 0; i < Player::MAX_HEALTH / 2; i++) {
		int yo = heartsY;
		int ip2 = i + i + 1;

		if (armor > 0) {
		    int xo = xx + i * 8;
		    const int yo = heartsY - 10;
		    if (ip2 < armor) blit(xo, yo, 16 + 2 * 9, 9 * 1, 9, 9);
		    else if (ip2 == armor) blit(xo, yo, 16 + 4 * 9, 9 * 1, 9, 9);
		    else if (ip2 > armor) blit(xo, yo, 16 + 0 * 9, 9 * 1, 9, 9);
		}

		int bg = 0;
		if (blink) bg = 1;
		int xo = xx + i * 8;
		if (h <= 4) {
			yo = yo + random.nextInt(2) - 1;
		}
		if (inked) {
			blit(xo, heartsY, 99, 18, 9, 9);
			continue;
		}
		blit(xo, yo, 16 + bg * 9, 9 * 0, 9, 9);
		if (poisoned) {
			if (ip2 < h) blit(xo, yo, 81, 18, 9, 9);
			else if (ip2 == h) blit(xo, yo, 90, 18, 9, 9);
			continue;
		}
		if (blink) {
			if (ip2 < oh) blit(xo, yo, 16 + 6 * 9, 9 * 0, 9, 9);
			else if (ip2 == oh) blit(xo, yo, 16 + 7 * 9, 9 * 0, 9, 9);
		}
		if (ip2 < h) blit(xo, yo, 16 + 4 * 9, 9 * 0, 9, 9);
		else if (ip2 == h) blit(xo, yo, 16 + 5 * 9, 9 * 0, 9, 9);
	}
}

void Gui::renderBubbles() {
	// The Aquarium's pressure: a row like the air's, over it, that fills up.
	if (minecraft->player->pressure > 0) {
		int slotX, slotY;
		getSlotPos(0, slotX, slotY);
		const int screenWidth = (int)(minecraft->width * InvGuiScale);
		int yo = slotY - 11 - (minecraft->player->getArmorValue() > 0 ? 20 : 10)
			- (minecraft->player->isUnderLiquid(Material::water) ? 10 : 0);
		const int bx = screenWidth / 2 - (Player::MAX_HEALTH / 2 * 8 + 1) / 2;
		const bool full = minecraft->player->pressure >= Player::MAX_PRESSURE;
		for (int i = 0; i < minecraft->player->pressure; i++) {
			int jitter = full ? random.nextInt(2) - 1 : 0;
			blit(bx + i * 8, yo + jitter, 72, 18, 9, 9);
		}
	}
	if (minecraft->player->isUnderLiquid(Material::water)) {
		// Above the hearts (and the armour row over them), on the hotbar's centre.
		int slotX, slotY;
		getSlotPos(0, slotX, slotY);
		const int screenWidth = (int)(minecraft->width * InvGuiScale);
		int yo = slotY - 11 - (minecraft->player->getArmorValue() > 0 ? 20 : 10);
		const int bx = screenWidth / 2 - (Player::MAX_HEALTH / 2 * 8 + 1) / 2;
		int count = (int) std::ceil((minecraft->player->airSupply - 2) * 10.0f / Player::TOTAL_AIR_SUPPLY);
		int extra = (int) std::ceil((minecraft->player->airSupply) * 10.0f / Player::TOTAL_AIR_SUPPLY) - count;
		for (int i = 0; i < count + extra; i++) {
			int xo =  bx + i * 8;
			if (i < count) blit(xo, yo, 16, 9 * 2, 9, 9);
			else blit(xo, yo, 16 + 9, 9 * 2, 9, 9);
		}
	}
}

static OffsetPosTranslator posTranslator;
void Gui::onLevelGenerated() {
	if (Level* level = minecraft->level) {
		Pos p = level->getSharedSpawnPos();
		posTranslator = OffsetPosTranslator((float)-p.x, (float)-p.y, (float)-p.z);
	}
}

void Gui::renderDebugInfo() {
	static char buf[256];
	float xx = minecraft->player->x;
	float yy = minecraft->player->y - minecraft->player->heightOffset;
	float zz = minecraft->player->z;
	posTranslator.to(xx, yy, zz);
	sprintf(buf, "pos: %3.1f, %3.1f, %3.1f\n", xx, yy, zz);
	Tesselator& t = Tesselator::instance;
	t.beginOverride();
	t.scale2d(InvGuiScale, InvGuiScale);
	minecraft->font->draw(buf, 2, 2, 0xffffff);
	t.resetScale();
	t.endOverrideAndDraw();
}

void Gui::renderSleepAnimation( const int screenWidth, const int screenHeight ) {
	int timer = minecraft->player->getSleepTimer();
	float amount = (float) timer / (float) Player::SLEEP_DURATION;
	if (amount > 1) {
		// waking up
		amount = 1.0f - ((float) (timer - Player::SLEEP_DURATION) / (float) Player::WAKE_UP_DURATION);
	}

	int color = (int) (220.0f * amount) << 24 | (0x101020);
	fill(0, 0, screenWidth, screenHeight, color);
}

void Gui::renderOnSelectItemNameText( const int screenWidth, Font* font, int ySlot ) {
	if(itemNameOverlayTime < 1.0f) {
		ItemInstance* item = minecraft->player->inventory->getSelected();
		if(item != NULL) {
			float x = float(screenWidth / 2 - font->width(item->getName()) / 2);
			float y = float(ySlot - 22);
			int alpha = 255;
			if(itemNameOverlayTime > 0.75) {
				float time = 0.25f - (itemNameOverlayTime - 0.75f);
				float percentage = cubeSmoothStep(time *  4, 0.0f, 1.0f);
				alpha = int(percentage * 255);
			}
			if(alpha != 0)
				font->drawShadow(item->getName(), x, y, 0x00ffffff + (alpha << 24));
		}
	}
}

/** Potion effects in the top right corner: Java's icon (gui/effects.png, its
    inventory.png grid) with the time left under it. In its last ten seconds
    the icon blinks, as the HUD's does in Java. */
void Gui::renderEffects(const int screenWidth, Font* font, bool isTouchInterface) {
	const std::vector<MobEffectInstance>& effects = minecraft->player->activeEffects;
	if (effects.empty()) return;

	const int size = 18, gap = 4;
	int x = screenWidth - size - 4;
	// Below the touch screen's pause button.
	const int y = isTouchInterface ? 26 : 4;

	glEnable(GL_BLEND);
	glColor4f2(1, 1, 1, 1);
	for (unsigned int i = 0; i < effects.size(); ++i, x -= size + gap + 4) {
		const MobEffects::Info* info = MobEffects::get(effects[i].id);
		if (!info || info->iconX < 0) continue;
		const int left = effects[i].duration;
		const bool hidden = left <= 200 && (left / 5) % 2 == 0;
		if (!hidden) {
			minecraft->textures->loadAndBindTexture("gui/effects.png");
			blit(x, y, info->iconX * size, info->iconY * size, size, size);
		}
		const int seconds = left / SharedConstants::TicksPerSecond;
		char text[16];
		sprintf(text, "%d:%02d", seconds / 60, seconds % 60);
		const int tw = font->width(text);
		font->drawShadow(text, (float) (x + size / 2 - tw / 2), (float) (y + size + 1), 0xffffffff);
	}
}

/// Hold Tab: who is in the world, like Java's list. Host first, as the host sent it.
void Gui::renderPlayerList(const int screenWidth, Font* font) {
	if (!Keyboard::isKeyDown(9))
		return;
	renderPlayerPanel(screenWidth / 2, 14, font);
}

void Gui::renderPlayerPanel(int centerX, int y0, Font* font) {
	std::vector<std::string> names = minecraft->playerNames;
	if (names.empty() && minecraft->player)
		names.push_back(minecraft->player->name);

	std::string world = minecraft->worldTitle;
	if (world.empty() && minecraft->level && !minecraft->level->isClientSide)
		world = minecraft->level->getLevelData()->getLevelName();
	const std::string& motd = minecraft->worldMotd;
	std::ostringstream count;
	count << "Players (" << names.size() << "/10)";

	int width = 120;
	for (unsigned int i = 0; i < names.size(); ++i)
		width = std::max(width, font->width(names[i]) + 24);
	width = std::max(width, font->width(world) + 16);
	width = std::max(width, font->widthFormatted(motd) + 16);

	const int rowHeight = 10;
	int headerRows = 1 + (world.empty() ? 0 : 1) + (motd.empty() ? 0 : 1);
	const int header = 4 + headerRows * rowHeight + 2;
	const int height = header + (int) names.size() * rowHeight + 2;
	const int x0 = centerX - width / 2;

	fill(x0, y0, x0 + width, y0 + height, 0x80000000);
	int y = y0 + 4;
	if (!world.empty()) {
		font->drawShadow(world, (float) (x0 + (width - font->width(world)) / 2), (float) y, 0xffffffff);
		y += rowHeight;
	}
	if (!motd.empty()) {
		font->drawFormattedShadow(motd, (float) (x0 + (width - font->widthFormatted(motd)) / 2), (float) y, 0xffaaaaaa);
		y += rowHeight;
	}
	font->drawShadow(count.str(), (float) (x0 + (width - font->width(count.str())) / 2), (float) y, 0xffffffff);
	for (unsigned int i = 0; i < names.size(); ++i) {
		const int ry = y0 + header + (int) i * rowHeight;
		fill(x0 + 4, ry - 1, x0 + width - 4, ry + rowHeight - 2, 0x30ffffff);
		// The first is the owner of the world.
		// (Not before the host's list has arrived: then the only name is ours.)
		const bool owner = (i == 0) && !minecraft->playerNames.empty();
		font->drawShadow(names[i], (float) (x0 + 8), (float) ry, owner ? 0xffffff55 : 0xffffffff);
	}
}

void Gui::renderChatMessages( const int screenHeight, unsigned int max, bool isChatting, Font* font ) {
	//        if (minecraft.screen instanceof ChatScreen) {
	//            max = 20;
	//            isChatting = true;
	//        }
	//
	//        glEnable(GL_BLEND);
	//        glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//        glDisable(GL_ALPHA_TEST);
	//
	//        glPushMatrix2();
	//        glTranslatef2(0, screenHeight - 48, 0);
	//        // glScalef2(1.0f / ssc.scale, 1.0f / ssc.scale, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int baseY = screenHeight - 48;
	for (unsigned int i = 0; i < guiMessages.size() && i < max; i++) {
		if (guiMessages.at(i).ticks < 20 * 10 || isChatting) {
			float t = guiMessages.at(i).ticks / (20 * 10.0f);
			t = 1 - t;
			t = t * 10;
			if (t < 0) t = 0;
			if (t > 1) t = 1;
			t = t * t;
			int alpha = (int) (255 * t);
			if (isChatting) alpha = 255;

			if (alpha > 0) {
				const float x = 2;
				const float y = (float)(baseY - i * 9);
				std::string msg = guiMessages.at(i).message;
				this->fill(x, y - 1, x + MAX_MESSAGE_WIDTH, y + 8, (alpha / 2) << 24);
				glEnable(GL_BLEND);

				// "<owner> line": the owner's name is yellow.
				std::string owner = !minecraft->playerNames.empty() ? minecraft->playerNames[0]
					: ((minecraft->player && !minecraft->level->isClientSide) ? minecraft->player->name : std::string());
				const std::string tag = "<" + owner + ">";
				if (!owner.empty() && msg.compare(0, tag.size(), tag) == 0) {
					font->drawShadow(tag, x, y, 0xffff55 + (alpha << 24));
					// What was said takes & codes; the name never does.
					font->drawFormattedShadow(msg.substr(tag.size()), x + font->width(tag), y, 0xffffff + (alpha << 24));
				} else {
					font->drawFormattedShadow(msg, x, y, guiMessages.at(i).color + (alpha << 24));
				}
			}
		}
	}
}

void Gui::renderToolBar( float a, int ySlot, const int screenWidth ) {
	glColor4f2(1, 1, 1, .5);
	minecraft->textures->loadAndBindTexture("gui/gui.png");

	Inventory* inventory = minecraft->player->inventory;

	int xBase, yBase;
	getSlotPos(0, xBase, yBase);
	const float baseItemX = (float)xBase + 3;
	const int slotsWidth = 20 * getNumSlots();
	// Left + right side of the selection bar
	blit(xBase, yBase, 0, 0, slotsWidth, 22);
	blit(xBase + slotsWidth, yBase, 180, 0, 2, 22);

	if (_currentDropSlot >= 0 && inventory->getItem(_currentDropSlot)) {
		int x = xBase + 3 +  _currentDropSlot * 20;
		int color = 0x8000ff00;
		int yy = (int)(17.0f * (_currentDropTicks + a) / DropTicks);

		if (_currentDropTicks >= 3) {
			glColor4f2(0, 1, 0, 0.5f);
		}
		fill(x, ySlot+16-yy, x+16, ySlot+16, color);
	}
	blit(xBase-1 + 20*inventory->selected, yBase - 1, 0, 22, 24, 22);
	glColor4f2(1, 1, 1, 1);

	// Flash a slot background
	if (_flashSlotId >= 0) {
		const float since = getTimeS() - _flashSlotStartTime;
		if (since > 0.2f) _flashSlotId = -1;
		else {
			int x = screenWidth / 2 - getNumSlots() * 10 + _flashSlotId * 20 + 2;
			int color = 0xffffff + (((int)(/*0x80 * since +*/ 0x51 - 0x50 * Mth::cos(10 * 6.28f * since))) << 24);
			//LOGI("Color: %.8x\n", color);
			fill(x, ySlot, x+16, ySlot+16, color);
		}
	}
	glColor4f2(1, 1, 1, 1);

	//static Stopwatch w;
	//w.start();

	Tesselator& t = Tesselator::instance;
	t.beginOverride();

	float x = baseItemX;
	for (int i = 0; i < getNumSlots()-1; i++) {
		renderSlot(i, (int)x, ySlot, a);
		x += 20;
	}
	_inventoryNeedsUpdate = false;
	//_inventoryRc = t.end(_inventoryRc.vboId);

	//drawArrayVTC(_inventoryRc.vboId, _inventoryRc.vertexCount);

	//renderSlotWatch.stop();
	//renderSlotWatch.printEvery(100, "Render slots:");

	//int x = screenWidth / 2 + getNumSlots() * 10 + (getNumSlots()-1) * 20 + 2;
	blit(screenWidth / 2 + 10 * getNumSlots() - 20 + 4, ySlot + 6, 242, 252, 14, 4, 14, 4);

	minecraft->textures->loadAndBindTexture("gui/gui_blocks.png");
	t.endOverrideAndDraw();

	// Render damaged items (@todo: investigate if it's faster by drawing in same batch)
	glDisable2(GL_DEPTH_TEST);
	glDisable2(GL_TEXTURE_2D);
	t.beginOverride();
	x = baseItemX;
	for (int i = 0; i < getNumSlots()-1; i++) {
		ItemRenderer::renderGuiItemDecorations(minecraft->player->inventory->getItem(i), x, (float)ySlot);
		x += 20;
	}
	t.endOverrideAndDraw();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	//w.stop();
	//w.printEvery(100, "gui-slots");

	// Draw count
	//Tesselator& t = Tesselator::instance;
	glPushMatrix2();
	glScalef2(InvGuiScale + InvGuiScale, InvGuiScale + InvGuiScale, 1);
	const float k = 0.5f * GuiScale;

	t.beginOverride();
	if (minecraft->gameMode->isSurvivalType()) {
		x = baseItemX;
		for (int i = 0; i < getNumSlots()-1; i++) {
			ItemInstance* item = minecraft->player->inventory->getItem(i);
			if (item && item->count >= 0)
				renderSlotText(item, k*x, k*ySlot + 1, true, true);
			x += 20;
		}
	}
	minecraft->textures->loadAndBindTexture("font/default8.png");
	t.endOverrideAndDraw();

	glPopMatrix2();
}
