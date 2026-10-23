#include "Drowned.h"
#include "../EntityTypes.h"
#include "../player/Player.h"
#include "../../level/Level.h"
#include "../../level/material/Material.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../../../util/Mth.h"
#include <math.h>

Drowned::Drowned(Level* level)
:	super(level)
{
	textureName = "mob/drowned.png";
}

int Drowned::getEntityTypeId() const { return MobTypes::Drowned; }

bool Drowned::hasTrident() const
{
	// 15%, the same answer on every machine.
	return ((unsigned int) entityId * 2654435761u >> 16) % 100 < 15;
}

ItemInstance* Drowned::getCarriedItem()
{
	static ItemInstance trident;
	if (!hasTrident()) return NULL;
	trident = ItemInstance(Item::trident);
	return &trident;
}

Player* Drowned::nearestPrey(Mob* from, float range)
{
	Player* best = NULL;
	float bestD = range * range;
	for (unsigned int i = 0; i < from->level->players.size(); ++i) {
		Player* p = from->level->players[i];
		if (!p->isAlive() || p->abilities.instabuild) continue;
		const float d = p->distanceToSqr(from);
		if (d < bestD) { bestD = d; best = p; }
	}
	return best;
}

void Drowned::updateAi()
{
	super::updateAi();
	jumping = false;
}

void Drowned::travel(float xa, float ya)
{
	if (!isInWater()) { super::travel(xa, ya); return; }
	if (!level->isClientSide) {
		Player* p = nearestPrey(this, (float) HuntRange);
		if (p) {
			// Straight at the player's chest, at a swimmer's pace.
			const float dx = p->x - x, dy = (p->y - p->heightOffset + 0.4f) - y, dz = p->z - z;
			const float d = sqrtf(dx * dx + dy * dy + dz * dz);
			if (d > 0.01f) {
				const float speed = 0.13f;
				xd += (dx / d * speed - xd) * 0.25f;
				yd += (dy / d * speed - yd) * 0.25f;
				zd += (dz / d * speed - zd) * 0.25f;
				yRot = yBodyRot = -atan2f(dx, dz) * Mth::RADDEG;
				xRot = -atan2f(dy, sqrtf(dx * dx + dz * dz)) * Mth::RADDEG;
			}
			if (attackTime <= 0 && d < 1.6f) {
				attackTime = 20;
				doHurtTarget(p);
			}
		} else {
			xd *= 0.8f; yd *= 0.8f; zd *= 0.8f;
		}
	}
	move(xd, yd, zd);
}
