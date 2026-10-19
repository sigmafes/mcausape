#ifndef NET_MINECRAFT_WORLD_ITEM__StoneSlabTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__StoneSlabTileItem_H__

//package net.minecraft.world.item;

#include "TileItem.h"
#include "../Facing.h"
#include "../level/Level.h"
#include "../entity/player/Player.h"
#include "../level/tile/StoneSlabTile.h"
#include "../level/tile/Tile.h"

class StoneSlabTileItem: public TileItem
{
	typedef TileItem super;
public:
	// slabSet 0 is the original slab, 1 the nether brick / quartz one.
    StoneSlabTileItem(int id, int slabSet = 0)
	:	super(id),
		slabSet(slabSet)
	{
        setMaxDamage(0);
        setStackedByData(true);
    }

    /*@Override*/
    int getIcon(int itemAuxValue) {
        return halfTile()->getTexture(2, itemAuxValue);
    }

    /*@Override*/
    int getLevelDataForAuxValue(int auxValue) {
        return auxValue;
    }

    /*@Override*/
    std::string getDescriptionId(const ItemInstance* instance) const {
		int auxValue = instance->getAuxValue();
		const int count = slabSet == 1? StoneSlabTile::SLAB_NAMES2_COUNT : StoneSlabTile::SLAB_NAMES_COUNT;
        if (auxValue < 0 || auxValue >= count)
            auxValue = 0;
		const std::string& name = slabSet == 1? StoneSlabTile::SLAB_NAMES2[auxValue] : StoneSlabTile::SLAB_NAMES[auxValue];
        return super::getDescriptionId() + "." + name;
    }

    /*@Override*/
    bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
        if (instance->count == 0) return false;
        //if (!player->mayBuild(x, y, z)) return false;

        int currentTile = level->getTile(x, y, z);
        int currentData = level->getData(x, y, z);
		int slabType = currentData & StoneSlabTile::TYPE_MASK;
		bool isUpper = (currentData & StoneSlabTile::TOP_SLOT_BIT) != 0;

		Tile* half = halfTile();
		Tile* full = fullTile();
        if (((face == Facing::UP && !isUpper) || (face == Facing::DOWN && isUpper)) && currentTile == half->id && slabType == instance->getAuxValue()) {
			bool unobstructed = level->isUnobstructed(*full->getAABB(level, x, y, z));
            if (unobstructed && level->setTileAndData(x, y, z, full->id, slabType)) {
                level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, full->soundType->getStepSound(), (full->soundType->getVolume() + 1) / 2, full->soundType->getPitch() * 0.8f);
                instance->count--;
            }
            return true;
        } else {
            return super::useOn(instance, player, level, x, y, z, face, clickX, clickY, clickZ);
        }
    }

private:
	Tile* halfTile() const { return slabSet == 1? Tile::stoneSlabHalf2 : Tile::stoneSlabHalf; }
	Tile* fullTile() const { return slabSet == 1? Tile::stoneSlab2 : Tile::stoneSlab; }

	int slabSet;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__StoneSlabTileItem_H__*/
