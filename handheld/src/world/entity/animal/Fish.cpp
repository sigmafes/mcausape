#include "Fish.h"
#include "../EntityTypes.h"
#include "../EntityRendererId.h"
#include "../../level/Level.h"
#include "../../level/material/Material.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../../item/DyePowderItem.h"
#include "../../../SharedConstants.h"
#include "../../../util/Mth.h"
#include <math.h>

Fish::Fish(Level* level, bool salmon)
:	super(level),
	salmon(salmon),
	tx(0), ty(0), tz(0),
	hasTarget(false),
	dryTicks(0),
	panicTicks(0)
{
	entityRendererId = salmon ? ER_SALMON_RENDERER : ER_COD_RENDERER;
	textureName = salmon ? "mob/salmon.png" : "mob/cod.png";
	if (salmon) setSize(0.7f, 0.4f);
	else setSize(0.5f, 0.3f);
}

int Fish::getMaxHealth() { return 3; }

int Fish::getEntityTypeId() const { return salmon ? MobTypes::Salmon : MobTypes::Cod; }

int Fish::getMaxSpawnClusterSize() { return salmon ? 5 : 8; }

int Fish::getDeathLoot() { return salmon ? Item::rawSalmon->id : Item::rawCod->id; }

void Fish::dropDeathLoot()
{
	spawnAtLocation(getDeathLoot(), 1);
	// Java: bone meal one time in twenty.
	if (random.nextInt(20) == 0)
		spawnAtLocation(new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 0);
}

bool Fish::waterAt(float x, float y, float z)
{
	const int xi = Mth::floor(x), yi = Mth::floor(y), zi = Mth::floor(z);
	return level->getMaterial(xi, yi, zi) == Material::water || level->allWaterAt(xi, yi, zi);
}

bool Fish::isInWater()
{
	return level->checkAndHandleWater(bb, Material::water, this);
}

bool Fish::canSpawn()
{
	// The Aquarium is all water; the Overworld's fish keep below sea level.
	if (level->dimension && level->dimension->isAllWater()) return super::canSpawn();
	return y < 63 && super::canSpawn();
}

void Fish::aiStep()
{
	super::aiStep();
	if (level->isClientSide) return;

	if (isInWater()) {
		dryTicks = 0;
		// Swim towards the target, turning to face the way it goes.
		const float speed = (panicTicks > 0 ? 0.12f : 0.05f) * (salmon ? 1.1f : 1.0f) * swimScale();
		if (hasTarget) {
			float dx = tx - x, dy = ty - y, dz = tz - z;
			const float d = sqrtf(dx * dx + dy * dy + dz * dz);
			if (d < 1.0f) {
				hasTarget = false;
			} else {
				xd += (dx / d * speed - xd) * 0.15f;
				yd += (dy / d * speed - yd) * 0.15f;
				zd += (dz / d * speed - zd) * 0.15f;
			}
		}
		if (xd * xd + zd * zd > 0.0001f) {
			const float want = -atan2f(xd, zd) * 180 / Mth::PI;
			float diff = want - yBodyRot;
			while (diff < -180) diff += 360;
			while (diff >= 180) diff -= 360;
			yBodyRot += diff * 0.3f;
			yRot = yBodyRot;
		}
	} else {
		dryStep();
	}
	if (panicTicks > 0) --panicTicks;
}

void Fish::dryStep()
{
	{
		// Out of water: flops about on the ground, and runs out of breath
		// after a few seconds (Java's 300 ticks of air, then a heart a second).
		if (onGround && random.nextInt(4) == 0) {
			yd = 0.4f;
			xd = (random.nextFloat() * 2 - 1) * 0.05f;
			zd = (random.nextFloat() * 2 - 1) * 0.05f;
			yRot = yBodyRot = random.nextFloat() * 360;
		}
		if (++dryTicks > SharedConstants::TicksPerSecond * 15 && dryTicks % SharedConstants::TicksPerSecond == 0)
			hurt(NULL, 2);
	}
}

void Fish::travel(float xa, float ya)
{
	if (isInWater()) {
		move(xd, yd, zd);
		xd *= 0.9f;
		yd *= 0.9f;
		zd *= 0.9f;
		if (!hasTarget) yd -= 0.005f;
	} else {
		yd -= 0.08f;
		move(xd, yd, zd);
		yd *= 0.98f;
		const float friction = onGround ? 0.6f : 0.91f;
		xd *= friction;
		zd *= friction;
	}
}

void Fish::updateAi()
{
	// Hurt: off somewhere else, fast, for a few seconds.
	if (hurtTime > 0 && panicTicks <= 0) {
		panicTicks = SharedConstants::TicksPerSecond * 3;
		hasTarget = false;
	}
	// Java's RandomSwimmingGoal: a new point within 10 across and 7 up or
	// down, now and then (often while panicking), if it is water.
	if (isInWater() && (!hasTarget || random.nextInt(panicTicks > 0 ? 10 : 60) == 0)) {
		for (int i = 0; i < 6; ++i) {
			const float nx = x + random.nextInt(21) - 10;
			const float ny = y + random.nextInt(15) - 7;
			const float nz = z + random.nextInt(21) - 10;
			if (waterAt(nx, ny, nz)) {
				tx = nx; ty = ny; tz = nz;
				hasTarget = true;
				break;
			}
		}
	}
	checkDespawn();
}
