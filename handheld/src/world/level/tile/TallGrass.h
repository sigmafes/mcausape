#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__TallGrass_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__TallGrass_H__
#include "Bush.h"
class Pos;
class TallGrass : public Bush {
public:
	typedef Bush super;

public:
	static const int DEAD_SHRUB = 0;
	static const int TALL_GRASS = 1;
	static const int FERN = 3;
	/// A snow layer shares the block (worldgen put snow where it stood).
	static const int SnowyBit = 8;
	static int grassColorAt(LevelSource* level, int x, int z);


	TallGrass(int id, int tex);
	int getTexture(int face, int data);
	int getColor();
	int getColor(int auxData);
	int getColor(LevelSource* level, int x, int y, int z);
	int getResource(int data, Random* random);
	void playerDestroy(Level* level, Player* player, int x, int y, int z, int data);
	void destroy(Level* level, int x, int y, int z, int data);
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE__TallGrass_H__ */