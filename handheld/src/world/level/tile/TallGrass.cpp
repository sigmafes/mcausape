#include "TallGrass.h"
#include "../biome/Biome.h"
#include "../FoliageColor.h"
#include "../../entity/player/Player.h"
#include "../../item/Item.h"
#include "../../item/ShearsItem.h"

TallGrass::TallGrass( int id, int tex ) : super(id, tex, Material::replaceable_plant) {
	float ss = 0.4f;
	setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, 0.8f, 0.5f + ss);
}

int TallGrass::getTexture( int face, int data ) {
	data &= 7;	// bit 8: a snow layer in the same block (SnowyBit)
	if(data == TALL_GRASS) return tex;
	if(data == FERN) return tex + 16 + 1;
	if(data == DEAD_SHRUB) return tex + 16;
	return tex;
}

int TallGrass::getColor() {
	/*double temp = 0.5;
	double rain = 1.0;
	return GrassColor.get(temp, rain);*/
	return 0x339933;
}

int TallGrass::getColor( int auxData ) {
	if(auxData == DEAD_SHRUB) return 0xffffffff;
	return FoliageColor::getDefaultColor();
}

int TallGrass::getColor( LevelSource* level, int x, int y, int z ) {
	int d = level->getData(x, y, z) & 7;
	if (d == DEAD_SHRUB) return 0xffffff;
	return grassColorAt(level, x, z);
}

int TallGrass::grassColorAt( LevelSource* level, int x, int z ) {
	// In the cold the grass takes the spruce needles' colour.
	Biome* b = level->getBiome(x, z);
	if (b == Biome::taiga || b == Biome::tundra || b == Biome::iceDesert)
		return FoliageColor::getEvergreenColor();
	return 0x339933;
}

void TallGrass::destroy( Level* level, int x, int y, int z, int data ) {
	// The snow it stood in stays behind.
	if ((data & SnowyBit) && level->getTile(x, y, z) == 0)
		level->setTile(x, y, z, Tile::topSnow->id);
}

int TallGrass::getResource( int data, Random* random ) {
	if (random->nextInt(8) == 0) {
		return Item::seeds_wheat->id;
	}
	return -1;
}

void TallGrass::playerDestroy( Level* level, Player* player, int x, int y, int z, int data ) {
	if (!level->isClientSide && player->getSelectedItem() != NULL && player->getSelectedItem()->id == Item::shears->id) {
		//player->awardStat(Stats.blockMined[id], 1);

		// drop leaf block instead of sapling
		ItemInstance itemInstance(Tile::tallgrass, 1, data & 7);
		popResource(level, x, y, z, itemInstance);
	} else {
		super::playerDestroy(level, player, x, y, z, data);
	}
}
