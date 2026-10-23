#include "AquaticBlaze.h"
#include "../EntityTypes.h"
#include "../projectile/WaterBolt.h"
#include "../../level/Level.h"
#include "../../../SharedConstants.h"
#include "../../../util/Mth.h"

AquaticBlaze::AquaticBlaze(Level* level)
:	super(level),
	shots(0),
	shotCooldown(0)
{
	entityRendererId = ER_AQUATICBLAZE_RENDERER;
	textureName = "mob/aquatic_blaze.png";
}

int AquaticBlaze::getEntityTypeId() const { return MobTypes::AquaticBlaze; }

void AquaticBlaze::aiStep()
{
	// Bubbles off it, as the blaze gives off smoke.
	for (int i = 0; i < 2; i++)
		level->addParticle("bubble",
			x + (random.nextFloat() - 0.5f) * bbWidth,
			y + random.nextFloat() * bbHeight,
			z + (random.nextFloat() - 0.5f) * bbWidth, 0, 0.05f, 0);
	Monster::aiStep();
	jumping = false;
	if (level->isClientSide) return;
	// It keeps level with its target and follows it across, staying about
	// six blocks off to shoot; with no target it hangs where it is.
	Mob* target = getTarget();
	if (!target) { xd *= 0.8f; yd *= 0.8f; zd *= 0.8f; return; }
	const float dy = (target->y - target->heightOffset + 0.6f) - y;
	yd += ((dy > 0.3f ? 0.08f : (dy < -0.3f ? -0.08f : 0.0f)) - yd) * 0.2f;
	const float dx = target->x - x, dz = target->z - z;
	const float d = Mth::sqrt(dx * dx + dz * dz);
	if (d > 0.01f) {
		const float want = d > 7 ? 0.08f : (d < 4 ? -0.05f : 0.0f);
		xd += (dx / d * want - xd) * 0.2f;
		zd += (dz / d * want - zd) * 0.2f;
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

void AquaticBlaze::checkHurtTarget(Entity* target, float d)
{
	if (attackTime <= 0 && d < 2.0f && target->bb.y1 > bb.y0 && target->bb.y0 < bb.y1) {
		attackTime = 20;
		doHurtTarget(target);
		return;
	}
	if (d >= 30) return;
	const float xdd = target->x - x;
	const float ydd = (target->bb.y0 + target->bbHeight / 2) - (y + bbHeight / 2);
	const float zdd = target->z - z;
	if (attackTime == 0) {
		// As the blaze: a wind-up, three shots close together, a rest.
		++shots;
		if (shots == 1) attackTime = SharedConstants::TicksPerSecond * 3;
		else if (shots <= 4) attackTime = SharedConstants::TicksPerSecond / 3;
		else { attackTime = SharedConstants::TicksPerSecond * 5; shots = 0; }
		if (shots > 1) {
			const float spread = Mth::sqrt(d) * 0.3f;
			WaterBolt* bolt = new WaterBolt(level, this, x, y + bbHeight / 2 + 0.5f, z);
			bolt->shoot(xdd + random.nextGaussian() * spread, ydd, zdd + random.nextGaussian() * spread, 0.9f, 1);
			level->addEntity(bolt);
		}
	}
	yRot = (float) (Mth::atan2(zdd, xdd) * Mth::RADDEG) - 90;
	holdGround = true;
}
