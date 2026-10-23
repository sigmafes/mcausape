#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__CoralPlantTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__CoralPlantTile_H__

#include "Bush.h"
#include "../../entity/player/Player.h"
#include "../../entity/player/Inventory.h"
#include "../../item/HatchetItem.h"

/// Coral, block or plant, is only got with an iron axe or better (coral's
/// included): anything else breaks it to nothing.
inline bool coralToolOk(Player* player) {
	ItemInstance* held = player ? player->inventory->getSelected() : NULL;
	if (!held || held->isNull()) return false;
	HatchetItem* axe = dynamic_cast<HatchetItem*>(held->getItem());
	return axe && axe->getTier().getLevel() >= Item::Tier::IRON.getLevel();
}

class CoralBlockTile: public Tile
{
public:
	CoralBlockTile(int id, int tex) : Tile(id, tex, Material::coral) {}
	void playerDestroy(Level* level, Player* player, int x, int y, int z, int data) {
		if (coralToolOk(player)) Tile::playerDestroy(level, player, x, y, z, data);
	}
};

/// Java's coral (the plant, not the block): an X of two crossed pictures,
/// not solid, standing on a coral block, prismarine or sand. The Aquarium's
/// "water" is air, so unlike Java it needs no water around it, nor light.
class CoralPlantTile: public Bush
{
public:
	CoralPlantTile(int id, int tex) : Bush(id, tex) {
		setTicking(false);
		const float ss = 0.375f;
		setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, 0.8f, 0.5f + ss);
	}

	void playerDestroy(Level* level, Player* player, int x, int y, int z, int data) {
		if (coralToolOk(player)) Bush::playerDestroy(level, player, x, y, z, data);
	}

	bool canSurvive(Level* level, int x, int y, int z) {
		return mayPlaceOn(level->getTile(x, y - 1, z));
	}

protected:
	bool mayPlaceOn(int tile) {
		for (int i = 0; i < 5; ++i)
			if (Tile::coral[i] && tile == Tile::coral[i]->id) return true;
		return tile == Tile::prismarine->id || tile == Tile::sand->id;
	}
};

#endif
