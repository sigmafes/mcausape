#ifndef NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__
#define NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"
#include "../level/tile/PortalTile.h"
#include "../level/tile/FireTile.h"
#include "../entity/player/Player.h"
#include "../Facing.h"

class FlintAndSteelItem: public Item
{
    typedef Item super;
public:
    FlintAndSteelItem(int id)
    :   super(id)
    {
		setMaxStackSize(1);
		setMaxDamage(64);
    }

    bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
        // Using flint & steel on the obsidian frame of a portal lights it
        // (1.0 behavior). This version's fire block has its logic disabled
        // (@fire), so a failed ignition does not place fire either.
        if (level->getTile(x, y, z) != Tile::obsidian->id) {
            // Anywhere else, the flint lights a fire on the free block in front of it.
            int fx = x, fy = y, fz = z;
            switch (face) {
                case Facing::DOWN : fy--; break;
                case Facing::UP   : fy++; break;
                case Facing::NORTH: fz--; break;
                case Facing::SOUTH: fz++; break;
                case Facing::WEST : fx--; break;
                case Facing::EAST : fx++; break;
            }
            if (!level->isEmptyTile(fx, fy, fz) || !Tile::fire->mayPlace(level, fx, fy, fz, face))
                return false;
            level->setTile(fx, fy, fz, Tile::fire->id);
            level->playSound(fx + 0.5f, fy + 0.5f, fz + 0.5f, "fire.ignite", 1, 1);
            instance->hurt(1);
            return true;
        }

        switch (face) {
            case Facing::DOWN : y--; break;
            case Facing::UP   : y++; break;
            case Facing::NORTH: z--; break;
            case Facing::SOUTH: z++; break;
            case Facing::WEST : x--; break;
            case Facing::EAST : x++; break;
        }

        if (((PortalTile*) Tile::portalTile)->trySpawnPortal(level, x, y, z)) {
            level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "fire.ignite", 1, 1);
            instance->hurt(1);
            return true;
        }

        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__FlintAndSteelItem_H__*/
