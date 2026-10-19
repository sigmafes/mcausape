#include "NetherWartTile.h"
#include "Tile.h"
#include "../Level.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../../entity/item/ItemEntity.h"
#include "../../../util/Random.h"

NetherWartTile::NetherWartTile(int id, int tex)
:	super(id, tex)
{
	setTicking(true);
	float ss = 0.5f;
	setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, 0.25f, 0.5f + ss);
}

bool NetherWartTile::mayPlaceOn(int tile)
{
	return tile == Tile::hellSand->id;
}

// Bush asks for light; the Nether has none to give, so only the soil counts.
bool NetherWartTile::canSurvive(Level* level, int x, int y, int z)
{
	return mayPlaceOn(level->getTile(x, y - 1, z));
}

void NetherWartTile::tick(Level* level, int x, int y, int z, Random* random)
{
	int age = level->getData(x, y, z);
	if (age < MAX_AGE && random->nextInt(10) == 0)
		level->setData(x, y, z, age + 1);

	super::tick(level, x, y, z, random);
}

void NetherWartTile::growCropsToMax(Level* level, int x, int y, int z)
{
	level->setData(x, y, z, MAX_AGE);
}

// Three pictures on the terrain atlas, one after another: young, half grown, ripe.
int NetherWartTile::getTexture(int face, int data)
{
	if (data >= MAX_AGE) return tex + 2;
	if (data > 0)        return tex + 1;
	return tex;
}

int NetherWartTile::getRenderShape()
{
	return Tile::SHAPE_ROWS;
}

int NetherWartTile::getRenderLayer()
{
	return Tile::RENDERLAYER_ALPHATEST;
}

// A young plant gives its seed back; a ripe one gives two to four.
void NetherWartTile::spawnResources(Level* level, int x, int y, int z, int data, float odds)
{
	if (level->isClientSide)
		return;

	int count = 1;
	if (data >= MAX_AGE)
		count = 2 + level->random.nextInt(3);

	for (int i = 0; i < count; i++) {
		float s = 0.7f;
		float xo = level->random.nextFloat() * s + (1 - s) * 0.5f;
		float yo = level->random.nextFloat() * s + (1 - s) * 0.5f;
		float zo = level->random.nextFloat() * s + (1 - s) * 0.5f;
		ItemEntity* item = new ItemEntity(level, float(x) + xo, float(y) + yo, float(z) + zo, ItemInstance(Item::netherStalkSeeds));
		item->throwTime = 10;
		level->addEntity(item);
	}
}

int NetherWartTile::getResource(int data, Random* random)
{
	return -1;
}

int NetherWartTile::getResourceCount(Random* random)
{
	return 0;
}
