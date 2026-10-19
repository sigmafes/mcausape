#include "MagmaCube.h"
#include "../../Difficulty.h"
#include "../../level/Level.h"
#include "../EntityTypes.h"

MagmaCube::MagmaCube(Level* level)
:	super(level)
{
	entityRendererId = ER_MAGMACUBE_RENDERER;
	this->textureName = "mob/magmacube.png";
	setSize(1.2f, 1.2f);
}

int MagmaCube::getMaxHealth() {
	return 16;
}

int MagmaCube::getEntityTypeId() const {
	return MobTypes::MagmaCube;
}

bool MagmaCube::canSpawn() {
	return PathfinderMob::canSpawn() && level->difficulty > Difficulty::PEACEFUL;
}
