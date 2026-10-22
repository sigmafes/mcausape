#include "BrewingStandTile.h"
#include "entity/BrewingStandTileEntity.h"
#include "../Level.h"
#include "../../entity/item/ItemEntity.h"
#include "../../entity/player/Player.h"
#include "../../item/ItemInstance.h"

BrewingStandTile::BrewingStandTile(int id)
:	super(id, 13 + 9 * 16, Material::metal),
	drawingBase(false)
{
	setShape(7 / 16.0f, 0, 7 / 16.0f, 9 / 16.0f, 14 / 16.0f, 9 / 16.0f);
}

TileEntity* BrewingStandTile::newTileEntity()
{
	return TileEntityFactory::createTileEntity(TileEntityType::BrewingStand);
}

bool BrewingStandTile::use(Level* level, int x, int y, int z, Player* player)
{
	if (level->isClientSide)
		return true;
	// Stands placed before brewing existed have no tile entity: give them one.
	if (level->getTileEntity(x, y, z) == NULL)
		level->setTileEntity(x, y, z, newTileEntity());
	BrewingStandTileEntity* stand = static_cast<BrewingStandTileEntity*>(level->getTileEntity(x, y, z));
	if (stand != NULL) player->openBrewingStand(stand);
	return true;
}

void BrewingStandTile::onRemove(Level* level, int x, int y, int z)
{
	// What is in it falls out, as from a furnace (FurnaceTile::onRemove).
	if (!level->isClientSide) {
		BrewingStandTileEntity* stand = (BrewingStandTileEntity*) level->getTileEntity(x, y, z);
		if (stand != NULL) {
			for (int i = 0; i < stand->getContainerSize(); i++) {
				ItemInstance* item = stand->getItem(i);
				if (!item || item->isNull()) continue;
				ItemEntity* drop = new ItemEntity(level, x + 0.5f, y + 0.5f, z + 0.5f, ItemInstance(item->id, item->count, item->getAuxValue()));
				drop->yd = 0.2f;
				level->addEntity(drop);
				item->setNull();
			}
		}
	}
	super::onRemove(level, x, y, z);
}
