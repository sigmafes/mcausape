#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__PumpkinTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__PumpkinTile_H__

#include "Tile.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../entity/Mob.h"
#include "../../../util/Mth.h"

/** A pumpkin, or -- lit -- a jack o' lantern. Both have a face on one side,
    toward whoever placed them; which side is kept in the data (0 to 3).

    Pictures in terrain.png: top (6,6), side (6,7), face (7,7), lit face (8,7). */
class PumpkinTile: public Tile
{
	typedef Tile super;
public:
	PumpkinTile(int id, bool lit)
	:	super(id, 6 + 7 * 16, Material::vegetable),
		lit(lit)
	{}

	int getTexture(int face, int data) {
		if (face == 0 || face == 1)
			return 6 + 6 * 16;

		// Data 2 shows its face to the north (face 2), 3 to the east (5), 0 to the
		// south (3) and 1 to the west (4): the same table Minecraft uses.
		const bool front = (data == 2 && face == 2) || (data == 3 && face == 5)
			|| (data == 0 && face == 3) || (data == 1 && face == 4);
		if (front)
			return lit ? 8 + 7 * 16 : 7 + 7 * 16;
		return 6 + 7 * 16;
	}

	void setPlacedBy(Level* level, int x, int y, int z, Mob* by) {
		const int dir = Mth::floor(by->yRot * 4 / 360 + 2.5f) & 3;
		level->setData(x, y, z, dir);
	}

private:
	bool lit;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__PumpkinTile_H__*/
