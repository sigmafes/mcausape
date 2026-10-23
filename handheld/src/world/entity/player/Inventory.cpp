#include "Inventory.h"
#include "../../level/material/Material.h"
#include "../../level/tile/QuartzBlockTile.h"
#include "../../level/tile/TreeTile.h"
#include "../../level/tile/StoneSlabTile.h"
#include "../../level/tile/MobSpawnerTile.h"
#include "../../level/tile/WoodTile.h"
#include "../../item/DyePowderItem.h"
#include "../../item/crafting/Recipe.h"
#include "../../item/CoalItem.h"
#include "../../level/tile/SandStoneTile.h"

Inventory::Inventory( Player* player, bool creativeMode )
:   super(	36 + Inventory::MAX_SELECTION_SIZE,
			MAX_SELECTION_SIZE,
			ContainerType::INVENTORY,
			creativeMode),
	player(player),
	selected(0)
{
	setupDefault();
	compressLinkedSlotList(0);
}

Inventory::~Inventory() {
}

ItemInstance* Inventory::getSelected() {
	return getLinked(selected);
}

void Inventory::selectSlot( int slot ) {
	if (slot < MAX_SELECTION_SIZE && slot >= 0)
		selected = slot;
}

bool Inventory::moveToSelectedSlot( int inventorySlot, bool propagate ) {
	return linkSlot(selected, inventorySlot, propagate);
}

int Inventory::getSelectionSize() {
	return MAX_SELECTION_SIZE;
}

void Inventory::setupDefault() {
	clearInventory();
	int Sel[MAX_SELECTION_SIZE] = {0};

#ifdef DEMO_MODE
	if (_isCreative) {
		Sel[0] = addItem(new ItemInstance(Item::shovel_stone));
		addItem(new ItemInstance(Item::pickAxe_stone));
		addItem(new ItemInstance(Item::hatchet_stone));
		addItem(new ItemInstance((Item*)Item::shears));
		addItem(new ItemInstance(Tile::ladder));
		Sel[3] = addItem(new ItemInstance(Tile::torch));
		addItem(new ItemInstance(Item::door_wood));

		Sel[4] = addItem(new ItemInstance(Tile::stoneBrick));
		Sel[5] = addItem(new ItemInstance(Tile::wood));
		Sel[2] = addItem(new ItemInstance(Tile::redBrick));
		Sel[1] = addItem(new ItemInstance(Tile::dirt));
		addItem(new ItemInstance(Tile::sandStone));
		addItem(new ItemInstance(Tile::gravel));
		addItem(new ItemInstance(Tile::rock));
		addItem(new ItemInstance(Tile::sand));
		//addItem(new ItemInstance(Tile::clay));

		addItem(new ItemInstance(Tile::cloth, 1, 15));
		addItem(new ItemInstance(Tile::cloth, 1, 14));
		addItem(new ItemInstance(Tile::cloth, 1, 13));
		Sel[7] = addItem(new ItemInstance(Tile::cloth, 1, 12));
		addItem(new ItemInstance(Tile::cloth, 1, 11));
		addItem(new ItemInstance(Tile::cloth, 1, 10));
		Sel[8] = addItem(new ItemInstance(Tile::cloth, 1, 9));
		addItem(new ItemInstance(Tile::cloth, 1, 8));
		Sel[6] = addItem(new ItemInstance(Tile::glass));
		addItem(new ItemInstance(Tile::thinGlass));
		addItem(new ItemInstance(Tile::stairs_stone));
        addItem(new ItemInstance(Tile::bookshelf));
        addItem(new ItemInstance(Tile::workBench));
		addItem(new ItemInstance(Tile::chest));
		addItem(new ItemInstance(Tile::furnace));

		addItem(new ItemInstance(((Tile*)Tile::flower)));
		addItem(new ItemInstance(Tile::cactus));

		//
		// Those below are inactive due to demo
		//
		addItem(new ItemInstance(Item::sword_stone));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 0));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 1));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 2));
		addItem(new ItemInstance(Tile::fence));
		addItem(new ItemInstance(Tile::fenceGate));
		addItem(new ItemInstance(Item::reeds));
		addItem(new ItemInstance(((Tile*)Tile::rose)));
		addItem(new ItemInstance(((Tile*)Tile::mushroom2)));
		addItem(new ItemInstance(((Tile*)Tile::mushroom1)));
		addItem(new ItemInstance(Tile::cloth, 1, 7));
		addItem(new ItemInstance(Tile::cloth, 1, 6));
		addItem(new ItemInstance(Tile::cloth, 1, 5));
		addItem(new ItemInstance(Tile::cloth, 1, 4));
		addItem(new ItemInstance(Tile::cloth, 1, 3));
		addItem(new ItemInstance(Tile::stairs_wood));
		addItem(new ItemInstance(Tile::goldBlock));
		addItem(new ItemInstance(Tile::ironBlock));
		addItem(new ItemInstance(Tile::emeraldBlock));
		addItem(new ItemInstance(Tile::lapisBlock));
		addItem(new ItemInstance(Tile::obsidian));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 0));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 1));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 2));
		addItem(new ItemInstance(Tile::stoneSlabHalf));
	} else {
#if defined(WIN32)
		// Survival
		addItem(new ItemInstance((Item*)Item::shears));
		addItem(new ItemInstance(Tile::redBrick));
		addItem(new ItemInstance(Tile::glass));
#endif
	}
