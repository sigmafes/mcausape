#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__BrewingStandTileEntity_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__BrewingStandTileEntity_H__

#include "FurnaceTileEntity.h"

/** The brewing stand (docs/brewing-plan.md): a furnace with a fourth slot.
    It keeps the furnace's slot numbers, fuel timer and progress (so
    FurnaceMenu carries it to visitors unchanged) and adds a slot of bottles
    waiting their turn. One bottle and one ingredient make one potion, every
    20 seconds, into the result slot; the bottle and ingredient slots hold
    stacks so it carries on by itself while there is room for the result.

    Fuel is blaze powder, which lasts as long as coal in the furnace (80 s):
    four potions a powder. */
class BrewingStandTileEntity: public FurnaceTileEntity
{
	typedef FurnaceTileEntity super;
public:
	static const int SLOT_BOTTLES = 3;
	static const int BREW_TIME = 20 * 20;
	static const int FUEL_TIME = 10 * 20 * 8;

	BrewingStandTileEntity();

	std::string getName() const;
	int getContainerSize() const;

	void load(CompoundTag* base);
	void tick();

	int getBrewProgress(int max);

	static bool isBrewFuel(const ItemInstance& item);
	static bool isBottle(const ItemInstance& item);
	static bool isIngredient(const ItemInstance& item);
	/// What a bottle with an ingredient makes; null if they make nothing.
	static ItemInstance getBrewResult(const ItemInstance& bottle, const ItemInstance& ingredient);

private:
	bool canBrew();
	void brew();
	/// Bottles on the stand's arms (its data): 1 = bottles waiting, 2 = a
	/// potion done, 4 = more than one bottle waiting.
	void updateArms();
};

#endif
