#ifndef NET_MINECRAFT_CLIENT_GUI__Gui_H__
#define NET_MINECRAFT_CLIENT_GUI__Gui_H__

//package net.minecraft.client.gui;

#include "GuiComponent.h"
#include "Font.h"
#include "../player/input/touchscreen/TouchAreaModel.h"
#include "../renderer/RenderChunk.h"
#include "../../util/Random.h"
#include "../IConfigListener.h"

class Minecraft;
class ItemInstance;
class Textures;
class Tesselator;
struct IntRectangle;

struct GuiMessage
{
	std::string message;
	int ticks;
	int color;	///< 0xRRGGBB; white unless the line asked for one
};

typedef std::vector<GuiMessage> GuiMessageList;

class Gui: public GuiComponent, IConfigListener
{
public:
    Gui(Minecraft* minecraft);
	~Gui();

	int getSlotIdAt(int x, int y);
	void flashSlot(int slotId);
	bool isInside(int x, int y);
	RectangleArea getRectangleArea(int extendSide);
	/// Touch only: the Chat button, top centre, in screen pixels.
	RectangleArea getChatButtonArea();
	void getSlotPos(int slot, int& posX, int& posY);
	int getNumSlots();

	void handleClick(int button, int x, int y);
	void handleKeyPressed( int key );

	void tick();
	void render(float a, bool mouseFree, int xMouse, int yMouse);

	void renderToolBar( float a, int ySlot, const int screenWidth );

	void renderChatMessages( const int screenHeight, unsigned int max, bool isChatting, Font* font );
	void renderPlayerList( const int screenWidth, Font* font );
public:
	/// The player list's panel: world, motd, "Players X/10", the names (the
	/// host's in yellow). Centred on centerX, from y0. Tab, and the touch pause menu.
	void renderPlayerPanel(int centerX, int y0, Font* font);
	void renderEffects( const int screenWidth, Font* font, bool isTouchInterface );

	void renderOnSelectItemNameText( const int screenWidth, Font* font, int ySlot );

	void renderSleepAnimation( const int screenWidth, const int screenHeight );

	void renderBubbles();
	void renderHearts();
	void renderDebugInfo();

	void renderProgressIndicator( const bool isTouchInterface, const int screenWidth, const int screenHeight, float a );

    void addMessage(const std::string& string, int color = 0xffffff);
	void postError(int errCode);

    void onGraphicsReset();
	void inventoryUpdated();

	void setNowPlaying(const std::string& string);
	void displayClientMessage(const std::string& messageId);
	void renderSlotText(const ItemInstance* item, float x, float y, bool hasFinite, bool shadow);
	void texturesLoaded( Textures* textures );

	void onConfigChanged(const Config& config);
	void onLevelGenerated();

	void setScissorRect(const IntRectangle& rect);

	static float floorAlignToScreenPixel(float);
	static int itemCountItoa(char* buf, int count);
private:
	void renderVignette(float br, int w, int h);
	void renderSlot(int slot, int x, int y, float a);
	void tickItemDrop();
	float cubeSmoothStep(float percentage, float min, float max);
public:
	float progress;
	std::string selectedName;
	static float InvGuiScale;
	static float GuiScale;

private:
	int MAX_MESSAGE_WIDTH;
	//ItemRenderer itemRenderer;
	GuiMessageList guiMessages;
	Random random;

	Minecraft* minecraft;
	int tickCount;
	float itemNameOverlayTime;
	// What was in hand last tick: a change shows its name again, however it
	// came (number keys, mouse wheel, a tap on the hotbar).
	int lastSelectedSlot;
	int lastSelectedId;
	std::string overlayMessageString;
	int overlayMessageTime;
	bool animateOverlayMessageColor;

	float tbr;

	RenderChunk _inventoryRc;
	bool _inventoryNeedsUpdate;
	
	int _flashSlotId;
	float _flashSlotStartTime;

	Font* _slotFont;
	int _numSlots;

	RenderChunk rcFeedbackOuter;
	RenderChunk rcFeedbackInner;

	// For dropping
	static const float DropTicks;
	float  _currentDropTicks;
	int    _currentDropSlot;
};

#endif /*NET_MINECRAFT_CLIENT_GUI__Gui_H__*/