#else
	if (_isCreative) {
		// Blocks
		Sel[1] = addItem(new ItemInstance(Tile::stoneBrick));
		addItem(new ItemInstance(Tile::stoneBrickSmooth, 1, 0));
		addItem(new ItemInstance(Tile::stoneBrickSmooth, 1, 1));
		addItem(new ItemInstance(Tile::stoneBrickSmooth, 1, 2));
		addItem(new ItemInstance(Tile::mossStone));
		Sel[5] =  addItem(new ItemInstance(Tile::wood));
		addItem(new ItemInstance(Tile::wood, 1, WoodTile::SPRUCE_PLANKS));
		addItem(new ItemInstance(Tile::wood, 1, WoodTile::BIRCH_PLANKS));
		addItem(new ItemInstance(Tile::wood, 1, WoodTile::CRIMSON_PLANKS));
		addItem(new ItemInstance(Tile::wood, 1, WoodTile::WARPED_PLANKS));
		addItem(new ItemInstance(Tile::stairs_crimson));
		addItem(new ItemInstance(Tile::stairs_warped));
		addItem(new ItemInstance(Tile::stoneSlabHalf2, 1, StoneSlabTile::CRIMSON_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf2, 1, StoneSlabTile::WARPED_SLAB));
		addItem(new ItemInstance(Tile::fence_crimson));
		addItem(new ItemInstance(Tile::fence_warped));
		addItem(new ItemInstance(Tile::fenceGate_crimson));
		addItem(new ItemInstance(Tile::fenceGate_warped));
		addItem(new ItemInstance(Tile::trapdoor_crimson));
		addItem(new ItemInstance(Tile::trapdoor_warped));
		addItem(new ItemInstance(Item::door_crimson));
		addItem(new ItemInstance(Item::door_warped));
		Sel[2] = addItem(new ItemInstance(Tile::redBrick));

#ifdef RPI
		Sel[3] = addItem(new ItemInstance(Tile::rock));
#else
		Sel[0] = addItem(new ItemInstance(Tile::rock));
#endif
		Sel[4] = addItem(new ItemInstance(Tile::dirt));
		addItem(new ItemInstance(Tile::grass));
		addItem(new ItemInstance(Tile::clay));
		addItem(new ItemInstance(Tile::sandStone, 1, 0));
		addItem(new ItemInstance(Tile::sandStone, 1, 1));
		addItem(new ItemInstance(Tile::sandStone, 1, 2));
		addItem(new ItemInstance(Tile::sand));
		addItem(new ItemInstance(Tile::gravel));
		
		Sel[7] = addItem(new ItemInstance(Tile::treeTrunk, 1, 0));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 1));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 2));
		addItem(new ItemInstance(Tile::netherBrick));
		addItem(new ItemInstance(Tile::netherrack));
		addItem(new ItemInstance(Tile::hellSand));
		addItem(new ItemInstance(Tile::stairs_stone));
		addItem(new ItemInstance(Tile::stairs_wood));
		addItem(new ItemInstance(Tile::stairs_spruce));
		addItem(new ItemInstance(Tile::stairs_birch));
		Sel[6] = addItem(new ItemInstance(Tile::stairs_brick));
		addItem(new ItemInstance(Tile::stairs_sandStone));
		addItem(new ItemInstance(Tile::stairs_stoneBrickSmooth));
		addItem(new ItemInstance(Tile::stairs_netherBricks));
		addItem(new ItemInstance(Tile::stairs_quartz));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::STONE_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::COBBLESTONE_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::WOOD_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::SPRUCE_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::BIRCH_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::BRICK_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::SAND_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::SMOOTHBRICK_SLAB));

		addItem(new ItemInstance(Tile::quartzBlock, 1, QuartzBlockTile::TYPE_DEFAULT));
		addItem(new ItemInstance(Tile::quartzBlock, 1, QuartzBlockTile::TYPE_LINES));
		addItem(new ItemInstance(Tile::quartzBlock, 1, QuartzBlockTile::TYPE_CHISELED));
		addItem(new ItemInstance(Tile::stoneSlabHalf2, 1, StoneSlabTile::NETHERBRICK_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf2, 1, StoneSlabTile::QUARTZ_SLAB));

		
		
		// Ores
		addItem(new ItemInstance(Tile::coalOre));
		addItem(new ItemInstance(Tile::ironOre));
		addItem(new ItemInstance(Tile::goldOre));
		addItem(new ItemInstance(Tile::emeraldOre));
		addItem(new ItemInstance(Tile::lapisOre));
		addItem(new ItemInstance(Tile::redStoneOre));
		addItem(new ItemInstance(Tile::quartzOre));

		addItem(new ItemInstance(Tile::goldBlock));
		addItem(new ItemInstance(Tile::ironBlock));
		addItem(new ItemInstance(Tile::emeraldBlock));
		addItem(new ItemInstance(Tile::lapisBlock));
		addItem(new ItemInstance(Tile::obsidian));
		addItem(new ItemInstance(Tile::snow));
		addItem(new ItemInstance(Tile::glass));
        addItem(new ItemInstance(Tile::lightGem));

		addItem(new ItemInstance(Tile::netherReactor));
		
		addItem(new ItemInstance(Tile::cloth, 1, 0));
		addItem(new ItemInstance(Tile::cloth, 1, 7));
		addItem(new ItemInstance(Tile::cloth, 1, 6));
		addItem(new ItemInstance(Tile::cloth, 1, 5));
		addItem(new ItemInstance(Tile::cloth, 1, 4));
		addItem(new ItemInstance(Tile::cloth, 1, 3));
		addItem(new ItemInstance(Tile::cloth, 1, 2));
		addItem(new ItemInstance(Tile::cloth, 1, 1));

		addItem(new ItemInstance(Tile::cloth, 1, 15));
		addItem(new ItemInstance(Tile::cloth, 1, 14));
		addItem(new ItemInstance(Tile::cloth, 1, 13));
		addItem(new ItemInstance(Tile::cloth, 1, 12));
		addItem(new ItemInstance(Tile::cloth, 1, 11));
		addItem(new ItemInstance(Tile::cloth, 1, 10));
		addItem(new ItemInstance(Tile::cloth, 1, 9));
		addItem(new ItemInstance(Tile::cloth, 1, 8));
		addItem(new ItemInstance(Tile::ladder));
