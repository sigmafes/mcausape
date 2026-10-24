#include "MagmaCube.h"
#include "../../Difficulty.h"
#include "../../level/Level.h"
#include "../EntityTypes.h"
#include "../player/Player.h"
#include "../../item/Item.h"

MagmaCube::MagmaCube(Level* level)
:	super(level),
	squish(0),
	oSquish(0),
	targetSquish(0),
	jumpDelay(0),
	touchCooldown(0)
{
	jumpDelay = random.nextInt(20) + 10;
	walkingSpeed = 0.2f;
	attackDamage = SIZE + 2;
	entityRendererId = ER_MAGMACUBE_RENDERER;
	this->textureName = "mob/magmacube.png";
	setSize(1.2f, 1.2f);
}

int MagmaCube::getMaxHealth() {
	return 16;
}

int MagmaCube::getEntityTypeId() const {
	return MobTypes::MagmaCube;
}

bool MagmaCube::canSpawn() {
	return PathfinderMob::canSpawn() && level->difficulty > Difficulty::PEACEFUL;
}

void MagmaCube::tick() {
	squish += (targetSquish - squish) * 0.5f;
	oSquish = squish;
	const bool wasOnGround = onGround;
	super::tick();

	if (onGround && !wasOnGround) {
		targetSquish = -0.5f;		// lands: flattens
		level->playSound(this, "mob.magmacube.big", 0.4f * SIZE, ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) / 0.8f);
	}
	else if (!onGround && wasOnGround)
		targetSquish = 1;			// leaves the ground: stretches
	targetSquish *= 0.90f;

	if (touchCooldown > 0) --touchCooldown;
}

/** Slime::serverAiStep: wait on the ground, then hop. Overrides the whole of
    Monster's walking AI, which paths to its target on foot. */
void MagmaCube::updateAi() {
	noActionTime++;
	Player* nearest = level->getNearestPlayer(this, -1);
	checkDespawn(nearest);

	Player* player = level->getNearestAttackablePlayer(this, 16);
	if (player != NULL)
		lookAt(player, 10, 20);

	if (onGround && jumpDelay-- <= 0) {
		jumpDelay = (random.nextInt(20) + 10) * 4;
		if (player != NULL)
			jumpDelay /= 3;
		jumping = true;
		level->playSound(this, "mob.magmacube.jump", 0.4f * SIZE, ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) * 0.8f);
		xxa = 1 - random.nextFloat() * 2;
		yya = (float) SIZE;
	} else {
		jumping = false;
		if (onGround)
			xxa = yya = 0;
	}

	// It hurts by touching, not by swinging.
	if (player != NULL && touchCooldown <= 0 && canSee(player)
		&& distanceToSqr(player) < (0.6f * SIZE) * (0.6f * SIZE)) {
		touchCooldown = 20;
		doHurtTarget(player);
	}
}

void MagmaCube::jumpFromGround() {
	yd = 0.42f + SIZE * 0.1f;
}

void MagmaCube::causeFallDamage(float distance) {
}

std::string MagmaCube::getHurtSound() { return "mob.magmacube.small"; }
std::string MagmaCube::getDeathSound() { return "mob.magmacube.small"; }

// Magma cream, one time in four (the Console Edition's nextInt(4) - 2, which is a
// negative number three times in four).
void MagmaCube::dropDeathLoot() {
	int count = random.nextInt(4) - 2;
	for (int i = 0; i < count; i++)
		spawnAtLocation(Item::magmaCream->id, 1);
}
