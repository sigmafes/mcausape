#include "BlackSquid.h"
#include "../EntityTypes.h"
#include "../player/Player.h"
#include "../../effect/MobEffect.h"
#include "../../level/Level.h"
#include "../../../SharedConstants.h"
#include "../../../util/Mth.h"
#include <math.h>

BlackSquid::BlackSquid(Level* level)
:	super(level),
	fleeTicks(0),
	unstickTicks(0),
	progressTicks(0),
	stalled(false),
	lastX(0), lastY(0), lastZ(0)
{
	textureName = "mob/black_squid.png";
}

int BlackSquid::getEntityTypeId() const { return MobTypes::BlackSquid; }

bool BlackSquid::canSpawn() { return level->isUnobstructed(bb); }

void BlackSquid::updateAi()
{
	if (fleeTicks > 0) --fleeTicks;
	// Against a wall (or the floor or ceiling) it used to push on into it:
	// it turns off a random way for a second instead, rising a little.
	if (unstickTicks > 0) { --unstickTicks; noActionTime = 0; return; }
	// Stuck: against something, or hardly moved in a second.
	if (++progressTicks >= SharedConstants::TicksPerSecond) {
		const float mx = x - lastX, my = y - lastY, mz = z - lastZ;
		stalled = mx * mx + my * my + mz * mz < 0.25f;
		lastX = x; lastY = y; lastZ = z;
		progressTicks = 0;
	}
	if (horizontalCollision || verticalCollision || stalled) {
		stalled = false;
		const float angle = random.nextFloat() * Mth::PI * 2.0f;
		tx = Mth::cos(angle) * 0.5f;
		ty = 0.25f + random.nextFloat() * 0.2f;
		tz = Mth::sin(angle) * 0.5f;
		unstickTicks = SharedConstants::TicksPerSecond;
		return;
	}
	// The nearest player within 16 blocks, not in creative.
	Player* prey = NULL;
	float best = 48 * 48;	// far: the deep is meant to be dangerous
	for (unsigned int i = 0; i < level->players.size(); ++i) {
		Player* p = level->players[i];
		if (!p->isAlive() || p->abilities.instabuild) continue;
		const float d = p->distanceToSqr(this);
		if (d < best) { best = d; prey = p; }
	}
	if (!prey) { super::updateAi(); return; }

	float dx = prey->x - x, dy = (prey->y - prey->heightOffset + 0.8f) - y, dz = prey->z - z;
	const float d = sqrtf(dx * dx + dy * dy + dz * dz);
	if (d < 0.01f) return;
	// Towards them -- or, while its ink lasts, away.
	const float sign = fleeTicks > 0 ? -1.0f : 1.0f;
	// A hard dash: twice the plain squid's push.
	tx = sign * dx / d * 0.6f;
	ty = sign * dy / d * 0.6f;
	tz = sign * dz / d * 0.6f;
	noActionTime = 0;
}

void BlackSquid::aiStep()
{
	super::aiStep();
	if (level->isClientSide || !isAlive() || fleeTicks > 0) return;
	for (unsigned int i = 0; i < level->players.size(); ++i) {
		Player* p = level->players[i];
		if (!p->isAlive() || p->abilities.instabuild || !p->bb.intersects(bb.grow(0.2f, 0.2f, 0.2f))) continue;
		p->addEffect(MobEffects::Ink, MobEffects::InkTicks);
		fleeTicks = MobEffects::InkTicks;
		break;
	}
}