#ifdef RPI
		addItem(new ItemInstance(Tile::torch));
#else
		Sel[3] = addItem(new ItemInstance(Tile::torch));
#endif
		addItem(new ItemInstance(Tile::thinGlass));

		addItem(new ItemInstance(Item::door_wood));
		addItem(new ItemInstance(Tile::trapdoor));
		addItem(new ItemInstance(Tile::fence));
		addItem(new ItemInstance(Tile::fenceGate));
		addItem(new ItemInstance(Tile::enchantTable));
		addItem(new ItemInstance(Item::brewingStand));
		addItem(new ItemInstance(Tile::cobbleWall));
		addItem(new ItemInstance(Tile::prismarine));
		addItem(new ItemInstance(Tile::oceanCrystalBlock));
		for (int i = 0; i < 5; ++i) addItem(new ItemInstance(Tile::coral[i]));
		for (int i = 0; i < 5; ++i) addItem(new ItemInstance(Tile::coralPlant[i]));
		addItem(new ItemInstance(Item::kelp));
		addItem(new ItemInstance(Tile::boneBlock));
		addItem(new ItemInstance(Tile::seaLantern));
		addItem(new ItemInstance(Tile::crimsonNylium));
		addItem(new ItemInstance(Tile::warpedNylium));
		addItem(new ItemInstance(Tile::soulSoil));
		addItem(new ItemInstance(Tile::basalt));
		addItem(new ItemInstance(Tile::blackstone));
		addItem(new ItemInstance(Tile::polishedBasalt));
		addItem(new ItemInstance(Tile::polishedBlackstoneBricks));
		addItem(new ItemInstance(Tile::crackedPolishedBlackstoneBricks));
		addItem(new ItemInstance(Tile::polishedBlackstone));
		addItem(new ItemInstance(Tile::chiseledPolishedBlackstone));
		addItem(new ItemInstance(Tile::gildedBlackstone));
		addItem(new ItemInstance(Tile::netheriteBlock));
		addItem(new ItemInstance(Item::netheriteScrap));
		addItem(new ItemInstance(Item::netheriteIngot));
		addItem(new ItemInstance(Item::sword_netherite));
		addItem(new ItemInstance(Item::pickAxe_netherite));
		addItem(new ItemInstance(Item::hatchet_netherite));
		addItem(new ItemInstance(Item::shovel_netherite));
		addItem(new ItemInstance(Item::hoe_netherite));
		addItem(new ItemInstance(Item::helmet_netherite));
		addItem(new ItemInstance(Item::chestplate_netherite));
		addItem(new ItemInstance(Item::leggings_netherite));
		addItem(new ItemInstance(Item::boots_netherite));
		addItem(new ItemInstance(Tile::crimsonStem));
		addItem(new ItemInstance(Tile::warpedStem));
		addItem(new ItemInstance(Tile::netherWartBlock));
		addItem(new ItemInstance(Tile::warpedWartBlock));
		addItem(new ItemInstance(Tile::shroomlight));
		addItem(new ItemInstance(Tile::crimsonFungus));
		addItem(new ItemInstance(Tile::warpedFungus));
		addItem(new ItemInstance(Tile::crimsonRoots));
		addItem(new ItemInstance(Tile::warpedRoots));
		addItem(new ItemInstance(Tile::netherSprouts));
		addItem(new ItemInstance(Tile::weepingVines));
		addItem(new ItemInstance(Tile::twistingVines));
		addItem(new ItemInstance(Tile::magmaBlock));
		addItem(new ItemInstance(Tile::netherGoldOre));
		addItem(new ItemInstance(Tile::ancientDebris));
		addItem(new ItemInstance(Tile::prismarineBricks));
		addItem(new ItemInstance(Tile::darkPrismarine));
		addItem(new ItemInstance(Tile::stairs_prismarineBricks));
		addItem(new ItemInstance(Tile::stairs_darkPrismarine));
		addItem(new ItemInstance(Tile::stoneSlabHalf2, 1, StoneSlabTile::PRISMARINE_BRICK_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf2, 1, StoneSlabTile::DARK_PRISMARINE_SLAB));
		addItem(new ItemInstance(Item::prismarineCrystals));
		addItem(new ItemInstance(Item::rawCod));
		addItem(new ItemInstance(Item::rawSalmon));
		addItem(new ItemInstance(Item::cookedCod));
		addItem(new ItemInstance(Item::cookedSalmon));
		addItem(new ItemInstance(Item::pufferfish));
		addItem(new ItemInstance(Item::shrimp));
		addItem(new ItemInstance(Item::turtleScute));
		addItem(new ItemInstance(Item::crystalizedCoral));
		addItem(new ItemInstance(Item::coralIngot));
		addItem(new ItemInstance(Item::axolotlBucket));
		addItem(new ItemInstance(Item::trident));
		addItem(new ItemInstance(Item::pickAxe_coral));
		addItem(new ItemInstance(Item::hatchet_coral));
		addItem(new ItemInstance(Item::shovel_coral));
		addItem(new ItemInstance(Item::helmet_coral));
		addItem(new ItemInstance(Item::chestplate_coral));
		addItem(new ItemInstance(Item::leggings_coral));
		addItem(new ItemInstance(Item::boots_coral));
		addItem(new ItemInstance(Item::spiderEye));
		addItem(new ItemInstance(Item::unpoisonPotion));
		addItem(new ItemInstance(Tile::doubleGrass));
		addItem(new ItemInstance(Item::oceanCrystal));
		addItem(new ItemInstance(Item::oceanShard));
		addItem(new ItemInstance(Item::awkwardPotion));
		addItem(new ItemInstance(Item::waterBreathingPotion));
		addItem(new ItemInstance(Tile::mossyCobbleWall));
		addItem(new ItemInstance(Item::door_spruce));
		addItem(new ItemInstance(Item::door_birch));
		addItem(new ItemInstance(Tile::trapdoor_spruce));
		addItem(new ItemInstance(Tile::trapdoor_birch));
		addItem(new ItemInstance(Tile::fence_spruce));
		addItem(new ItemInstance(Tile::fence_birch));
		addItem(new ItemInstance(Tile::fenceGate_spruce));
		addItem(new ItemInstance(Tile::fenceGate_birch));

		addItem(new ItemInstance(Tile::netherFence));
		// One spawner per kind, so each can be tested without commands.
		addItem(new ItemInstance(Tile::mobSpawner, 1, MobSpawnerTile::ZOMBIE));
		addItem(new ItemInstance(Tile::mobSpawner, 1, MobSpawnerTile::SKELETON));
		addItem(new ItemInstance(Tile::mobSpawner, 1, MobSpawnerTile::SPIDER));
		addItem(new ItemInstance(Tile::mobSpawner, 1, MobSpawnerTile::PIG_ZOMBIE));
		addItem(new ItemInstance(Tile::mobSpawner, 1, MobSpawnerTile::WITHER_SKELETON));
		addItem(new ItemInstance(Tile::mobSpawner, 1, MobSpawnerTile::BLAZE));
		addItem(new ItemInstance(Tile::mobSpawner, 1, MobSpawnerTile::MAGMA_CUBE));
		addItem(new ItemInstance(Tile::mobSpawner, 1, MobSpawnerTile::GHAST));
		addItem(new ItemInstance(Item::bed));
		addItem(new ItemInstance(Tile::bookshelf));
		addItem(new ItemInstance(Item::painting));
		addItem(new ItemInstance(Tile::workBench));
		addItem(new ItemInstance(Tile::stonecutterBench));
		addItem(new ItemInstance(Tile::chest));
		addItem(new ItemInstance(Tile::furnace));
		addItem(new ItemInstance(Tile::tnt));

		addItem(new ItemInstance(((Tile*)Tile::flower)));
		addItem(new ItemInstance(((Tile*)Tile::rose)));
		addItem(new ItemInstance(((Tile*)Tile::mushroom1)));
		addItem(new ItemInstance(((Tile*)Tile::mushroom2)));
		addItem(new ItemInstance(Tile::cactus));
		addItem(new ItemInstance(Tile::melon));
		addItem(new ItemInstance(Tile::pumpkin));
		addItem(new ItemInstance(Tile::litPumpkin));
		addItem(new ItemInstance(Item::reeds));
		Sel[8] = addItem(new ItemInstance(Tile::sapling, 1, 0));
		addItem(new ItemInstance(Tile::sapling, 1, 1));
		addItem(new ItemInstance(Tile::sapling, 1, 2));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 0));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 1));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 2));

		addItem(new ItemInstance(Item::seeds_wheat));
		addItem(new ItemInstance(Item::seeds_melon));
		addItem(new ItemInstance(Item::seeds_pumpkin));
		addItem(new ItemInstance(Item::netherStalkSeeds));
		addItem(new ItemInstance(Item::netherbrick));
		addItem(new ItemInstance(Item::netherQuartz));
		addItem(new ItemInstance(Item::bucket_empty));
		addItem(new ItemInstance(Item::bucket_water));
		addItem(new ItemInstance(Item::bucket_lava));
		addItem(new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE));
		addItem(new ItemInstance(Item::flintAndSteel));
		addItem(new ItemInstance((Item*)Item::shears));

		// Every weapon, tool and set of armour, a material at a time.
