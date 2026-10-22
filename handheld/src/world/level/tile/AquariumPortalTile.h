#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__AquariumPortalTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__AquariumPortalTile_H__

// HalfTransparentTile.h relies on Tile.h being included first.
#include "Tile.h"
#include "HalfTransparentTile.h"

class Level;
class Entity;

/** The Aquarium's portal (docs/aquarium-plan.md): lying flat, 2x2, in a
    frame of 8 ocean crystal blocks (4x4 without its corners), lit by pouring
    a water bucket into it. Every one in the Overworld leads to the fixed
    portal in the middle of the Aquarium, and that one leads back to the last
    Overworld portal the player took. The swirl is the Nether portal's, in
    cyan (PortalTexture). */
class AquariumPortalTile: public HalfTransparentTile
{
public:
	AquariumPortalTile(int id, int tex)
	:	HalfTransparentTile(id, tex, Material::portal, false)
	{
		// A sheet lying in the bottom of the frame, like the End's.
		setShape(0, 0, 0, 1, 0.75f, 1);
	}

	AABB* getAABB(Level* level, int x, int y, int z) { return NULL; }
	bool isCubeShaped() { return false; }
	bool mayPick() { return false; }
	bool mayPick(int data, bool liquid) { return false; }
	int getResourceCount(Random* random) { return 0; }

	// Only the faces at the edge of the portal: none between its four cells.
	bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {
		if (level->getTile(x, y, z) == id) return false;
		return Tile::shouldRenderFace(level, x, y, z, face);
	}

	void neighborChanged(Level* level, int x, int y, int z, int type);
	void entityInside(Level* level, int x, int y, int z, Entity* entity);
	void animateTick(Level* level, int x, int y, int z, Random* random);

	/// Water poured at (x, y, z): if that is one of the four middle cells of a
	/// whole frame, the four become portal. True if they did.
	static bool tryLight(Level* level, int x, int y, int z);
	/// Is (x, y, z) a frame block, of either kind?
	static bool isFrame(Level* level, int x, int y, int z);
	/// The cell of the portal's 2x2 with the lowest x and z, from any of its cells.
	static void cornerOf(Level* level, int x, int y, int z, int* cx, int* cz);
	/// The 8 frame blocks around a 2x2 whose low corner is (cx, y, cz) are there.
	static bool frameWhole(Level* level, int cx, int y, int cz);
};

#endif
