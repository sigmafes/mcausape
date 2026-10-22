#ifndef NET_MINECRAFT_CLIENT_RENDERER__TerrainAtlas_H__
#define NET_MINECRAFT_CLIENT_RENDERER__TerrainAtlas_H__

/** Where a block texture number is in terrain.png. docs/terrain-atlas-plan.md.

    The atlas is 512x512 in four 256x256 quadrants, each laid out exactly as
    the old 256x256 atlas was (16 cells of 16 px a side). Numbers 0-255 are the
    old atlas, in the top left, unmoved: every texture number written into the
    blocks since 2013 still means the same cell, and tex + 1 / tex + 16 (the
    flowing water's 2x2, growth stages, door halves) still step to the next
    cell or row inside a quadrant. 256-511 are the top right, 512-767 the
    bottom left, 768-1023 the bottom right.

    Everything that turns a block texture number into pixels or UVs goes
    through here; item icons (gui/items.png, still 256) do not. */
namespace TerrainAtlas
{
	static const int Size = 512;
	static const float SizeF = 512.0f;

	/// Left edge of the cell, in pixels.
	inline int px(int tex) { return ((tex >> 8) & 1) * 256 + (tex & 15) * 16; }
	/// Top edge of the cell, in pixels.
	inline int py(int tex) { return ((tex >> 9) & 1) * 256 + ((tex >> 4) & 15) * 16; }
}

#endif
