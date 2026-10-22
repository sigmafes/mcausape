#include "../BrewingStandTile.h"
#include "BrewingStandTileEntity.h"
#include "../../Level.h"
#include "../../../Container.h"
#include "../../../item/Item.h"
#include "../../../item/ItemInstance.h"
#include "../../../../nbt/CompoundTag.h"

BrewingStandTileEntity::BrewingStandTileEntity()
:	super(TileEntityType::BrewingStand, ContainerType::BREWING_STAND)
{
}

std::string BrewingStandTileEntity::getName() const {
	return "Brewing Stand";
}

int BrewingStandTileEntity::getContainerSize() const {
	return 4;
}

void BrewingStandTileEntity::load(CompoundTag* base) {
	super::load(base);
	// The furnace works the duration out from its own fuels; this one has one.
	litDuration = FUEL_TIME;
}

int BrewingStandTileEntity::getBrewProgress(int max) {
	return tickCount * max / BREW_TIME;
}

bool BrewingStandTileEntity::isBrewFuel(const ItemInstance& item) {
	return !item.isNull() && item.id == Item::blazePowder->id;
}

bool BrewingStandTileEntity::isBottle(const ItemInstance& item) {
	return !item.isNull() && (item.id == Item::waterBottle->id || item.id == Item::awkwardPotion->id);
}

bool BrewingStandTileEntity::isIngredient(const ItemInstance& item) {
	return !item.isNull() && (item.id == Item::netherStalkSeeds->id || item.id == Item::oceanCrystal->id
		|| item.id == Item::pufferfish->id || item.id == Item::spiderEye->id);
}

ItemInstance BrewingStandTileEntity::getBrewResult(const ItemInstance& bottle, const ItemInstance& ingredient) {
	if (bottle.isNull() || ingredient.isNull()) return ItemInstance();
	if (bottle.id == Item::waterBottle->id && ingredient.id == Item::netherStalkSeeds->id)
		return ItemInstance(Item::awkwardPotion);
	if (bottle.id == Item::awkwardPotion->id && ingredient.id == Item::oceanCrystal->id)
		return ItemInstance(Item::waterBreathingPotion);
	if (bottle.id == Item::awkwardPotion->id && (ingredient.id == Item::pufferfish->id || ingredient.id == Item::spiderEye->id))
		return ItemInstance(Item::unpoisonPotion);
	return ItemInstance();
}

bool BrewingStandTileEntity::canBrew() {
	ItemInstance result = getBrewResult(items[SLOT_BOTTLES], items[SLOT_INGREDIENT]);
	if (result.isNull()) return false;
	const ItemInstance& out = items[SLOT_RESULT];
	if (out.isNull()) return true;
	// Potions do not stack; the plain potion does (16), like the bottles.
	return out.id == result.id && out.count < out.getMaxStackSize();
}

void BrewingStandTileEntity::brew() {
	ItemInstance result = getBrewResult(items[SLOT_BOTTLES], items[SLOT_INGREDIENT]);
	if (result.isNull()) return;
	if (items[SLOT_RESULT].isNull()) items[SLOT_RESULT] = result;
	else items[SLOT_RESULT].count++;
	if (--items[SLOT_BOTTLES].count <= 0) items[SLOT_BOTTLES].setNull();
	if (--items[SLOT_INGREDIENT].count <= 0) items[SLOT_INGREDIENT].setNull();
}

void BrewingStandTileEntity::tick() {
	bool changed = false;
	if (litTime > 0) --litTime;

	if (!level->isClientSide) {
		// A powder is lit only when there is something to brew, as coal is.
		if (litTime == 0 && canBrew() && isBrewFuel(items[SLOT_FUEL])) {
			litDuration = litTime = FUEL_TIME;
			if (--items[SLOT_FUEL].count <= 0) items[SLOT_FUEL].setNull();
			changed = true;
		}
		if (litTime > 0 && canBrew()) {
			if (++tickCount >= BREW_TIME) {
				tickCount = 0;
				brew();
				changed = true;
			}
		} else {
			tickCount = 0;
		}
		updateArms();
		BrewingStandTile::setLit(litTime > 0, level, x, y, z);
	}
	if (changed) setChanged();
}

void BrewingStandTileEntity::updateArms() {
	int data = 0;
	if (!items[SLOT_BOTTLES].isNull()) data |= 1;
	if (!items[SLOT_RESULT].isNull()) data |= 2;
	if (!items[SLOT_BOTTLES].isNull() && items[SLOT_BOTTLES].count > 1) data |= 4;
	if (level->getData(x, y, z) != data)
		level->setData(x, y, z, data);
}
