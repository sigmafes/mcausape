#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__NetherSurfaceTiles_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__NetherSurfaceTiles_H__

#include "Tile.h"
#include "../material/Material.h"

/// Nylium (crimson or warped): netherrack with a living top. Its top at tex,
/// its sides at tex + 1, netherrack underneath; mined, it is netherrack.
class NyliumTile: public Tile
{
public:
	NyliumTile(int id, int tex) : Tile(id, tex, Material::stone) {}
	int getTexture(int face) {
		if (face == 1) return tex;
		if (face == 0) return Tile::netherrack->tex;
		return tex + 1;
	}
	int getTexture(int face, int data) { return getTexture(face); }
	int getResource(int data, Random* random) { return Tile::netherrack->id; }
};

/// A block with its own top and bottom (tex + 1) and sides (tex): basalt,
/// blackstone. (Basalt is a pillar in Java; upright only, for now.)
class TopSideTile: public Tile
{
public:
	TopSideTile(int id, int tex, const Material* m) : Tile(id, tex, m) {}
	int getTexture(int face) { return face <= 1 ? tex + 1 : tex; }
	int getTexture(int face, int data) { return getTexture(face); }
};

#endif
