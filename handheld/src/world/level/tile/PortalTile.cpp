#include "PortalTile.h"
#include "FireTile.h"
#include "../Level.h"
#include "../../entity/Entity.h"
#include "../../entity/EntityTypes.h"
#include "../../../util/Random.h"
#include "../../Facing.h"

// Port of 1.0's PortalShape (world/level/block/PortalBlock.cpp) onto the
// 0.6.1 Level API. The shape scans a rectangle of empty blocks (air, fire or
// portal) surrounded by obsidian: 2x3 up to 21x21.
class PortalShape
{
public:
    PortalShape(Level* level, int x, int y, int z, int axis);

    int getHeight() {
        return mHeight;
    }
    int getWidth() {
        return mWidth;
    }
    int getNumberOfPortalBlocks() {
        return mNumPortalBlocks;
    }

    bool isValid();
    void createPortalBlocks();

private:
    int getDistanceUntilEdge(int x, int y, int z, int direction);
    int calculatePortalHeight();
    bool isEmptyBlock(int tile);

    Level* mLevel;
    int mAxis;
    int mRightDir;
    int mLeftDir;
    int mNumPortalBlocks;
    int mBottomLeftX, mBottomLeftY, mBottomLeftZ;
    bool mBottomLeftValid;
    int mHeight;
    int mWidth;
};

static void stepTowards(int dir, int n, int& x, int& y, int& z) {
    switch (dir) {
        case Facing::DOWN : y -= n; break;
        case Facing::UP   : y += n; break;
        case Facing::NORTH: z -= n; break;
        case Facing::SOUTH: z += n; break;
        case Facing::WEST : x -= n; break;
        case Facing::EAST : x += n; break;
    }
}

//------------------------------------------------------------------------------

void PortalTile::neighborChanged(Level* level, int x, int y, int z, int type) {
    int axis = getAxis(level->getData(x, y, z));

    if (axis == AXIS_X || axis == AXIS_Z) {
        PortalShape shape(level, x, y, z, axis);
        if (!shape.isValid()
            || shape.getNumberOfPortalBlocks() < shape.getWidth() * shape.getHeight()) {
            level->setTile(x, y, z, 0);
        }
    }
}

void PortalTile::animateTick(Level* level, int x, int y, int z, Random* random) {
    // No portal sound ships in this version's sound bank, so until one is added
    // to SoundEngine this is silent: an unknown name is ignored, not an error.
    if (random->nextInt(100) == 0) {
        level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "portal.portal", 0.5f, random->nextFloat() * 0.4f + 0.8f);
    }

    bool alongX = level->getTile(x - 1, y, z) == id || level->getTile(x + 1, y, z) == id;
    for (int i = 0; i < 4; i++) {
        float xp = x + random->nextFloat();
        float yp = y + random->nextFloat();
        float zp = z + random->nextFloat();
        float xa = (random->nextFloat() - 0.5f) * 0.5f;
        float ya = (random->nextFloat() - 0.5f) * 0.5f;
        float za = (random->nextFloat() - 0.5f) * 0.5f;
        int flip = random->nextInt(2) * 2 - 1;

        // Particles start on one face of the panel and are pulled through it.
        if (alongX) {
            zp = z + 0.5f + 0.25f * flip;
            za = random->nextFloat() * 2 * flip;
        } else {
            xp = x + 0.5f + 0.25f * flip;
            xa = random->nextFloat() * 2 * flip;
        }

        level->addParticle(PARTICLETYPE(portal), xp, yp, zp, xa, ya, za);
    }
}

void PortalTile::entityInside(Level* level, int x, int y, int z, Entity* entity) {
    // Only the client-side tab's own player travels; remote players on a
    // server-side level are left to the network code.
    if (level->isClientSide || !entity->isPlayer()) return;
    level->requestPortalTravel(entity);
}

bool PortalTile::findNearbyPortal(Level* level, int x, int z, int radius, int* outX, int* outY, int* outZ) {
    int bestDist = -1;
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dz = -radius; dz <= radius; dz++) {
            int px = x + dx;
            int pz = z + dz;
            if (px < 0 || pz < 0) continue;
            int dist = dx * dx + dz * dz;
            if (bestDist >= 0 && dist >= bestDist) continue;
            for (int py = 1; py < 127; py++) {
                if (level->getTile(px, py, pz) == id && level->getTile(px, py - 1, pz) != id) {
                    bestDist = dist;
                    *outX = px;
                    *outY = py;
                    *outZ = pz;
                    break;
                }
            }
        }
    }
    return bestDist >= 0;
}

bool PortalTile::buildArrivalPortal(Level* level, int x, int y, int z) {
    // Clear a room of air around the portal so the arrival is never inside rock.
    // No floor is built: the only obsidian is the frame itself, below.
    for (int dx = -2; dx <= 3; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            for (int dy = 0; dy <= 3; dy++)
                level->setTile(x + dx, y + dy, z + dz, 0);
        }
    }

    // Obsidian floor and ceiling run one block past the interior on each side,
    // as in the 1.0 frame, so PortalShape finds the same 2x3 shape it would
    // find on a hand-built frame.
    for (int i = -1; i <= 2; i++) {
        level->setTile(x + i, y - 1, z, Tile::obsidian->id);
        level->setTile(x + i, y + 3, z, Tile::obsidian->id);
    }
    for (int j = 0; j <= 2; j++) {
        level->setTile(x - 1, y + j, z, Tile::obsidian->id);
        level->setTile(x + 2, y + j, z, Tile::obsidian->id);
        // Clear the interior so the arrival cannot be blocked by lava or rock.
        level->setTile(x, y + j, z, 0);
        level->setTile(x + 1, y + j, z, 0);
    }
    return trySpawnPortal(level, x, y, z);
}

