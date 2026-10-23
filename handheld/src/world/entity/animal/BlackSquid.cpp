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
	unstickTicks(0)
{
	textureName = "mob/black_squid.png";
}

int BlackSquid::getEntityTypeId() const { return MobTypes::BlackSquid; }

void BlackSquid::updateAi()
{
	if (fleeTicks > 0) --fleeTicks;
	// Against a wall (or the floor or ceiling) it used to push on into it:
	// it turns off a random way for a second instead, rising a little.
	if (unstickTicks > 0) { --unstickTicks; noActionTime = 0; return; }
	if (horizontalCollision || (verticalCollision && !onGround)) {
		const float angle = random.nextFloat() * Mth::PI * 2.0f;
		tx = Mth::cos(angle) * 0.3f;
		ty = 0.1f + random.nextFloat() * 0.15f;
		tz = Mth::sin(angle) * 0.3f;
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
	tx = sign * dx / d * 0.3f;
	ty = sign * dy / d * 0.3f;
	tz = sign * dz / d * 0.3f;
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
