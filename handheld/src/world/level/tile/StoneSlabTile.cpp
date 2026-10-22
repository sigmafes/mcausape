#include "StoneSlabTile.h"
#include "WoodTile.h"
#include "QuartzBlockTile.h"
#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"
#include "../../Facing.h"

const std::string StoneSlabTile::SLAB_NAMES[] = {
	"stone", "sand", "wood", "cobble", "brick", "smoothStoneBrick", "birch", "spruce"
};
const int StoneSlabTile::SLAB_NAMES_COUNT = sizeof(SLAB_NAMES) / sizeof(std::string);
const std::string StoneSlabTile::SLAB_NAMES2[] = {
	"netherBrick", "quartz", "prismarineBricks", "darkPrismarine"
};
const int StoneSlabTile::SLAB_NAMES2_COUNT = sizeof(SLAB_NAMES2) / sizeof(std::string);

StoneSlabTile::StoneSlabTile(int id, bool fullSize, int slabSet)
:	Tile(id, 6, Material::stone)
{
    this->fullSize = fullSize;
    this->slabSet = slabSet;

    if (!fullSize) {
        setShape(0, 0, 0, 1, 0.5f, 1);
    }
    setLightBlock(255);
}

void StoneSlabTile::updateShape(LevelSource* level, int x, int y, int z)
{
	if (fullSize) {
		setShape(0, 0, 0, 1, 1, 1);
	} else {
		bool upper = (level->getData(x, y, z) & TOP_SLOT_BIT) != 0;
		if (upper) {
			setShape(0, 0.5f, 0, 1, 1, 1);
		} else {
			setShape(0, 0, 0, 1, 0.5f, 1);
		}
	}
}

void StoneSlabTile::updateDefaultShape()
{
	if (fullSize) {
		setShape(0, 0, 0, 1, 1, 1);
	} else {
		setShape(0, 0, 0, 1, 0.5f, 1);
	}
}

Tile* StoneSlabTile::getHalfTile() {
	return slabSet == 1? Tile::stoneSlabHalf2 : Tile::stoneSlabHalf;
}

Tile* StoneSlabTile::getFullTile() {
	return slabSet == 1? Tile::stoneSlab2 : Tile::stoneSlab;
}

bool StoneSlabTile::isHalfSlabId(int tileId) {
	return tileId == Tile::stoneSlabHalf->id || tileId == Tile::stoneSlabHalf2->id;
}

int StoneSlabTile::getTexture(int face, int data) {
	data = data & TYPE_MASK;
	if (slabSet == 1) {
		if (data == QUARTZ_SLAB) return Tile::quartzBlock->getTexture(face, QuartzBlockTile::TYPE_DEFAULT);
		if (data == PRISMARINE_BRICK_SLAB) return Tile::prismarineBricks->tex;
		if (data == DARK_PRISMARINE_SLAB) return Tile::darkPrismarine->tex;
		return Tile::netherBrick->tex;
	}
    if (data == STONE_SLAB) {
        if (face <= 1) return 6;
        return 5;
    } else if (data == SAND_SLAB) {
        if (face == Facing::DOWN) return 13 * 16;
        if (face == Facing::UP) return 11 * 16;
        return 12 * 16;
    } else if (data == WOOD_SLAB) {
        return Tile::wood->getTexture(face, WoodTile::OAK_PLANKS);
    } else if (data == BIRCH_SLAB) {
        return Tile::wood->getTexture(face, WoodTile::BIRCH_PLANKS);
    } else if (data == SPRUCE_SLAB) {
        return Tile::wood->getTexture(face, WoodTile::SPRUCE_PLANKS);
    } else if (data == COBBLESTONE_SLAB) {
        return 16;
	} else if (data == BRICK_SLAB) {
		return Tile::redBrick->tex;
	} else if (data == SMOOTHBRICK_SLAB) {
		return Tile::stoneBrickSmooth->tex;
	}
    return 6;
}

int StoneSlabTile::getTexture(int face) {
    return getTexture(face, 0);
}

bool StoneSlabTile::isSolidRender() {
    return fullSize;
}

int StoneSlabTile::getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue)
{
	if (fullSize) return itemValue;

	if (face == Facing::DOWN || (face != Facing::UP && clickY > 0.5)) {
		return itemValue | TOP_SLOT_BIT;
	}
	return itemValue;
}

/*
void StoneSlabTile::onPlace(Level* level, int x, int y, int z) {
    if (this != Tile::stoneSlabHalf) Tile::onPlace(level, x, y, z);
    int below = level->getTile(x, y - 1, z);

    int myData = level->getData(x, y, z);
    int belowData = level->getData(x, y - 1, z);

    if (myData != belowData) {
        return;
    }

    if (below == Tile::stoneSlabHalf->id) {
        level->setTile(x, y, z, 0);
        level->setTileAndData(x, y - 1, z, Tile::stoneSlab->id, myData);
    }
}
*/

int StoneSlabTile::getResource(int data, Random* random) {
    return getHalfTile()->id;
}

int StoneSlabTile::getResourceCount(Random* random) {
    if (fullSize) {
        return 2;
    }
    return 1;
}

bool StoneSlabTile::isCubeShaped() {
    return fullSize;
}

void StoneSlabTile::addAABBs( Level* level, int x, int y, int z, const AABB* box, std::vector<AABB>& boxes ) {
	updateShape(level, x, y, z);
	super::addAABBs(level, x, y, z, box, boxes);
}

static bool isHalfSlab(int tileId) {
	return StoneSlabTile::isHalfSlabId(tileId);
}

bool StoneSlabTile::shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {

	if (fullSize) return super::shouldRenderFace(level, x, y, z, face);

	if (face != Facing::UP && face != Facing::DOWN && !super::shouldRenderFace(level, x, y, z, face)) {
		return false;
	}

	int ox = x, oy = y, oz = z;
	ox += Facing::STEP_X[Facing::OPPOSITE_FACING[face]];
	oy += Facing::STEP_Y[Facing::OPPOSITE_FACING[face]];
	oz += Facing::STEP_Z[Facing::OPPOSITE_FACING[face]];

	bool isUpper = (level->getData(ox, oy, oz) & TOP_SLOT_BIT) != 0;
	if (isUpper) {
		if (face == Facing::DOWN) return true;
		if (face == Facing::UP && super::shouldRenderFace(level, x, y, z, face)) return true;
		return !(isHalfSlab(level->getTile(x, y, z)) && (level->getData(x, y, z) & TOP_SLOT_BIT) != 0);
	} else {
		if (face == Facing::UP) return true;
		if (face == Facing::DOWN && super::shouldRenderFace(level, x, y, z, face)) return true;
		return !(isHalfSlab(level->getTile(x, y, z)) && (level->getData(x, y, z) & TOP_SLOT_BIT) == 0);
	}
}

int StoneSlabTile::getSpawnResourcesAuxValue(int data) {
    return data & TYPE_MASK;
}
