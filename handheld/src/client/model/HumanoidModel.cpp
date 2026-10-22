#include "HumanoidModel.h"
#include "../Minecraft.h"
#include "../../util/Mth.h"
#include <cmath>
#include "../../world/entity/player/Player.h"
#include "../../world/entity/player/Inventory.h"

HumanoidModel::HumanoidModel( float g /*= 0*/, float yOffset /*= 0*/ )
:	holdingLeftHand(false),
	holdingRightHand(false),
	sneaking(false),
	bowAndArrow(false),
	swimAmount(0),
	head(0, 0),
	//ear (24, 0),
	//hair(32, 0),
	body(16, 16),
	arm0(24 + 16, 16),
	arm1(24 + 16, 16),
	leg0(0, 16),
	leg1(0, 16)
{
	head.setModel(this);
	body.setModel(this);
	arm0.setModel(this);
	arm1.setModel(this);
	leg0.setModel(this);
	leg1.setModel(this);

	head.addBox(-4, -8, -4, 8, 8, 8, g); // Head
	head.setPos(0, 0 + yOffset, 0);

	//ear.addBox(-3, -6, -1, 6, 6, 1, g); // Ear

	//hair.addBox(-4, -8, -4, 8, 8, 8, g + 0.5f); // Head
	//      hair.setPos(0, 0 + yOffset, 0);

	body.addBox(-4, 0, -2, 8, 12, 4, g); // Body
	body.setPos(0, 0 + yOffset, 0);

	arm0.addBox(-3, -2, -2, 4, 12, 4, g); // Arm0
	arm0.setPos(-5, 2 + yOffset, 0);

	arm1.mirror = true;
	arm1.addBox(-1, -2, -2, 4, 12, 4, g); // Arm1
	arm1.setPos(5, 2 + yOffset, 0);

	leg0.addBox(-2, 0, -2, 4, 12, 4, g); // Leg0
	leg0.setPos(-2, 12 + yOffset, 0);

	leg1.mirror = true;
	leg1.addBox(-2, 0, -2, 4, 12, 4, g); // Leg1
	leg1.setPos(2, 12 + yOffset, 0);
}

void HumanoidModel::render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale )
{
	if(e != NULL && e->isMob()) {		
		Mob* mob = (Mob*)(e);
		ItemInstance* item = mob->getCarriedItem();
		if(item != NULL) {
			if(mob->getUseItemDuration() >  0) {
				UseAnim::UseAnimation anim = item->getUseAnimation();
				if(anim == UseAnim::bow) {
					bowAndArrow = true;
				}
			}
		}
	}
	
	setupAnim(time, r, bob, yRot, xRot, scale);
	
	head.render(scale);
	body.render(scale);
	arm0.render(scale);
	arm1.render(scale);
	leg0.render(scale);
	leg1.render(scale);
	bowAndArrow = false;
	//hair.render(scale);
}

void HumanoidModel::render( HumanoidModel* model, float scale )
{
	head.yRot = model->head.yRot;
	head.y = model->head.y;
	head.xRot = model->head.xRot;
	//hair.yRot = head.yRot;
	//hair.xRot = head.xRot;

	arm0.xRot = model->arm0.xRot;
	arm0.yRot = model->arm0.yRot;
	arm0.zRot = model->arm0.zRot;

	arm1.xRot = model->arm1.xRot;
	arm1.yRot = model->arm1.yRot;
	arm1.zRot = model->arm1.zRot;

	leg0.xRot = model->leg0.xRot;
	leg1.xRot = model->leg1.xRot;

	head.render(scale);
	body.render(scale);
	arm0.render(scale);
	arm1.render(scale);
	leg0.render(scale);
	leg1.render(scale);
	//hair.render(scale);
}

