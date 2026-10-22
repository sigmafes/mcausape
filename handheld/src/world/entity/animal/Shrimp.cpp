#include "Shrimp.h"
#include "../EntityTypes.h"
#include "../EntityRendererId.h"
#include "../../item/Item.h"

Shrimp::Shrimp(Level* level)
:	super(level, false)
{
	entityRendererId = ER_SHRIMP_RENDERER;
	textureName = "mob/shrimp.png";
	setSize(0.4f, 0.3f);
}

int Shrimp::getEntityTypeId() const { return MobTypes::Shrimp; }

int Shrimp::getDeathLoot() { return Item::shrimp->id; }

void Shrimp::dropDeathLoot()
{
	spawnAtLocation(Item::shrimp->id, 1);
}
