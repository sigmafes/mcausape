#ifndef _BREWINGSTANDSCREEN_H__
#define _BREWINGSTANDSCREEN_H__

#include "BaseContainerScreen.h"

/* The brewing stand's screen (docs/brewing-plan.md): FurnaceScreen with a
   fourth slot for the bottles. The inventory on the left lists what fits the
   selected slot; the fire is the fuel left, the arrow the brew's progress,
   both the furnace's pictures. */

#include "../components/InventoryPane.h"
#include "../components/Button.h"

class Font;
class CItem;
class Textures;
class NinePatchLayer;
class Tesselator;
class BrewingStandTileEntity;

class BrewingStandScreen:	public BaseContainerScreen,
						public Touch::IInventoryPaneCallback
{
	typedef BaseContainerScreen super;
	typedef std::vector<CItem*> ItemList;
public:
    BrewingStandScreen(Player* player, BrewingStandTileEntity* stand);
	~BrewingStandScreen();

    void init();
	void setupPositions();

	void tick();
	void render(int xm, int ym, float a);
	bool renderGameBehind();
	void buttonClicked(Button* button);

	// IInventoryPaneCallback
	bool addItem(const Touch::InventoryPane* pane, int itemId);
	bool isAllowed( int slot );
	std::vector<const ItemInstance*> getItems( const Touch::InventoryPane* forPane );
private:
	//void addItem(Recipe* recipe);
	void recheckRecipes();

	void clearItems();
	void updateResult(const ItemInstance* item);
	void setupInventoryPane();
	void updateItems();

	void drawSlotItemAt(Tesselator& t, const ItemInstance* item, int x, int y, bool selected);
	ItemInstance moveOver(const ItemInstance* item, int maxCount);
	void takeAndClearSlot( int slot );
	bool handleAddItem( int slot, const ItemInstance* item );
	void handleRenderPane(Touch::InventoryPane* pane, Tesselator& t, int xm, int ym, float a);
	bool canMoveToFurnace(int inventorySlot, const ItemInstance* item);
	ItemList _items;

	std::string currentItemDesc;
	ItemInstance burnResult;
	float descWidth;
	ImageButton btnClose;
	BlankButton btnIngredient;
	BlankButton btnFuel;
	BlankButton btnResult;
	BlankButton btnBottles;
	Touch::THeader bHeader;

	Touch::InventoryPane* inventoryPane;
	IntRectangle inventoryPaneRect;
	
	ItemList listFuel;
	ItemList listIngredient;
	ItemList listBottles;
	std::vector<int> inventorySlots;
	std::vector<const ItemInstance*> inventoryItems;
	bool doRecreatePane;

	int selectedSlot;
	int lastBurnTypeId;

	// GUI elements such as 9-Patches
	NinePatchLayer* guiBackground;
	NinePatchLayer* guiSlot;
	NinePatchLayer* guiSlotMarked;
	NinePatchLayer* guiSlotMarker;
	NinePatchLayer* guiPaneFrame;
	Player* player;
	BrewingStandTileEntity* furnace;
};

#endif /*_BREWINGSTANDSCREEN_H__*/
