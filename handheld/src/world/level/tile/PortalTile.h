#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__PortalTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__PortalTile_H__

//package net.minecraft.world.level.tile;

// HalfTransparentTile.h relies on Tile.h being included first.
#include "Tile.h"
#include "HalfTransparentTile.h"

class Level;

// Port of MCPE 1.0's PortalBlock onto the 0.6.1 tile API: a half-transparent
// block without collision whose data stores the portal axis. It is ignited
// with flint & steel on an obsidian frame and cleans itself up (through
// neighborChanged) when the frame is broken.
class PortalTile: public HalfTransparentTile
{
public:
    static const int AXIS_UNKNOWN = 0;
    static const int AXIS_X = 1;
    static const int AXIS_Z = 2;

    static const int MIN_WIDTH = 2;
    static const int MAX_WIDTH = 21;
    static const int MIN_HEIGHT = 3;
    static const int MAX_HEIGHT = 21;

    PortalTile(int id, int tex)
    :	HalfTransparentTile(id, tex, Material::portal, false)
    {
    }

    static int getAxis(int data) {
        return (data == AXIS_X || data == AXIS_Z)? data : AXIS_UNKNOWN;
    }

    // No collision, like fire (see FireTile::getAABB).
    AABB* getAABB(Level* level, int x, int y, int z) {
        return NULL;
    }

    // A thin panel along the portal's axis, not a full block (1.0 does the
    // same). The axis is read from the neighbours, so it holds for both
    // orientations and does not depend on the data value having been saved.
    void updateShape(LevelSource* level, int x, int y, int z) {
        float xr = 0.125f;
        float zr = 0.125f;
        if (level->getTile(x - 1, y, z) == id || level->getTile(x + 1, y, z) == id) xr = 0.5f;
        else zr = 0.5f;
        setShape(0.5f - xr, 0, 0.5f - zr, 0.5f + xr, 1, 0.5f + zr);
    }

    bool isCubeShaped() {
        return false;
    }

    // Purple particles drifting into the panel, and the ambient hum.
    void animateTick(Level* level, int x, int y, int z, Random* random);

    // Not targetable: clicks pass through a portal, like in 1.0's survival.
    bool mayPick() {
        return false;
    }
    bool mayPick(int data, bool liquid) {
        return false;
    }

    // The portal is never dropped as an item.
    int getResourceCount(Random* random) {
        return 0;
    }

    void neighborChanged(Level* level, int x, int y, int z, int type);

    bool trySpawnPortal(Level* level, int x, int y, int z);

    // A player standing in the portal asks for the trip. The request only
    // records who asked; the level is swapped later by Minecraft::tick, never
    // from inside the entity update that called this.
    void entityInside(Level* level, int x, int y, int z, Entity* entity);

    // Builds an obsidian frame with a lit 2x3 portal in it, with (x, y, z) the
    // bottom-left interior cell. Used on arrival, where no portal exists yet.
    bool buildArrivalPortal(Level* level, int x, int y, int z);

    // The lowest block of the portal nearest to (x, z) within radius, if any.
    // A trip arrives at one that is already there before building another.
    bool findNearbyPortal(Level* level, int x, int z, int radius, int* outX, int* outY, int* outZ);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__PortalTile_H__*/
