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
	// The blaze's hovering after its target, without the smoke.
	if (!level->isClientSide) {
		Mob* target = getTarget();
		if (target) {
			const float dy = (target->y + target->getHeadHeight()) - (y + getHeadHeight() + 0.5f);
			yd += ((dy > 0 ? 0.12f : -0.06f) - yd) * 0.2f;
		}
	}
	Monster::aiStep();
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
