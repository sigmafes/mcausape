#ifndef NET_MINECRAFT_CLIENT_GUI__ItemTooltip_H__
#define NET_MINECRAFT_CLIENT_GUI__ItemTooltip_H__

#include "GuiComponent.h"
#include "Font.h"
#include "../../world/item/ItemInstance.h"
#include "../../world/item/Item.h"
#include "../../world/item/WeaponItem.h"
#include "../../world/item/DiggerItem.h"
#include "../../world/item/ArmorItem.h"
#include "../../world/item/PickaxeItem.h"
#include "../../world/item/HatchetItem.h"
#include "../../world/item/ShovelItem.h"
#include "../../world/effect/MobEffect.h"
#include <cstdio>
#include <string>
#include <vector>

/** Java's tooltip over an item under the mouse in an inventory: its name,
    and for weapons, tools, armour and potions a line in blue saying what it
    does. Drawn by the screens holding an InventoryPane, on a PC only (a
    touch has nothing hovering). */
namespace ItemTooltip
{
	inline std::string statLine(const ItemInstance& item)
	{
		Item* it = item.getItem();
		if (!it) return "";
		char buf[64] = "";
		const int id = item.id;
		if (Item::sword_wood && (id == Item::sword_wood->id || id == Item::sword_stone->id || id == Item::sword_iron->id
				|| id == Item::sword_gold->id || id == Item::sword_emerald->id)) {
			snprintf(buf, sizeof buf, "+%d Attack Damage", ((WeaponItem*) it)->getAttackDamage(NULL));
		} else if (Item::pickAxe_wood && (id == Item::pickAxe_wood->id || id == Item::pickAxe_stone->id || id == Item::pickAxe_iron->id
				|| id == Item::pickAxe_gold->id || id == Item::pickAxe_emerald->id)) {
			snprintf(buf, sizeof buf, "+%d Mining Efficiency", (int) ((DiggerItem*) it)->getSpeed());
		} else if (Item::hatchet_wood && (id == Item::hatchet_wood->id || id == Item::hatchet_stone->id || id == Item::hatchet_iron->id
				|| id == Item::hatchet_gold->id || id == Item::hatchet_emerald->id)) {
			snprintf(buf, sizeof buf, "+%d Chop Efficiency", (int) ((DiggerItem*) it)->getSpeed());
		} else if (Item::shovel_wood && (id == Item::shovel_wood->id || id == Item::shovel_stone->id || id == Item::shovel_iron->id
				|| id == Item::shovel_gold->id || id == Item::shovel_emerald->id)) {
			snprintf(buf, sizeof buf, "+%d Dig Efficiency", (int) ((DiggerItem*) it)->getSpeed());
		} else if (it->isArmor()) {
			snprintf(buf, sizeof buf, "+%d Armor", ((ArmorItem*) it)->defense);
		} else if (Item::waterBreathingPotion && id == Item::waterBreathingPotion->id) {
			const int s = Item::PotionDurationTicks / 20;
			snprintf(buf, sizeof buf, "Water Breathing (%d:%02d)", s / 60, s % 60);
		}
		return buf;
	}

	/// GuiComponent::fill is protected: a component of its own to draw with.
	struct Painter: public GuiComponent {
		void box(int x0, int y0, int x1, int y1, int col) { fill(x0, y0, x1, y1, col); }
	};

	inline void render(GuiComponent* /*screen*/, Font* font, const ItemInstance* item, int mx, int my, int screenW, int screenH)
	{
		if (!item || item->isNull()) return;
		std::vector<std::string> lines;
		std::vector<int> colors;
		lines.push_back(item->getName());
		colors.push_back(0xffffffff);
		std::string stat = statLine(*item);
		if (!stat.empty()) {
			lines.push_back(stat);
			colors.push_back(0xff5555ff);	// Java's blue for what an item does
		}
		int w = 0;
		for (unsigned int i = 0; i < lines.size(); ++i)
			if (font->width(lines[i]) > w) w = font->width(lines[i]);
		const int h = (int) lines.size() * 10 - 2;
		int x = mx + 12, y = my - 12;
		if (x + w + 4 > screenW) x = mx - 16 - w;
		if (y + h + 4 > screenH) y = screenH - h - 4;
		if (y < 4) y = 4;
		// Java's look: near-black inside, a violet edge.
		Painter p;
		Painter* gui = &p;
		gui->box(x - 3, y - 4, x + w + 3, y + h + 4, 0xf0100010);
		gui->box(x - 4, y - 3, x + w + 4, y + h + 3, 0xf0100010);
		gui->box(x - 3, y - 3, x + w + 3, y - 2, 0x505000ff);
		gui->box(x - 3, y + h + 2, x + w + 3, y + h + 3, 0x505000ff);
		gui->box(x - 3, y - 2, x - 2, y + h + 2, 0x505000ff);
		gui->box(x + w + 2, y - 2, x + w + 3, y + h + 2, 0x505000ff);
		for (unsigned int i = 0; i < lines.size(); ++i)
			font->drawShadow(lines[i], (float) x, (float) (y + i * 10), colors[i]);
	}
}

#endif