#ifdef RPI
		Sel[0] = addItem(new ItemInstance(Item::sword_iron));
#else
		addItem(new ItemInstance(Item::sword_wood));
		addItem(new ItemInstance(Item::sword_stone));
		addItem(new ItemInstance(Item::sword_iron));
		addItem(new ItemInstance(Item::sword_gold));
		addItem(new ItemInstance(Item::sword_emerald));
#endif
		addItem(new ItemInstance(Item::bow));
		addItem(new ItemInstance(Item::arrow));

		addItem(new ItemInstance(Item::shovel_wood));
		addItem(new ItemInstance(Item::pickAxe_wood));
		addItem(new ItemInstance(Item::hatchet_wood));
		addItem(new ItemInstance(Item::hoe_wood));
		addItem(new ItemInstance(Item::shovel_stone));
		addItem(new ItemInstance(Item::pickAxe_stone));
		addItem(new ItemInstance(Item::hatchet_stone));
		addItem(new ItemInstance(Item::hoe_stone));
		addItem(new ItemInstance(Item::shovel_iron));
		addItem(new ItemInstance(Item::pickAxe_iron));
		addItem(new ItemInstance(Item::hatchet_iron));
		addItem(new ItemInstance(Item::hoe_iron));
		addItem(new ItemInstance(Item::shovel_gold));
		addItem(new ItemInstance(Item::pickAxe_gold));
		addItem(new ItemInstance(Item::hatchet_gold));
		addItem(new ItemInstance(Item::hoe_gold));
		addItem(new ItemInstance(Item::shovel_emerald));
		addItem(new ItemInstance(Item::pickAxe_emerald));
		addItem(new ItemInstance(Item::hatchet_emerald));
		addItem(new ItemInstance(Item::hoe_emerald));

		addItem(new ItemInstance(Item::helmet_cloth));
		addItem(new ItemInstance(Item::chestplate_cloth));
		addItem(new ItemInstance(Item::leggings_cloth));
		addItem(new ItemInstance(Item::boots_cloth));
		addItem(new ItemInstance(Item::helmet_chain));
		addItem(new ItemInstance(Item::chestplate_chain));
		addItem(new ItemInstance(Item::leggings_chain));
		addItem(new ItemInstance(Item::boots_chain));
		addItem(new ItemInstance(Item::helmet_iron));
		addItem(new ItemInstance(Item::chestplate_iron));
		addItem(new ItemInstance(Item::leggings_iron));
		addItem(new ItemInstance(Item::boots_iron));
		addItem(new ItemInstance(Item::helmet_gold));
		addItem(new ItemInstance(Item::chestplate_gold));
		addItem(new ItemInstance(Item::leggings_gold));
		addItem(new ItemInstance(Item::boots_gold));
		addItem(new ItemInstance(Item::helmet_diamond));
		addItem(new ItemInstance(Item::chestplate_diamond));
		addItem(new ItemInstance(Item::leggings_diamond));
		addItem(new ItemInstance(Item::boots_diamond));

		// Materials and food, so nothing a recipe needs has to be found first.
		addItem(new ItemInstance(Item::stick));
		addItem(new ItemInstance(Item::string));
		addItem(new ItemInstance(Item::feather));
		addItem(new ItemInstance(Item::flint));
		addItem(new ItemInstance(Item::coal, 1, CoalItem::STONE_COAL));
		addItem(new ItemInstance(Item::coal, 1, CoalItem::CHAR_COAL));
		addItem(new ItemInstance(Item::ironIngot));
		addItem(new ItemInstance(Item::goldIngot));
		addItem(new ItemInstance(Item::goldNugget));
		addItem(new ItemInstance(Item::emerald));
		addItem(new ItemInstance(Item::brick));
		addItem(new ItemInstance(Item::clay));
		addItem(new ItemInstance(Item::leather));
		addItem(new ItemInstance(Item::paper));
		addItem(new ItemInstance(Item::book));
		addItem(new ItemInstance(Item::snowBall));
		addItem(new ItemInstance(Item::slimeBall));
		addItem(new ItemInstance(Item::bone));
		addItem(new ItemInstance(Item::sugar));
		addItem(new ItemInstance(Item::egg));
		addItem(new ItemInstance(Item::sulphur));
		addItem(new ItemInstance(Item::yellowDust));
		addItem(new ItemInstance(Item::blazeRod));
		addItem(new ItemInstance(Item::blazePowder));
		addItem(new ItemInstance(Item::magmaCream));
		addItem(new ItemInstance(Item::glassBottle));
		addItem(new ItemInstance(Item::waterBottle));
		addItem(new ItemInstance(Item::compass));
		addItem(new ItemInstance(Item::clock));
		addItem(new ItemInstance(Item::wheat));
		addItem(new ItemInstance(Item::bread));
		addItem(new ItemInstance(Item::apple));
		addItem(new ItemInstance(Item::carrot));
		addItem(new ItemInstance(Item::goldenCarrot));
		addItem(new ItemInstance(Item::potato));
		addItem(new ItemInstance(Item::bakedPotato));
		addItem(new ItemInstance(Item::porkChop_raw));
		addItem(new ItemInstance(Item::porkChop_cooked));
		addItem(new ItemInstance(Item::beef_raw));
		addItem(new ItemInstance(Item::beef_cooked));
		addItem(new ItemInstance(Item::chicken_raw));
		addItem(new ItemInstance(Item::chicken_cooked));
		addItem(new ItemInstance(Item::melon));
		for (int dye = 0; dye < 16; ++dye)
			addItem(new ItemInstance(Item::dye_powder, 1, dye));

		addItem(new ItemInstance(Item::sign));
	} else {
// Survival starts empty. The Windows build used to hand out a developer kit
		// here (iron, wood, coal, a furnace...) for testing crafting.
	}
