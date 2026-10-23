#include "ToolRecipes.h"
#include "Recipes.h"
#include "../ShearsItem.h"
#include "../../level/tile/Tile.h"


static RowList shapes[] = {
	Recipes::Shape(	"XXX", //
					" # ",//
					" # "),//

	Recipes::Shape(	"X",//
					"#",//
					"#"),//

	Recipes::Shape(	"XX",//
					"X#",//
					" #"),//

	Recipes::Shape(	"XX",//
					" #",//
					" #")//
};

void ToolRecipes::addRecipes( Recipes* r )
{
	int materialIds[] = {Tile::wood->id, Tile::stoneBrick->id, Item::ironIngot->id, Item::emerald->id, Item::goldIngot->id};
	const int NumMaterials = sizeof(materialIds) / sizeof(int);
	const int NumRecipes = sizeof(shapes) / sizeof(RowList);

	Item* map[NumRecipes][NumMaterials] = {
		{Item::pickAxe_wood, Item::pickAxe_stone, Item::pickAxe_iron, Item::pickAxe_emerald, Item::pickAxe_gold}, 
		{Item::shovel_wood, Item::shovel_stone, Item::shovel_iron, Item::shovel_emerald, Item::shovel_gold}, 
		{Item::hatchet_wood, Item::hatchet_stone, Item::hatchet_iron, Item::hatchet_emerald, Item::hatchet_gold}, 
		{Item::hoe_wood, Item::hoe_stone, Item::hoe_iron, Item::hoe_emerald, Item::hoe_gold}, 
	};

	//const int OVERRIDDEN_MaterialCount = 2;
	for (int m = 0; m < NumMaterials; m++) {
		int mId = materialIds[m];
		for (int t = 0; t < NumRecipes; t++) {
			Item* target = (Item*) map[t][m];

			if (mId < 256) { // Tile
				r->addShapedRecipe( ItemInstance(target), shapes[t],
					definition('#', Item::stick, 'X', Tile::tiles[mId]) );
			} else { // Item
				r->addShapedRecipe( ItemInstance(target), shapes[t],
					definition('#', Item::stick, 'X', Item::items[mId]) );
			}
		}
	}

	// Coral: from coral ingots, as iron's.
	r->addShapedRecipe(ItemInstance(Item::pickAxe_coral), shapes[0], definition('#', Item::stick, 'X', Item::coralIngot));
	r->addShapedRecipe(ItemInstance(Item::shovel_coral), shapes[1], definition('#', Item::stick, 'X', Item::coralIngot));
	r->addShapedRecipe(ItemInstance(Item::hatchet_coral), shapes[2], definition('#', Item::stick, 'X', Item::coralIngot));

	r->addShapedRecipe( ItemInstance(Item::shears), //
	        " #", //
	        "# ", //

	        definition('#', Item::ironIngot));
}
