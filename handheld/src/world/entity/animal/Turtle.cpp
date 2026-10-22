#include "Turtle.h"
#include "../EntityTypes.h"
#include "../EntityRendererId.h"
#include "../player/Player.h"
#include "../../level/Level.h"
#include "../../item/Item.h"
#include "../../../SharedConstants.h"

Turtle::Turtle(Level* level)
:	super(level, false),
	angryAt(0),
	angryTicks(0),
	biteCooldown(0)
{
	entityRendererId = ER_TURTLE_RENDERER;
	textureName = "mob/turtle.png";
	setSize(1.2f, 0.4f);
}

int Turtle::getEntityTypeId() const { return MobTypes::Turtle; }

int Turtle::getMaxHealth() { return 30; }

int Turtle::getDeathLoot() { return Item::turtleScute->id; }

void Turtle::dropDeathLoot()
{
	spawnAtLocation(Item::turtleScute->id, 1 + random.nextInt(2));
}

bool Turtle::hurt(Entity* source, int dmg)
{
	const bool hit = super::hurt(source, dmg);
	if (hit && source && source->isPlayer() && !((Player*) source)->abilities.instabuild) {
		angryAt = source->entityId;
		angryTicks = SharedConstants::TicksPerSecond * 30;
	}
	return hit;
}

void Turtle::aiStep()
{
	super::aiStep();
	if (level->isClientSide || !isAlive()) return;
	if (biteCooldown > 0) --biteCooldown;
	if (angryAt == 0) return;

	Entity* target = level->getEntity(angryAt);
	if (!target || !target->isAlive() || --angryTicks <= 0 || target->distanceToSqr(this) > 24 * 24) {
		angryAt = 0;
		return;
	}
	// After it, and a bite when close.
	tx = target->x; ty = target->y - 0.5f; tz = target->z;
	hasTarget = true;
	if (biteCooldown == 0 && target->distanceToSqr(this) < 2.0f * 2.0f) {
		target->hurt(this, 3);
		biteCooldown = SharedConstants::TicksPerSecond;
	}
}