#endif

	for (unsigned int i = 0; i < items.size(); ++i) {
		ItemInstance* item = items[i];

		if (i < MAX_SELECTION_SIZE) {
			if (item)
				LOGE("Error: Should not have items on slot %i\n", i);

			items[i] = NULL;
			continue;
		}

		if (item && _isCreative)
			item->count = 5;
	}

	for (int i = 0; i < MAX_SELECTION_SIZE; ++i) {
		linkedSlots[i] = LinkedSlot(Sel[i]);
	}

	//LOGI("Inventory has %d items\n", (int)items.size());
}

void Inventory::clearInventoryWithDefault()
{
	clearInventory();
	setupDefault();
}

int Inventory::getAttackDamage( Entity* entity )
{
	ItemInstance* item = getSelected();
	if (item != NULL) return item->getAttackDamage(entity);
	return 1;
}

bool Inventory::canDestroy( Tile* tile )
{
	if (tile->material->isAlwaysDestroyable()) return true;

	ItemInstance* item = getSelected();
	if (item != NULL) return item->canDestroySpecial(tile);
	return false;
}

float Inventory::getDestroySpeed( Tile* tile )
{
	ItemInstance* item = getSelected();
	if (item && item->id >= 256) {
		return Item::items[item->id]->getDestroySpeed(NULL, tile);
	}
	return 1.0f;
}

bool Inventory::moveToSelectionSlot( int selectionSlot, int inventorySlot, bool propagate ) {
	return linkSlot(selectionSlot, inventorySlot, propagate);
}

bool Inventory::moveToEmptySelectionSlot( int inventorySlot ) {
	return linkEmptySlot(inventorySlot);
}

void Inventory::doDrop( ItemInstance* item, bool randomly )
{
	player->drop(item, randomly);
}

bool Inventory::stillValid(Player* player) {
	if (this->player->removed) return false;
	if (player->distanceToSqr(this->player) > 8 * 8) return false;
	return true;
}

bool Inventory::add( ItemInstance* item ){
	if (_isCreative || player->hasFakeInventory)
		return true;

	return super::add(item);
}

bool Inventory::removeItem( const ItemInstance* samePtr ) {
	for (int i = MAX_SELECTION_SIZE; i < (int)items.size(); ++i) {
		if (items[i] == samePtr) {
			clearSlot(i);
			return true;
		}
	}
	return false;
}
