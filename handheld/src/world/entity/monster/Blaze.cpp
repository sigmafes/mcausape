#include "Blaze.h"
#include "../../Difficulty.h"
#include "../../level/Level.h"
#include "../EntityTypes.h"
#include "../projectile/Fireball.h"
#include "../../item/Item.h"
#include "../../level/Level.h"
#include "../../../SharedConstants.h"
#include "../../../util/Mth.h"

Blaze::Blaze(Level* level)
:	super(level),
	allowedHeightOffset(0.5f),
	nextHeightOffsetChangeTick(0),
	attackCounter(0)
{
	entityRendererId = ER_BLAZE_RENDERER;
	this->textureName = "mob/blaze.png";
	attackDamage = 6;
}

int Blaze::getMaxHealth() {
	return 20;
}

int Blaze::getEntityTypeId() const {
	return MobTypes::Blaze;
}

int Blaze::getDeathLoot() {
	return Item::yellowDust->id;
}

void Blaze::dropDeathLoot() {
	int count = random.nextInt(3);
	for (int i = 0; i < count; i++)
		spawnAtLocation(Item::yellowDust->id, 1);
}

void Blaze::aiStep() {
	if (!level->isClientSide) {
		// Every five seconds it picks a new height to hover at, relative to its target.
		if (--nextHeightOffsetChangeTick <= 0) {
			nextHeightOffsetChangeTick = SharedConstants::TicksPerSecond * 5;
			allowedHeightOffset = 0.5f + random.nextGaussian() * 3;
		}

		Mob* target = getTarget();
		if (target != NULL && (target->y + target->getHeadHeight()) > (y + getHeadHeight() + allowedHeightOffset)) {
			yd = yd + (0.3f - yd) * 0.3f;
		}
	}

	// slow falling, like a chicken
	if (!onGround && yd < 0)
		yd *= 0.6f;

	for (int i = 0; i < 2; i++) {
		level->addParticle(PARTICLETYPE(largesmoke),
			x + (random.nextFloat() - 0.5f) * bbWidth,
			y + random.nextFloat() * bbHeight,
			z + (random.nextFloat() - 0.5f) * bbWidth, 0, 0, 0);
	}

	super::aiStep();
}

void Blaze::checkHurtTarget(Entity* target, float d) {
	if (attackTime <= 0 && d < 2.0f && target->bb.y1 > bb.y0 && target->bb.y0 < bb.y1) {
		attackTime = 20;
		doHurtTarget(target);
	} else if (d < 30) {
		float xdd = target->x - x;
		float ydd = (target->bb.y0 + target->bbHeight / 2) - (y + bbHeight / 2);
		float zdd = target->z - z;

		if (attackTime == 0) {
			attackCounter++;
			if (attackCounter == 1) {
				attackTime = SharedConstants::TicksPerSecond * 3;           // the charge
			} else if (attackCounter <= 4) {
				attackTime = SharedConstants::TicksPerSecond / 3;           // three shots, close together
			} else {
				attackTime = SharedConstants::TicksPerSecond * 5;           // then a rest
				attackCounter = 0;
			}

			if (attackCounter > 1) {
				float sqd = Mth::sqrt(d) * 0.5f;
				Fireball* fb = new Fireball(level, this, x, y + bbHeight / 2 + 0.5f, z, true);
				fb->shoot(xdd + random.nextGaussian() * sqd, ydd, zdd + random.nextGaussian() * sqd, 1.0f, 1);
				level->addEntity(fb);
			}
		}
		yRot = (float) (Mth::atan2(zdd, xdd) * Mth::RADDEG) - 90;

		holdGround = true;
	}
}

bool Blaze::canSpawn() {
	return PathfinderMob::canSpawn() && level->difficulty > Difficulty::PEACEFUL;
}
