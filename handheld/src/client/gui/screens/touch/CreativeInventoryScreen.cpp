#include "CreativeInventoryScreen.h"
#include "../ArmorScreen.h"
#include "../../../../platform/input/Keyboard.h"
#include "../../components/NinePatch.h"
#include "../../Gui.h"
#include "../../../Minecraft.h"
#include "../../../player/LocalPlayer.h"
#include "../../../renderer/Tesselator.h"
#include "../../../renderer/Textures.h"
#include "../../../renderer/gles.h"
#include "../../../renderer/entity/ItemRenderer.h"
#include "../../../sound/SoundEngine.h"
#include "../../../../world/entity/player/Inventory.h"
#include "../../../../world/item/Item.h"
#include "../../../../world/item/ShearsItem.h"
#include "../../../../world/item/ItemInstance.h"
#include "../../../../world/level/tile/TileInclude.h"
#include "../../../../util/Mth.h"

namespace Touch {

static const float BorderPixels = 4;
static const int ItemSize = 24 + 8;

/// The picture on each section's tab.
static ItemInstance tabIcon(int section) {
	switch (section) {
		case CreativeInventoryScreen::Building:		return ItemInstance(Tile::redBrick);
		case CreativeInventoryScreen::Colored:		return ItemInstance(Tile::cloth, 1, 9);
		case CreativeInventoryScreen::Natural:		return ItemInstance(Tile::grass);
		case CreativeInventoryScreen::Functional:	return ItemInstance(Tile::workBench);
		case CreativeInventoryScreen::Tools:		return ItemInstance(Item::pickAxe_iron);
		case CreativeInventoryScreen::WeaponsArmor:	return ItemInstance(Item::sword_emerald);
		case CreativeInventoryScreen::FoodDrinks:	return ItemInstance(Item::apple);
		default:									return ItemInstance(Item::ironIngot);
	}
}

static bool isOneOf(int id, Item* const* items, int n) {
	for (int i = 0; i < n; ++i) if (items[i] && items[i]->id == id) return true;
	return false;
}
static bool isOneOf(int id, Tile* const* tiles, int n) {
	for (int i = 0; i < n; ++i) if (tiles[i] && tiles[i]->id == id) return true;
	return false;
}
#define COUNT(a) ((int) (sizeof(a) / sizeof((a)[0])))

int CreativeInventoryScreen::sectionOf(const ItemInstance& item)
{
	const int id = item.id;
	if (id < 256) {
		if (id == Tile::cloth->id) return Colored;
		Tile* const natural[] = {
			Tile::grass, Tile::dirt, Tile::rock, Tile::sand, Tile::gravel, Tile::treeTrunk,
			Tile::leaves, Tile::sapling, Tile::flower, Tile::rose, Tile::mushroom1, Tile::mushroom2,
			Tile::cactus, Tile::reeds, Tile::clay, Tile::snow, Tile::topSnow, Tile::ice, Tile::obsidian,
			Tile::coalOre, Tile::ironOre, Tile::goldOre, Tile::emeraldOre, Tile::lapisOre,
			Tile::redStoneOre, Tile::quartzOre, Tile::netherrack, Tile::hellSand, Tile::lightGem,
			Tile::pumpkin, Tile::melon, Tile::tallgrass, Tile::web, Tile::unbreakable,
			Tile::prismarine, Tile::coral[0], Tile::coral[1], Tile::coral[2], Tile::coral[3], Tile::coral[4],
		};
		if (isOneOf(id, natural, COUNT(natural))) return Natural;
		Tile* const functional[] = {
			Tile::workBench, Tile::furnace, Tile::chest, Tile::stonecutterBench, Tile::tnt,
			Tile::enchantTable, Tile::oceanCrystalBlock, Tile::netherReactor, Tile::mobSpawner,
			Tile::brewingStand,
		};
		if (isOneOf(id, functional, COUNT(functional))) return Functional;
		return Building;
	}
	Item* const self = Item::items[id];
	if (!self) return Ingredients;
	if (self->isArmor()) return WeaponsArmor;
	Item* const weapons[] = {
		Item::sword_wood, Item::sword_stone, Item::sword_iron, Item::sword_gold, Item::sword_emerald,
		Item::bow, Item::arrow,
	};
	if (isOneOf(id, weapons, COUNT(weapons))) return WeaponsArmor;
	Item* const tools[] = {
		Item::pickAxe_wood, Item::pickAxe_stone, Item::pickAxe_iron, Item::pickAxe_gold, Item::pickAxe_emerald,
		Item::shovel_wood, Item::shovel_stone, Item::shovel_iron, Item::shovel_gold, Item::shovel_emerald,
		Item::hatchet_wood, Item::hatchet_stone, Item::hatchet_iron, Item::hatchet_gold, Item::hatchet_emerald,
		Item::hoe_wood, Item::hoe_stone, Item::hoe_iron, Item::hoe_gold, Item::hoe_emerald,
		Item::flintAndSteel, Item::shears, Item::bucket_empty, Item::bucket_water, Item::bucket_lava,
		Item::clock, Item::compass, Item::camera,
	};
	if (isOneOf(id, tools, COUNT(tools))) return Tools;
	Item* const drinks[] = { Item::waterBottle, Item::awkwardPotion, Item::waterBreathingPotion, Item::mushroomStew };
	if (self->isFood() || isOneOf(id, drinks, COUNT(drinks))) return FoodDrinks;
	Item* const building[] = {
		Item::door_wood, Item::door_iron, Item::door_spruce, Item::door_birch, Item::sign, Item::painting, Item::bed,
	};
	if (isOneOf(id, building, COUNT(building))) return Building;
	if (id == Item::brewingStand->id) return Functional;
	return Ingredients;
}

CreativeInventoryScreen::CreativeInventoryScreen()
:	section(0),
	pane(NULL),
	bClose(1, ""),
	tabSize(32),
	hotbarX(0), hotbarY(0), hotbarSlot(28),
	guiBackground(NULL), guiTab(NULL), guiTabSelected(NULL),
	guiPaneFrame(NULL), guiSlot(NULL), guiSlotSelected(NULL)
{
}

CreativeInventoryScreen::~CreativeInventoryScreen()
{
	delete pane;
	for (unsigned int i = 0; i < tabs.size(); ++i) delete tabs[i];
	delete guiBackground;
	delete guiTab;
	delete guiTabSelected;
	delete guiPaneFrame;
	delete guiSlot;
	delete guiSlotSelected;
}

void CreativeInventoryScreen::init()
{
	ImageDef def;
	def.name = "gui/spritesheet.png";
	def.x = 0;
	def.y = 1;
	def.width = def.height = 18;
	def.setSrc(IntRectangle(60, 0, 18, 18));
	bClose.setImageDef(def, true);
	bClose.scaleWhenPressed = false;
	buttons.push_back(&bClose);

	for (int i = 0; i < NumSections; ++i) {
		ImageButton* tab = new ImageButton(100 + i, "");
		tabs.push_back(tab);
		buttons.push_back(tab);
	}

	// The crafting screens' patches (PaneCraftingScreen::init).
	NinePatchFactory builder(minecraft->textures, "gui/spritesheet.png");
	guiBackground   = builder.createSymmetrical(IntRectangle(0, 0, 16, 16), 4, 4);
	guiPaneFrame    = builder.createSymmetrical(IntRectangle(0, 20, 8, 8), 1, 2)->setExcluded(1 << 4);
	guiTab          = builder.createSymmetrical(IntRectangle(8, 32, 8, 8), 2, 2);
	guiTabSelected  = builder.createSymmetrical(IntRectangle(0, 32, 8, 8), 2, 2);
	guiSlot         = builder.createSymmetrical(IntRectangle(0, 32, 8, 8), 3, 3);
	guiSlotSelected = builder.createSymmetrical(IntRectangle(10, 42, 16, 16), 5, 5);
}

void CreativeInventoryScreen::setupPositions()
{
	const int b = (int) BorderPixels;
	tabSize = (height - 2 * b - 3) / 4;
	for (int i = 0; i < NumSections; ++i) {
		ImageButton* tab = tabs[i];
		tab->width = tab->height = tabSize;
		tab->x = i < 4 ? b : width - b - tabSize;
		tab->y = b + (i % 4) * (tabSize + 1);
	}
	bClose.width = bClose.height = 19;
	bClose.x = width - bClose.width;
	bClose.y = 0;

	// The hotbar under the grid, its slots the size the grid's items are drawn.
	hotbarSlot = 22;
	// getNumSlots counts the hotbar's last "..." button too.
	// One more than the hotbar's: the "..." that opens the inventory with the
	// player in it (ArmorScreen), as on the HUD.
	const int slots = minecraft->gui.getNumSlots();
	hotbarX = width / 2 - slots * hotbarSlot / 2;
	hotbarY = height - b - hotbarSlot - 2;

	const int left = b + tabSize + b + 2;
	const int right = width - b - tabSize - b - 2;
	const int columns = Mth::Max(1, (right - left) / ItemSize);
	paneRect.w = columns * ItemSize;
	paneRect.x = left + (right - left - paneRect.w) / 2;
	paneRect.y = b + 8;
	paneRect.h = hotbarY - 6 - paneRect.y;

	guiBackground->setSize((float) width, (float) height);
	guiTab->setSize((float) tabSize, (float) tabSize);
	guiTabSelected->setSize((float) tabSize, (float) tabSize);
	guiPaneFrame->setSize((float) paneRect.w + 2, (float) paneRect.h + 4);
	guiSlot->setSize((float) hotbarSlot, (float) hotbarSlot);
	guiSlotSelected->setSize((float) hotbarSlot + 4, (float) hotbarSlot + 4);

	selectSection(section);
}

void CreativeInventoryScreen::selectSection(int s)
{
	section = s;
	slots.clear();
	Inventory* inventory = minecraft->player->inventory;
	for (int i = Inventory::MAX_SELECTION_SIZE; i < inventory->getContainerSize(); ++i) {
		ItemInstance* item = inventory->getItem(i);
		if (item && !item->isNull() && sectionOf(*item) == section)
			slots.push_back(i);
	}
	rebuildPane();
}

void CreativeInventoryScreen::rebuildPane()
{
	delete pane;
	pane = new InventoryPane(this, minecraft, paneRect, paneRect.w, BorderPixels, (int) slots.size(), ItemSize, (int) BorderPixels);
	pane->fillMarginX = 0;
	pane->fillMarginY = 0;
}

void CreativeInventoryScreen::removed()
{
	minecraft->gui.inventoryUpdated();
}

void CreativeInventoryScreen::tick()
{
	if (pane) pane->tick();
	super::tick();
}

int CreativeInventoryScreen::hotbarSlotAt(int x, int y)
{
	if (y < hotbarY || y >= hotbarY + hotbarSlot || x < hotbarX) return -1;
	const int slot = (x - hotbarX) / hotbarSlot;
	return slot < minecraft->gui.getNumSlots() ? slot : -1;
}

void CreativeInventoryScreen::mouseClicked(int x, int y, int buttonNum)
{
	const int slot = hotbarSlotAt(x, y);
	if (slot == minecraft->gui.getNumSlots() - 1) {
		minecraft->setScreen(new ArmorScreen());
		return;
	}
	if (slot >= 0) {
		minecraft->player->inventory->selectSlot(slot);
		return;
	}
	super::mouseClicked(x, y, buttonNum);
}

void CreativeInventoryScreen::keyPressed(int eventKey)
{
	// E closes it, as it opens it (Minecraft's key loop does not run while a
	// screen is up).
	if (eventKey == Keyboard::KEY_E) {
		minecraft->setScreen(NULL);
		return;
	}
	super::keyPressed(eventKey);
}

void CreativeInventoryScreen::mouseWheel(int dy)
{
	if (pane) pane->scrollByY(-dy * (float) ItemSize);
}

void CreativeInventoryScreen::buttonClicked(Button* button)
{
	if (button == &bClose) {
		minecraft->setScreen(NULL);
		return;
	}
	if (button->id >= 100 && button->id < 100 + NumSections)
		selectSection(button->id - 100);
}

bool CreativeInventoryScreen::addItem(const InventoryPane* forPane, int index)
{
	if (index < 0 || index >= (int) slots.size()) return false;
	Inventory* inventory = minecraft->player->inventory;
	// Into the hotbar slot that is selected, one of it: creative never runs out.
	inventory->moveToSelectionSlot(inventory->selected, slots[index], true);
	minecraft->soundEngine->playUI("random.pop2", 1.0f, 0.3f);
	minecraft->gui.flashSlot(inventory->selected);
	return true;
}

bool CreativeInventoryScreen::isAllowed(int slot)
{
	return slot >= 0 && slot < (int) slots.size();
}

std::vector<const ItemInstance*> CreativeInventoryScreen::getItems(const InventoryPane* forPane)
{
	std::vector<const ItemInstance*> out;
	Inventory* inventory = minecraft->player->inventory;
	for (unsigned int i = 0; i < slots.size(); ++i)
		out.push_back(inventory->getItem(slots[i]));
	return out;
}

void CreativeInventoryScreen::render(int xm, int ym, float a)
{
	Tesselator& t = Tesselator::instance;
	glDisable2(GL_DEPTH_TEST);
	glEnable2(GL_BLEND);
	glColor4f2(1, 1, 1, 1);
	guiBackground->draw(t, 0, 0);

	// Tabs: the selected one pressed in, each with its section's picture.
	for (int i = 0; i < NumSections; ++i) {
		ImageButton* tab = tabs[i];
		(i == section ? guiTabSelected : guiTab)->draw(t, (float) tab->x, (float) tab->y);
		ItemInstance icon = tabIcon(i);
		const float inset = tabSize * 0.2f;
		ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, &icon,
			tab->x + inset, tab->y + inset, tabSize - 2 * inset, tabSize - 2 * inset, true);
	}
	bClose.render(minecraft, xm, ym);

