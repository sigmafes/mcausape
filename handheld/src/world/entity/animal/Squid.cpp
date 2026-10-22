#include "Squid.h"
#include "../EntityTypes.h"
#include "../EntityRendererId.h"
#include "../../level/Level.h"
#include "../../level/material/Material.h"
#include "../../item/Item.h"
#include "../../../SharedConstants.h"
#include "../../../util/Mth.h"
#include <math.h>

Squid::Squid(Level* level)
:	super(level),
	xBodyRot(0), xBodyRotO(0),
	zBodyRot(0), zBodyRotO(0),
	tentacleMovement(0), oldTentacleMovement(0),
	tentacleAngle(0), oldTentacleAngle(0),
	speed(0), rotateSpeed(0),
	tx(0), ty(0), tz(0),
	dryTicks(0)
{
	entityRendererId = ER_SQUID_RENDERER;
	textureName = "mob/squid.png";
	setSize(0.95f, 0.95f);
	tentacleSpeed = 1 / (random.nextFloat() + 1) * 0.2f;
}

int Squid::getMaxHealth() { return 10; }

int Squid::getEntityTypeId() const { return MobTypes::Squid; }

int Squid::getDeathLoot() { return Item::oceanShard->id; }

void Squid::dropDeathLoot()
{
	// 0 to 3 shards (the Console Edition's dropped ink sacs here).
	const int count = random.nextInt(4);
	for (int i = 0; i < count; ++i)
		spawnAtLocation(Item::oceanShard->id, 1);
}

bool Squid::isInWater()
{
	// Its middle in water, not its edge: the bottom of a lake counts, a puddle does not.
	return level->checkAndHandleWater(bb.grow(0, -0.6f, 0), Material::water, this);
}

void Squid::aiStep()
{
	super::aiStep();

	xBodyRotO = xBodyRot;
	zBodyRotO = zBodyRot;
	oldTentacleMovement = tentacleMovement;
	oldTentacleAngle = tentacleAngle;

	tentacleMovement += tentacleSpeed;
	if (tentacleMovement > Mth::PI * 2.0f) {
		tentacleMovement -= Mth::PI * 2.0f;
		if (random.nextInt(10) == 0) tentacleSpeed = 1 / (random.nextFloat() + 1) * 0.2f;
	}

	if (isInWater()) {
		dryTicks = 0;
		if (tentacleMovement < Mth::PI) {
			const float tentacleScale = tentacleMovement / Mth::PI;
			tentacleAngle = Mth::sin(tentacleScale * tentacleScale * Mth::PI) * Mth::PI * 0.25f;
			if (tentacleScale > 0.75f) {
				speed = 1.0f;
				rotateSpeed = 1.0f;
			} else {
				rotateSpeed = rotateSpeed * 0.8f;
			}
		} else {
			tentacleAngle = 0.0f;
			speed = speed * 0.9f;
			rotateSpeed = rotateSpeed * 0.99f;
		}

		if (!level->isClientSide) {
			xd = tx * speed;
			yd = ty * speed;
			zd = tz * speed;
		}

		const float horizontalMovement = sqrtf(xd * xd + zd * zd);
		yBodyRot += ((-atan2f(xd, zd) * 180 / Mth::PI) - yBodyRot) * 0.1f;
		yRot = yBodyRot;
		zBodyRot = zBodyRot + Mth::PI * rotateSpeed * 1.5f;
		xBodyRot += ((-atan2f(horizontalMovement, yd) * 180 / Mth::PI) - xBodyRot) * 0.1f;
	} else {
		tentacleAngle = Mth::abs(Mth::sin(tentacleMovement)) * Mth::PI * 0.25f;
		if (!level->isClientSide) {
			// Unable to move: it falls, and lies there.
			xd = 0.0f;
			yd -= 0.08f;
			yd *= 0.98f;
			zd = 0.0f;
			// Out of water it suffocates, as in Java: a second of breath, then
			// a heart a second.
			if (++dryTicks > SharedConstants::TicksPerSecond && dryTicks % SharedConstants::TicksPerSecond == 0)
				hurt(NULL, 2);
		}
		// Falls over.
		xBodyRot += (-90 - xBodyRot) * 0.02f;
	}
}

void Squid::travel(float xa, float ya)
{
	move(xd, yd, zd);
}

void Squid::updateAi()
{
	noActionTime++;

	// The Console Edition's "ridiculous simple movement ai": a new direction
	// now and then, and standing still after five seconds of nothing.
	if (noActionTime > SharedConstants::TicksPerSecond * 5) {
		tx = ty = tz = 0;
	} else if (random.nextInt(50) == 0 || !wasInWater || (tx == 0 && ty == 0 && tz == 0)) {
		const float angle = random.nextFloat() * Mth::PI * 2.0f;
		tx = Mth::cos(angle) * 0.2f;
		ty = -0.1f + random.nextFloat() * 0.2f;
		tz = Mth::sin(angle) * 0.2f;
	}
	checkDespawn();
}

bool Squid::canSpawn()
{
	// Below sea level and above the caves, as in Java.
	return y > 45 && y < 63 && super::canSpawn();
}
