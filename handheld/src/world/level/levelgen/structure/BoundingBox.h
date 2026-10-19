#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__BoundingBox_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__BoundingBox_H__

// A box of whole blocks, inclusive at both ends. Ported from the Console Edition's
// BoundingBox, which structure generation (the Nether fortress) is built on.
class BoundingBox
{
public:
	int x0, y0, z0, x1, y1, z1;

	BoundingBox();
	BoundingBox(const BoundingBox* other);
	BoundingBox(int x0, int y0, int z0, int x1, int y1, int z1);
	// A column the size of a chunk area, the whole height of the world.
	BoundingBox(int x0, int z0, int x1, int z1);

	static BoundingBox* getUnknownBox();
	// The box a piece of the given size takes when its "foot" is at footX/Y/Z and
	// it faces the given Direction.
	static BoundingBox* orientBox(int footX, int footY, int footZ, int offX, int offY, int offZ, int width, int height, int depth, int orientation);

	bool intersects(const BoundingBox* other) const;
	bool intersects(int x0, int y0, int z0, int x1, int y1, int z1) const;
	bool intersects(int x0, int z0, int x1, int z1) const;
	void expand(const BoundingBox* other);
	void move(int dx, int dy, int dz);
	bool isInside(int x, int y, int z) const;

	int getXSpan() const { return x1 - x0 + 1; }
	int getYSpan() const { return y1 - y0 + 1; }
	int getZSpan() const { return z1 - z0 + 1; }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_STRUCTURE__BoundingBox_H__*/