bool PortalTile::trySpawnPortal(Level* level, int x, int y, int z) {
    PortalShape xSize(level, x, y, z, AXIS_X);
    if (xSize.isValid() && xSize.getNumberOfPortalBlocks() == 0) {
        xSize.createPortalBlocks();
        return true;
    }

    PortalShape zSize(level, x, y, z, AXIS_Z);
    if (zSize.isValid() && zSize.getNumberOfPortalBlocks() == 0) {
        zSize.createPortalBlocks();
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------

PortalShape::PortalShape(Level* level, int x, int y, int z, int axis)
:   mLevel(level),
    mAxis(axis),
    mNumPortalBlocks(0),
    mBottomLeftValid(false),
    mHeight(0),
    mWidth(0)

{
    if (axis == PortalTile::AXIS_X) {
        mLeftDir = Facing::EAST;
        mRightDir = Facing::WEST;
    } else {
        mLeftDir = Facing::NORTH;
        mRightDir = Facing::SOUTH;
    }

    int originalY = y;
    while (y > originalY - PortalTile::MAX_HEIGHT && y > 0
        && isEmptyBlock(mLevel->getTile(x, y - 1, z))) {
        y--;
    }

    int edge = getDistanceUntilEdge(x, y, z, mLeftDir) - 1;
    if (edge >= 0) {
        stepTowards(mLeftDir, edge, x, y, z);
        mBottomLeftX = x;
        mBottomLeftY = y;
        mBottomLeftZ = z;
        mBottomLeftValid = true;

        mWidth = getDistanceUntilEdge(mBottomLeftX, mBottomLeftY, mBottomLeftZ, mRightDir);

        if (mWidth < PortalTile::MIN_WIDTH || mWidth > PortalTile::MAX_WIDTH) {
            mBottomLeftValid = false;
            mWidth = 0;
        }
    }

    if (mBottomLeftValid) {
        mHeight = calculatePortalHeight();
    }
}

int PortalShape::getDistanceUntilEdge(int x, int y, int z, int direction) {
    int width = 0;

    for (; width < PortalTile::MAX_WIDTH + 1; width++) {
        int px = x, py = y, pz = z;
        stepTowards(direction, width, px, py, pz);

        if (!isEmptyBlock(mLevel->getTile(px, py, pz))) {
            break; // Something is blocking the portal
        }

        if (mLevel->getTile(px, py - 1, pz) != Tile::obsidian->id) {
            break; // There's no frame below this block
        }
    }

    int px = x, py = y, pz = z;
    stepTowards(direction, width, px, py, pz);
    if (mLevel->getTile(px, py, pz) == Tile::obsidian->id) {
        return width;
    }

    return 0;
}

int PortalShape::calculatePortalHeight() {
    bool shapeFailed = false;

    for (mHeight = 0; mHeight < PortalTile::MAX_HEIGHT; mHeight++) {
        for (int i = 0; i < mWidth; i++) {
            int px = mBottomLeftX, py = mBottomLeftY + mHeight, pz = mBottomLeftZ;
            stepTowards(mRightDir, i, px, py, pz);

            int tile = mLevel->getTile(px, py, pz);
            if (!isEmptyBlock(tile)) {
                shapeFailed = true;
                break; // Something is blocking the portal
            }

            if (tile == Tile::portalTile->id) {
                mNumPortalBlocks++;
            }

            if (i == 0) {
                stepTowards(mLeftDir, 1, px, py, pz);
                if (mLevel->getTile(px, py, pz) != Tile::obsidian->id) {
                    shapeFailed = true;
                    break; // Missing left frame
                }
            } else if (i == mWidth - 1) {
                stepTowards(mRightDir, 1, px, py, pz);
                if (mLevel->getTile(px, py, pz) != Tile::obsidian->id) {
                    shapeFailed = true;
                    break; // Missing right frame
                }
            }
        }

        if (shapeFailed) {
            break;
        }
    }

    // The shape needs an obsidian ceiling
    for (int i = 0; i < mWidth; i++) {
        int px = mBottomLeftX, py = mBottomLeftY + mHeight, pz = mBottomLeftZ;
        stepTowards(mRightDir, i, px, py, pz);
        if (mLevel->getTile(px, py, pz) != Tile::obsidian->id) {
            mHeight = 0;
            break;
        }
    }

    if (mHeight > PortalTile::MAX_HEIGHT || mHeight < PortalTile::MIN_HEIGHT) {
        mBottomLeftValid = false;
        mWidth = 0;
        mHeight = 0;
        return 0;
    }

    return mHeight;
}

bool PortalShape::isEmptyBlock(int tile) {
    return tile == 0 || tile == Tile::fire->id || tile == Tile::portalTile->id;
}

bool PortalShape::isValid() {
    return mBottomLeftValid
        && mWidth >= PortalTile::MIN_WIDTH && mWidth <= PortalTile::MAX_WIDTH
        && mHeight >= PortalTile::MIN_HEIGHT && mHeight <= PortalTile::MAX_HEIGHT;
}

void PortalShape::createPortalBlocks() {
    for (int horizontal = 0; horizontal < mWidth; horizontal++) {
        for (int vertical = 0; vertical < mHeight; vertical++) {
            int px = mBottomLeftX, py = mBottomLeftY + vertical, pz = mBottomLeftZ;
            stepTowards(mRightDir, horizontal, px, py, pz);
            // NoUpdate: the neighbors must not re-check the shape while it is
            // still incomplete (1.0 uses UPDATE_CLIENTS for the same reason).
            mLevel->setTileAndDataNoUpdate(px, py, pz, Tile::portalTile->id, mAxis);
        }
    }
}
