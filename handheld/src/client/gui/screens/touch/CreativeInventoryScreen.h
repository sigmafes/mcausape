#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__CreativeInventoryScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__CreativeInventoryScreen_H__

#include "../../Screen.h"
#include "../../components/InventoryPane.h"
#include "../../components/Button.h"
#include "../../components/ImageButton.h"
#include <vector>

class NinePatchLayer;
class ItemInstance;

namespace Touch {

/** The creative inventory, in the crafting screens' look: eight sections as
    tabs, four down each side, a grid of what is in the selected one in the
    middle, and the hotbar under it. A tap on an item puts it in the selected
    hotbar slot; a tap on a hotbar slot selects it. The items are the creative
    inventory's own (Inventory fills it), sorted into sections by
    CreativeInventoryScreen::sectionOf. */
class CreativeInventoryScreen:	public Screen,
								public IInventoryPaneCallback
{
	typedef Screen super;
public:
	static const int NumSections = 8;
	enum Section {
		Building, Colored, Natural, Functional,
		Tools, WeaponsArmor, FoodDrinks, Ingredients
	};
	static int sectionOf(const ItemInstance& item);

	CreativeInventoryScreen();
	~CreativeInventoryScreen();

	void init();
	void setupPositions();
	void removed();
	void tick();
	void render(int xm, int ym, float a);
	bool renderGameBehind() { return false; }
	void buttonClicked(Button* button);

	// IInventoryPaneCallback
	bool addItem(const InventoryPane* pane, int itemId);
	bool isAllowed(int slot);
	std::vector<const ItemInstance*> getItems(const InventoryPane* forPane);

protected:
	void mouseClicked(int x, int y, int buttonNum);
	void mouseWheel(int dy);

private:
	void selectSection(int section);
	void rebuildPane();
	int hotbarSlotAt(int x, int y);

	int section;
	std::vector<int> slots;		///< inventory slots in the section
	InventoryPane* pane;
	IntRectangle paneRect;
	ImageButton bClose;
	std::vector<ImageButton*> tabs;
	int tabSize;
	int hotbarX, hotbarY, hotbarSlot;

	NinePatchLayer* guiBackground;
	NinePatchLayer* guiTab;
	NinePatchLayer* guiTabSelected;
	NinePatchLayer* guiPaneFrame;
	NinePatchLayer* guiSlot;
	NinePatchLayer* guiSlotSelected;
};

}

#endif
