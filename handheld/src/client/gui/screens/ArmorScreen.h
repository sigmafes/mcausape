#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ArmorScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ArmorScreen_H__

#include "BaseContainerScreen.h"

#include "../components/InventoryPane.h"
#include "../components/Button.h"

class Font;
class CItem;
class Textures;
class NinePatchLayer;
class Tesselator;

class ArmorScreen:	public Screen,
					public Touch::IInventoryPaneCallback
{
	typedef Screen super;
	typedef std::vector<CItem*> ItemList;

	static const int NUM_ARMORBUTTONS = 4;
	// Hotbar row shown at the bottom of this screen: the 9 selection slots.
	static const int NUM_HOTBARBUTTONS = 9;
public:
    ArmorScreen();
	~ArmorScreen();

    void init();
	void setupPositions();

	void tick();
	void render(int xm, int ym, float a);
	bool renderGameBehind();
	void buttonClicked(Button* button);
	void keyPressed(int eventKey);
	void mouseWheel(int dy);

	// IInventoryPaneCallback
	bool addItem(const Touch::InventoryPane* pane, int itemId);
	bool isAllowed( int slot );
	std::vector<const ItemInstance*> getItems( const Touch::InventoryPane* forPane );
private:
	void renderPlayer(float xo, float yo, float ss);

	void setupInventoryPane();
	void updateItems();

	void drawSlotItemAt(Tesselator& t, int slot, const ItemInstance* item, int x, int y);
	void drawHotbarSlotAt(Tesselator& t, int slot, int x, int y, bool selected);
	ItemInstance moveOver(const ItemInstance* item, int maxCount);
	void takeAndClearSlot( int slot );
	void handleRenderPane(Touch::InventoryPane* pane, Tesselator& t, int xm, int ym, float a);
	bool canMoveToSlot(int slot, const ItemInstance* item);
	bool moveToHotbar(int itemIndex);
	ItemList _items;

	std::string currentItemDesc;
	ItemInstance burnResult;
	float descWidth;
	ImageButton btnClose;

	BlankButton btnArmor0;
	BlankButton btnArmor1;
	BlankButton btnArmor2;
	BlankButton btnArmor3;
	BlankButton* armorButtons[4];

	// Invisible click targets for the hotbar row; the slots themselves are drawn
	// by drawHotbarSlotAt, exactly like the armour slots above.
	BlankButton* hotbarButtons[NUM_HOTBARBUTTONS];

	Touch::THeader bHeader;
	Touch::TButton bCraft;

	Touch::InventoryPane* inventoryPane;
	IntRectangle inventoryPaneRect;
	IntRectangle guiPlayerBgRect;
	
	std::vector<const ItemInstance*> armorItems;
	// Inventory slot each entry of armorItems came from, so a click can move it
	// back into the hotbar.
	std::vector<int> armorItemSlots;
	bool doRecreatePane;

	// GUI elements such as 9-Patches
	NinePatchLayer* guiBackground;
	NinePatchLayer* guiSlot;
	NinePatchLayer* guiPaneFrame;
	NinePatchLayer* guiPlayerBg;
	Player* player;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ArmorScreen_H__*/
