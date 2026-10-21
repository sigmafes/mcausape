#include "Ghast.h"
#include "../EntityTypes.h"
#include "../player/Player.h"
#include "../projectile/Fireball.h"
#include "../../Difficulty.h"
#include "../../item/Item.h"
#include "../../level/Level.h"
#include "../../../util/Mth.h"

Ghast::Ghast(Level* level)
:	super(level),
	charge(0),
	xTarget(0), yTarget(0), zTarget(0),
	floatDuration(0)
{
	entityRendererId = ER_GHAST_RENDERER;
	this->textureName = "mob/ghast.png";
	setSize(4, 4);
}

int Ghast::getMaxHealth() {
	return 10;
}

int Ghast::getEntityTypeId() const {
	return MobTypes::Ghast;
}

int Ghast::getCreatureBaseType() const {
	return MobTypes::BaseEnemy;
}

bool Ghast::canSpawn() {
	return random.nextInt(20) == 0 && super::canSpawn() && level->difficulty > Difficulty::PEACEFUL;
}

const char* Ghast::getAmbientSound() { return "mob.ghast.moan"; }
std::string Ghast::getHurtSound() { return "mob.ghast.scream"; }
std::string Ghast::getDeathSound() { return "mob.ghast.death"; }

int Ghast::getDeathLoot() {
	return Item::sulphur->id;
}

void Ghast::dropDeathLoot() {
	int count = random.nextInt(3);
	for (int i = 0; i < count; i++)
		spawnAtLocation(Item::sulphur->id, 1);
}

// Steers by itself, not through the walking AI (a ghast has no path to find): it
// picks a spot within 16 blocks, accelerates towards it, and picks another on
// arrival or after a bump. Shots follow the charge of the Console Edition.
void Ghast::aiStep() {
	// On a client the position comes from the server (see Mob::tick).
	if (level->isClientSide)
		return;

	if (level->difficulty == Difficulty::PEACEFUL) {
		remove();
		return;
	}

	float dx = xTarget - x;
	float dy = yTarget - y;
	float dz = zTarget - z;
	float dd = dx * dx + dy * dy + dz * dz;

	if (dd < 1 || dd > 60 * 60) {
		xTarget = x + (random.nextFloat() * 2 - 1) * 16;
		yTarget = y + (random.nextFloat() * 2 - 1) * 16;
		zTarget = z + (random.nextFloat() * 2 - 1) * 16;
	}

	if (floatDuration-- <= 0) {
		floatDuration += random.nextInt(5) + 2;
		dd = Mth::sqrt(dd);
		if (dd > 0.001f) {
			xd += dx / dd * 0.1f;
			yd += dy / dd * 0.1f;
			zd += dz / dd * 0.1f;
		}
	}

	Player* target = level->getNearestPlayer(this, 64);
	if (target != NULL) {
		float xdd = target->x - x;
		float ydd = (target->bb.y0 + target->bbHeight / 2) - (y + bbHeight / 2);
		float zdd = target->z - z;
		yBodyRot = yRot = -(float) Mth::atan2(xdd, zdd) * Mth::RADDEG;

		if (canSee(target)) {
			charge++;
			// The wind-up is heard, as in the Console Edition: half way, then the shot.
			if (charge == 10)
				level->playSound(this, "mob.ghast.charge", getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
			if (charge == 20) {
				level->playSound(this, "mob.ghast.fireball", getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
				Vec3 v = getViewVector(1);
				float d = 4;
				Fireball* fb = new Fireball(level, this, x + v.x * d, y + bbHeight / 2 + 0.5f, z + v.z * d, false);
				fb->shoot(xdd, ydd, zdd, 0.9f, 8);
				level->addEntity(fb);
				charge = -40;
			}
		} else if (charge > 0) {
			charge--;
		}
	} else {
		yBodyRot = yRot = -(float) Mth::atan2(xd, zd) * Mth::RADDEG;
		if (charge > 0) charge--;
	}

	// The eyes shut while it charges.
	this->textureName = charge > 10? "mob/ghast_fire.png" : "mob/ghast.png";

	float lx = x, ly = y, lz = z;
	move(xd, yd, zd);
	xd *= 0.91f;
	yd *= 0.91f;
	zd *= 0.91f;
	// A bump into something: look for another spot to drift to.
	if (horizontalCollision || (x == lx && y == ly && z == lz && (xd != 0 || zd != 0))) {
		xTarget = x;
		yTarget = y;
		zTarget = z;
	}
}
