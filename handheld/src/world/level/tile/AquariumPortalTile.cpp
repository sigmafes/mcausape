#include "AquariumPortalTile.h"
#include "../Level.h"
#include "../dimension/Dimension.h"
#include "../../entity/Entity.h"
#include "../../entity/player/Player.h"
#include "../../../util/Random.h"

bool AquariumPortalTile::isFrame(Level* level, int x, int y, int z)
{
	const int t = level->getTile(x, y, z);
	return (Tile::oceanCrystalBlock && t == Tile::oceanCrystalBlock->id)
		|| (Tile::oceanCrystalHub && t == Tile::oceanCrystalHub->id);
}

void AquariumPortalTile::cornerOf(Level* level, int x, int y, int z, int* cx, int* cz)
{
	const int id = Tile::aquariumPortal->id;
	*cx = (level->getTile(x - 1, y, z) == id) ? x - 1 : x;
	*cz = (level->getTile(x, y, z - 1) == id) ? z - 1 : z;
}

bool AquariumPortalTile::frameWhole(Level* level, int cx, int y, int cz)
{
	// Two along each side of the 2x2; the corners of the 4x4 are not part of it.
	for (int i = 0; i < 2; ++i) {
		if (!isFrame(level, cx + i, y, cz - 1)) return false;
		if (!isFrame(level, cx + i, y, cz + 2)) return false;
		if (!isFrame(level, cx - 1, y, cz + i)) return false;
		if (!isFrame(level, cx + 2, y, cz + i)) return false;
	}
	return true;
}

static bool openForPortal(Level* level, int x, int y, int z)
{
	const int t = level->getTile(x, y, z);
	return t == 0 || level->getMaterial(x, y, z) == Material::water;
}

bool AquariumPortalTile::tryLight(Level* level, int x, int y, int z)
{
	// Any of the four cells will do: try each as the one the water went in.
	for (int dx = 0; dx < 2; ++dx) {
		for (int dz = 0; dz < 2; ++dz) {
			const int cx = x - dx, cz = z - dz;
			bool open = true;
			for (int i = 0; i < 2 && open; ++i)
				for (int j = 0; j < 2 && open; ++j)
					open = openForPortal(level, cx + i, y, cz + j);
			if (!open || !frameWhole(level, cx, y, cz)) continue;
			for (int i = 0; i < 2; ++i)
				for (int j = 0; j < 2; ++j)
					level->setTileNoUpdate(cx + i, y, cz + j, Tile::aquariumPortal->id);
			for (int i = 0; i < 2; ++i)
				for (int j = 0; j < 2; ++j)
					level->sendTileUpdated(cx + i, y, cz + j);
			return true;
		}
	}
	return false;
}

void AquariumPortalTile::neighborChanged(Level* level, int x, int y, int z, int type)
{
	// A frame block gone (or a cell of the portal): the whole portal goes out.
	int cx, cz;
	cornerOf(level, x, y, z, &cx, &cz);
	bool whole = frameWhole(level, cx, y, cz);
	for (int i = 0; i < 2 && whole; ++i)
		for (int j = 0; j < 2 && whole; ++j)
			whole = level->getTile(cx + i, y, cz + j) == id;
	if (!whole) level->setTile(x, y, z, 0);
}

void AquariumPortalTile::entityInside(Level* level, int x, int y, int z, Entity* entity)
{
	// Only the host's own player travels this way; visitors go by their
	// packets (ServerSideNetworkHandler::checkVisitorPortal).
	if (level->isClientSide || !entity->isPlayer()) return;
	if (level->dimension->id == Dimension::AQUARIUM) {
		level->requestPortalTravel(entity, Dimension::NORMAL);
		return;
	}
	if (!Dimension::isOverworld(level->dimension->id)) return;

	// Coming back lands on this portal's frame, beside the portal rather
	// than in it.
	Player* player = (Player*) entity;
	int cx, cz;
	cornerOf(level, x, y, z, &cx, &cz);
	player->hasAquariumReturn = true;
	player->aquariumReturnX = cx - 1;
	player->aquariumReturnY = y + 1;
	player->aquariumReturnZ = cz;
	level->requestPortalTravel(entity, Dimension::AQUARIUM);
}

void AquariumPortalTile::animateTick(Level* level, int x, int y, int z, Random* random)
{
	// Bubbles rising off the sheet.
	if (random->nextInt(3) != 0) return;
	level->addParticle("bubble", x + random->nextFloat(), y + 0.8f, z + random->nextFloat(), 0, 0.1f, 0);
}
