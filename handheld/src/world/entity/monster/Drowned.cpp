#include "Drowned.h"
#include "../EntityTypes.h"
#include "../../level/Level.h"
#include "../../level/material/Material.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"

Drowned::Drowned(Level* level)
:	super(level)
{
	textureName = "mob/drowned.png";
}

int Drowned::getEntityTypeId() const { return MobTypes::Drowned; }

bool Drowned::hasTrident() const
{
	// 15%, the same answer on every machine.
	return ((unsigned int) entityId * 2654435761u >> 16) % 100 < 15;
}

ItemInstance* Drowned::getCarriedItem()
{
	static ItemInstance trident;
	if (!hasTrident()) return NULL;
	trident = ItemInstance(Item::trident);
	return &trident;
}

void Drowned::aiStep()
{
	super::aiStep();
	// The monsters' AI "jumps" in water to keep afloat; with no gravity here
	// that shot it up to the ceiling in seconds, out of sight.
	jumping = false;
	if (level->isClientSide || !isInWater()) return;
	// Up or down to its target's height, as a swimming player steers.
	Mob* target = getTarget();
	if (!target) { yd *= 0.8f; return; }
	const float dy = (target->y - target->heightOffset + 0.5f) - (y - heightOffset);
	const float want = dy > 0.5f ? 0.08f : (dy < -0.5f ? -0.08f : 0.0f);
	yd += (want - yd) * 0.2f;
}

void Drowned::travel(float xa, float ya)
{
	if (!isInWater()) { super::travel(xa, ya); return; }
	// The player's swim: a good stroke, little drag, no sinking.
	moveRelative(xa, ya, 0.035f);
	move(xd, yd, zd);
	xd *= 0.9f;
	yd *= 0.9f;
	zd *= 0.9f;
}
