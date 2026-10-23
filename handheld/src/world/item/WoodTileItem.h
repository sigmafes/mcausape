#ifndef NET_MINECRAFT_WORLD_ITEM__WoodTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__WoodTileItem_H__

//package net.minecraft.world.item;

#include "AuxDataTileItem.h"
#include "../level/tile/WoodTile.h"

class WoodTileItem: public AuxDataTileItem
{
    typedef AuxDataTileItem super;
public:
    WoodTileItem(int id, Tile* parentTile)
    :   super(id, parentTile)
    {
    }

    /*@Override*/
    std::string getDescriptionId(const ItemInstance* instance) const {
        int auxValue = instance->getAuxValue();
        if (auxValue == WoodTile::SPRUCE_PLANKS)
            return super::getDescriptionId() + ".spruce";
        if (auxValue == WoodTile::BIRCH_PLANKS)
            return super::getDescriptionId() + ".birch";
        if (auxValue == WoodTile::CRIMSON_PLANKS)
            return super::getDescriptionId() + ".crimson";
        if (auxValue == WoodTile::WARPED_PLANKS)
            return super::getDescriptionId() + ".warped";
        // Oak keeps the plain "tile.wood" name
        return super::getDescriptionId();
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__WoodTileItem_H__*/