void HumanoidModel::renderHorrible( float time, float r, float bob, float yRot, float xRot, float scale )
{
	setupAnim(time, r, bob, yRot, xRot, scale);
	head.renderHorrible(scale);
	body.renderHorrible(scale);
	arm0.renderHorrible(scale);
	arm1.renderHorrible(scale);
	leg0.renderHorrible(scale);
	leg1.renderHorrible(scale);
	//hair.renderHorrible(scale);
}
// Updated to match Minecraft Java, all except hair.
void HumanoidModel::setupAnim( float time, float r, float bob, float yRot, float xRot, float scale )
{
	head.yRot = yRot / (180 / Mth::PI);
	head.xRot = xRot / (180 / Mth::PI);
	const float tcos0 = Mth::cos(time * 0.6662f) * r;
	const float tcos1 = Mth::cos(time * 0.6662f + Mth::PI) * r;

	arm0.xRot = tcos1;
	arm1.xRot = tcos0;
	arm0.zRot = 0;
	arm1.zRot = 0;

	leg0.xRot = tcos0 * 1.4f;
	leg1.xRot = tcos1 * 1.4f;
	leg0.yRot = 0;
	leg1.yRot = 0;

	if (riding) {
		arm0.xRot += -Mth::PI / 2 * 0.4f;
		arm1.xRot += -Mth::PI / 2 * 0.4f;
		leg0.xRot = -Mth::PI / 2 * 0.8f;
		leg1.xRot = -Mth::PI / 2 * 0.8f;
		leg0.yRot = Mth::PI / 2 * 0.2f;
		leg1.yRot = -Mth::PI / 2 * 0.2f;
	}

	if (holdingLeftHand != 0) {
		arm1.xRot = arm1.xRot * 0.5f - Mth::PI / 2.0f * 0.2f * holdingLeftHand;
	}
	if (holdingRightHand != 0) {
		arm0.xRot = arm0.xRot * 0.5f - Mth::PI / 2.0f * 0.2f * holdingRightHand;
	}
	arm0.yRot = 0;
	arm1.yRot = 0;

	if (attackTime > -9990) {
		float swing = attackTime;
		body.yRot = Mth::sin(Mth::sqrt(swing) * Mth::PI * 2) * 0.2f;
		arm0.z = Mth::sin(body.yRot) * 5;
		arm0.x = -Mth::cos(body.yRot) * 5;
		arm1.z = -Mth::sin(body.yRot) * 5;
		arm1.x = Mth::cos(body.yRot) * 5;
		arm0.yRot += body.yRot;
		arm1.yRot += body.yRot;
		arm1.xRot += body.yRot;

		swing = 1 - attackTime;
		swing *= swing;
		swing *= swing;
		swing = 1 - swing;
		float aa = Mth::sin(swing * Mth::PI);
		float bb = Mth::sin(attackTime * Mth::PI) * -(head.xRot - 0.7f) * 0.75f;
		arm0.xRot -= aa * 1.2f + bb;
		arm0.yRot += body.yRot * 2;
		arm0.zRot = Mth::sin(attackTime * Mth::PI) * -0.4f;
	}

	if (sneaking) {
		body.xRot = 0.5f;
		arm0.xRot += 0.4f;
		arm1.xRot += 0.4f;
		leg0.z = +4.0f;
		leg1.z = +4.0f;
		leg0.y = +9.0f;
		leg1.y = +9.0f;
		head.y = +1;
	} else {
		body.xRot = 0.0f;
		leg0.z = +0.0f;
		leg1.z = +0.0f;
		leg0.y = +12.0f;
		leg1.y = +12.0f;
		head.y = 0;
	}

	const float bcos = Mth::cos(bob * 0.09f) * 0.05f + 0.05f;
	const float bsin = Mth::sin(bob * 0.067f) * 0.05f;

	arm0.zRot += bcos;
	arm1.zRot -= bcos;
	arm0.xRot += bsin;
	arm1.xRot -= bsin;

	if (bowAndArrow) {
		float attack2 = 0;
		float attack = 0;

		arm0.zRot = 0;
		arm1.zRot = 0;
		arm0.yRot = -(0.1f - attack2 * 0.6f) + head.yRot;
		arm1.yRot = +(0.1f - attack2 * 0.6f) + head.yRot + 0.4f;
		arm0.xRot = -Mth::PI / 2.0f + head.xRot;
		arm1.xRot = -Mth::PI / 2.0f + head.xRot;
		arm0.xRot -= attack2 * 1.2f - attack * 0.4f;
		arm1.xRot -= attack2 * 1.2f - attack * 0.4f;

		arm0.zRot += bcos;
		arm1.zRot -= bcos;
		arm0.xRot += bsin;
		arm1.xRot -= bsin;
	}

	if (swimAmount > 0)
		setupSwim(time);
}