	if (pane) {
		pane->render(xm, ym, a);
		guiPaneFrame->draw(t, (float) (paneRect.x - 1), (float) (paneRect.y - 1));
	}

	// The hotbar.
	Inventory* inventory = minecraft->player->inventory;
	const int n = minecraft->gui.getNumSlots() - 1;
	for (int i = 0; i < n; ++i) {
		const float x = (float) (hotbarX + i * hotbarSlot), y = (float) hotbarY;
		guiSlot->draw(t, x, y);
		if (i == inventory->selected) guiSlotSelected->draw(t, x - 2, y - 2);
		ItemInstance* item = inventory->getItem(i);
		if (item && !item->isNull())
			ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, item, x + 3, y + 3, true);
	}
	// The "..." slot, with the HUD's dots (gui/gui.png).
	{
		const int x = hotbarX + n * hotbarSlot, y = hotbarY;
		// A slot like the others, then the dots. (A fill() here left the
		// texture off and the dots never drew.)
		guiSlot->draw(t, (float) x, (float) y);
		glEnable2(GL_TEXTURE_2D);
		minecraft->textures->loadAndBindTexture("gui/gui.png");
		glColor4f2(1, 1, 1, 1);
		blit(x + (hotbarSlot - 14) / 2, y + (hotbarSlot - 4) / 2, 242, 252, 14, 4, 14, 4);
	}
	glEnable2(GL_DEPTH_TEST);
}

}
