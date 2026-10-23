#include "AquaticBlaze.h"
#include "Drowned.h"
#include "../EntityTypes.h"
#include "../player/Player.h"
#include "../projectile/WaterBolt.h"
#include "../../level/Level.h"
#include "../../../SharedConstants.h"
#include "../../../util/Mth.h"
#include <math.h>

AquaticBlaze::AquaticBlaze(Level* level)
:	super(level),
	shots(0)
{
	entityRendererId = ER_AQUATICBLAZE_RENDERER;
	textureName = "mob/aquatic_blaze.png";
}

int AquaticBlaze::getEntityTypeId() const { return MobTypes::AquaticBlaze; }

void AquaticBlaze::updateAi()
{
	super::updateAi();
	jumping = false;	// no lift every tick (see Drowned::updateAi)
}

void AquaticBlaze::aiStep()
{
	// Bubbles off it, as the blaze gives off smoke.
	for (int i = 0; i < 2; i++)
		level->addParticle("bubble",
			x + (random.nextFloat() - 0.5f) * bbWidth,
			y + random.nextFloat() * bbHeight,
			z + (random.nextFloat() - 0.5f) * bbWidth, 0, 0.05f, 0);
	Monster::aiStep();
	if (level->isClientSide) return;

	Player* p = Drowned::nearestPrey(this, 48);
	if (!p) { xd *= 0.8f; yd *= 0.8f; zd *= 0.8f; return; }

	// Level with the player, and across after them, about six blocks off.
	const float dy = (p->y - p->heightOffset + 0.6f) - y;
	yd += ((dy > 0.3f ? 0.1f : (dy < -0.3f ? -0.1f : 0.0f)) - yd) * 0.25f;
	const float dx = p->x - x, dz = p->z - z;
	const float d = sqrtf(dx * dx + dz * dz);
	if (d > 0.01f) {
		const float want = d > 7 ? 0.1f : (d < 4 ? -0.06f : 0.0f);
		xd += (dx / d * want - xd) * 0.25f;
		zd += (dz / d * want - zd) * 0.25f;
	}
	yRot = yBodyRot = -atan2f(dx, dz) * Mth::RADDEG;

	// The blaze's volleys: a wind-up, three shots close together, a rest.
	if (attackTime > 0 || d > 32) return;
	++shots;
	if (shots == 1) attackTime = SharedConstants::TicksPerSecond * 3;
	else if (shots <= 4) attackTime = SharedConstants::TicksPerSecond / 3;
	else { attackTime = SharedConstants::TicksPerSecond * 5; shots = 0; }
	if (shots > 1) {
		const float tx = p->x - x;
		const float ty = (p->bb.y0 + p->bbHeight / 2) - (y + bbHeight / 2);
		const float tz = p->z - z;
		const float spread = Mth::sqrt(d) * 0.2f;
		WaterBolt* bolt = new WaterBolt(level, this, x, y + bbHeight / 2 + 0.5f, z);
		bolt->shoot(tx + random.nextGaussian() * spread, ty, tz + random.nextGaussian() * spread, 0.9f, 1);
		level->addEntity(bolt);
	}
}

void AquaticBlaze::travel(float xa, float ya)
{
	// Its own swimming (aiStep's): no sinking, no strokes from the AI.
	move(xd, yd, zd);
	xd *= 0.91f;
	yd *= 0.91f;
	zd *= 0.91f;
}
