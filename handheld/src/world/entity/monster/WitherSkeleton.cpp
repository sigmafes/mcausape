#include "WitherSkeleton.h"
#include "../../Difficulty.h"
#include "../../level/Level.h"
#include "../../item/Item.h"
#include "../../../util/Random.h"

WitherSkeleton::WitherSkeleton(Level* level)
:	super(level),
	sword(Item::sword_stone, 1)
{
	this->textureName = "mob/wither_skeleton.png";
}

int WitherSkeleton::getMaxHealth() {
	return 20;
}

// Skips Skeleton::aiStep, which sets the mob alight in daylight.
void WitherSkeleton::aiStep() {
	Monster::aiStep();
}

int WitherSkeleton::getDeathLoot() {
	return Item::coal->id;
}

void WitherSkeleton::dropDeathLoot() {
	int count = random.nextInt(2);
	for (int i = 0; i < count; i++)
		spawnAtLocation(Item::coal->id, 1);
}

// A stone sword in place of the normal skeleton's bow.
ItemInstance* WitherSkeleton::getCarriedItem() {
	return &sword;
}

int WitherSkeleton::getEntityTypeId() const {
	return MobTypes::WitherSkeleton;
}

// Melee: swing when the target is in arm's reach, instead of drawing a bow.
void WitherSkeleton::checkHurtTarget(Entity* target, float d) {
	if (d < 2.5f && attackTime == 0) {
		doHurtTarget(target);
		attackTime = 20;
	}
	holdGround = false;
}

bool WitherSkeleton::canSpawn() {
	return PathfinderMob::canSpawn() && level->difficulty > Difficulty::PEACEFUL;
}
