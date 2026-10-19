#include "BoundingBox.h"
#include "../../../Direction.h"
#include <climits>

static int imin(int a, int b) { return a < b? a : b; }
static int imax(int a, int b) { return a > b? a : b; }

BoundingBox::BoundingBox()
:	x0(0), y0(0), z0(0), x1(0), y1(0), z1(0)
{
}

BoundingBox::BoundingBox(const BoundingBox* other)
:	x0(other->x0), y0(other->y0), z0(other->z0), x1(other->x1), y1(other->y1), z1(other->z1)
{
}

BoundingBox::BoundingBox(int x0, int y0, int z0, int x1, int y1, int z1)
:	x0(x0), y0(y0), z0(z0), x1(x1), y1(y1), z1(z1)
{
}

BoundingBox::BoundingBox(int x0, int z0, int x1, int z1)
:	x0(x0), y0(1), z0(z0), x1(x1), y1(512), z1(z1)
{
}

BoundingBox* BoundingBox::getUnknownBox()
{
	return new BoundingBox(INT_MAX, INT_MAX, INT_MAX, INT_MIN, INT_MIN, INT_MIN);
}

BoundingBox* BoundingBox::orientBox(int footX, int footY, int footZ, int offX, int offY, int offZ, int width, int height, int depth, int orientation)
{
	switch (orientation) {
	default:
		return new BoundingBox(footX + offX, footY + offY, footZ + offZ, footX + width - 1 + offX, footY + height - 1 + offY, footZ + depth - 1 + offZ);
	case Direction::NORTH:
		// foot is at x0, y0, z1
		return new BoundingBox(footX + offX, footY + offY, footZ - depth + 1 + offZ, footX + width - 1 + offX, footY + height - 1 + offY, footZ + offZ);
	case Direction::SOUTH:
		// foot is at x0, y0, z0
		return new BoundingBox(footX + offX, footY + offY, footZ + offZ, footX + width - 1 + offX, footY + height - 1 + offY, footZ + depth - 1 + offZ);
	case Direction::WEST:
		// foot is at x1, y0, z0, but width and depth are flipped
		return new BoundingBox(footX - depth + 1 + offZ, footY + offY, footZ + offX, footX + offZ, footY + height - 1 + offY, footZ + width - 1 + offX);
	case Direction::EAST:
		// foot is at x0, y0, z0, but width and depth are flipped
		return new BoundingBox(footX + offZ, footY + offY, footZ + offX, footX + depth - 1 + offZ, footY + height - 1 + offY, footZ + width - 1 + offX);
	}
}

bool BoundingBox::intersects(const BoundingBox* other) const
{
	return !(x1 < other->x0 || x0 > other->x1 || z1 < other->z0 || z0 > other->z1 || y1 < other->y0 || y0 > other->y1);
}

bool BoundingBox::intersects(int ox0, int oy0, int oz0, int ox1, int oy1, int oz1) const
{
	return !(x1 < ox0 || x0 > ox1 || z1 < oz0 || z0 > oz1 || y1 < oy0 || y0 > oy1);
}

bool BoundingBox::intersects(int ox0, int oz0, int ox1, int oz1) const
{
	return !(x1 < ox0 || x0 > ox1 || z1 < oz0 || z0 > oz1);
}

void BoundingBox::expand(const BoundingBox* other)
{
	x0 = imin(x0, other->x0);
	y0 = imin(y0, other->y0);
	z0 = imin(z0, other->z0);
	x1 = imax(x1, other->x1);
	y1 = imax(y1, other->y1);
	z1 = imax(z1, other->z1);
}

void BoundingBox::move(int dx, int dy, int dz)
{
	x0 += dx; y0 += dy; z0 += dz;
	x1 += dx; y1 += dy; z1 += dz;
}

bool BoundingBox::isInside(int x, int y, int z) const
{
	return x >= x0 && x <= x1 && z >= z0 && z <= z1 && y >= y0 && y <= y1;
}