/// Java's AnimationUtils/HumanoidModel helpers: the stroke's arm sweep, and an
/// angle blend that goes the short way round.
static float quadraticArmUpdate(float f) { return -65.0f * f + f * f; }
static float rotlerpRad(float f, float from, float to) {
	const float tau = Mth::PI * 2;
	float d = fmodf(to - from, tau);
	if (d < -Mth::PI) d += tau;
	if (d >= Mth::PI) d -= tau;
	return from + f * d;
}
static float lerp(float f, float from, float to) { return from + f * (to - from); }

/** The crawl stroke and the kick from Java Edition 1.13's HumanoidModel,
    blended in by swimAmount. `time` is the walk cycle, which only moves while
    the player does: in still water the arms come to rest. The cycle is 26
    long: arms reach forward overhead (0-14), sweep down to the sides (14-22),
    and come back (22-26). arm0 is the right arm, arm1 the left. */
void HumanoidModel::setupSwim(float time)
{
	const float s = swimAmount;
	// The hitting arm finishes its swing instead of stroking.
	const float right = attackTime > 0 ? 0 : s;
	const float left = s;
	const float halfPi = Mth::PI / 2;
	const float t = fmodf(time, 26.0f);

	if (t < 14.0f) {
		const float sweep = 1.8707964f * quadraticArmUpdate(t) / quadraticArmUpdate(14.0f);
		arm1.xRot = rotlerpRad(left, arm1.xRot, 0.0f);
		arm0.xRot = lerp(right, arm0.xRot, 0.0f);
		arm1.yRot = rotlerpRad(left, arm1.yRot, Mth::PI);
		arm0.yRot = lerp(right, arm0.yRot, Mth::PI);
		arm1.zRot = rotlerpRad(left, arm1.zRot, Mth::PI + sweep);
		arm0.zRot = lerp(right, arm0.zRot, Mth::PI - sweep);
	} else if (t < 22.0f) {
		const float o = (t - 14.0f) / 8.0f;
		arm1.xRot = rotlerpRad(left, arm1.xRot, halfPi * o);
		arm0.xRot = lerp(right, arm0.xRot, halfPi * o);
		arm1.yRot = rotlerpRad(left, arm1.yRot, Mth::PI);
		arm0.yRot = lerp(right, arm0.yRot, Mth::PI);
		arm1.zRot = rotlerpRad(left, arm1.zRot, 5.012389f - 1.8707964f * o);
		arm0.zRot = lerp(right, arm0.zRot, 1.2707963f + 1.8707964f * o);
	} else {
		const float o = (t - 22.0f) / 4.0f;
		arm1.xRot = rotlerpRad(left, arm1.xRot, halfPi - halfPi * o);
		arm0.xRot = lerp(right, arm0.xRot, halfPi - halfPi * o);
		arm1.yRot = rotlerpRad(left, arm1.yRot, Mth::PI);
		arm0.yRot = lerp(right, arm0.yRot, Mth::PI);
		arm1.zRot = rotlerpRad(left, arm1.zRot, Mth::PI);
		arm0.zRot = lerp(right, arm0.zRot, Mth::PI);
	}

	// A small kick, the legs opposite each other.
	leg1.xRot = lerp(s, leg1.xRot, 0.3f * Mth::cos(time * 0.33333334f + Mth::PI));
	leg0.xRot = lerp(s, leg0.xRot, 0.3f * Mth::cos(time * 0.33333334f));

	// Lying flat the head looks ahead, not at the bottom.
	head.xRot = rotlerpRad(s, head.xRot, -Mth::PI / 4);
}

void HumanoidModel::onGraphicsReset()
{
	head.onGraphicsReset();
	body.onGraphicsReset();
	arm0.onGraphicsReset();
	arm1.onGraphicsReset();
	leg0.onGraphicsReset();
	leg1.onGraphicsReset();
	//hair.onGraphicsReset();
}

//void renderHair(float scale) {
//    hair.yRot = head.yRot;
//    hair.xRot = head.xRot;
//    hair.render(scale);
//}
//
//void renderEars(float scale) {
//    ear.yRot = head.yRot;
//    ear.xRot = head.xRot;
//    ear.x=0;
//    ear.y=0;
//    ear.render(scale);
//}
//
