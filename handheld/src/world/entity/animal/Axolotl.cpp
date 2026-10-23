#include "Axolotl.h"
#include "../EntityTypes.h"
#include "../EntityRendererId.h"
#include "../player/Player.h"
#include "../player/Inventory.h"
#include "../../level/Level.h"
#include "../../level/dimension/Dimension.h"
#include "../../level/dimension/AquariumDimension.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../../../nbt/CompoundTag.h"
#include "../../../SharedConstants.h"
#include "../../../util/Mth.h"

Axolotl::Axolotl(Level* level)
:	super(level, false),
	attackId(0),
	biteCooldown(0)
{
	entityRendererId = ER_AXOLOTL_RENDERER;
	textureName = "mob/axolotl.png";
	setSize(0.75f, 0.42f);
}

int Axolotl::getEntityTypeId() const { return MobTypes::Axolotl; }

int Axolotl::getMaxHealth() { return 14; }

bool Axolotl::isTamingFood(int id)
{
	return id == Item::rawCod->id || id == Item::rawSalmon->id
		|| id == Item::pufferfish->id || id == Item::shrimp->id;
}

bool Axolotl::interact(Player* player)
{
	ItemInstance* item = player->inventory->getSelected();
	if (!item || item->isNull()) return super::interact(player);

	// A water bucket takes it up, owner and all (aux 1 = tamed: whoever
	// lets it out again owns it).
	if (item->id == Item::bucket_water->id) {
		ItemInstance bucket(Item::axolotlBucket, 1, owner.empty() ? 0 : 1);
		if (item->count == 1) *item = bucket;
		else {
			item->count--;
			if (!player->inventory->add(&bucket)) player->drop(new ItemInstance(bucket), false);
		}
		if (!level->isClientSide) remove();
		return true;
	}

	if (isTamingFood(item->id) && owner.empty()) {
		if (!player->abilities.instabuild && --item->count <= 0)
			player->inventory->clearSlot(player->inventory->selected);
		if (!level->isClientSide) {
			const bool tamed = random.nextInt(3) == 0;
			if (tamed) owner = player->name;
			for (int i = 0; i < 7; ++i)
				level->addParticle(tamed ? "mobSpell" : "smoke",
					x + (random.nextFloat() - 0.5f) * bbWidth, y + random.nextFloat() * bbHeight,
					z + (random.nextFloat() - 0.5f) * bbWidth,
					tamed ? 1.0f : 0, tamed ? 0.4f : 0, tamed ? 0.7f : 0);
		}
		return true;
	}
	return super::interact(player);
}

bool Axolotl::hurt(Entity* source, int dmg)
{
	// Its owner's swings do not land on it.
	if (source && source->isPlayer() && !owner.empty() && ((Player*) source)->name == owner)
		return false;
	return super::hurt(source, dmg);
}

Player* Axolotl::findOwner()
{
	if (owner.empty()) return NULL;
	for (unsigned int i = 0; i < level->players.size(); ++i)
		if (level->players[i]->name == owner && level->players[i]->isAlive()) return level->players[i];
	return NULL;
}

void Axolotl::aiStep()
{
	super::aiStep();
	if (level->isClientSide || !isAlive() || owner.empty()) return;
	if (biteCooldown > 0) --biteCooldown;

	Player* p = findOwner();
	if (!p) {
		// The owner left the Aquarium (or the world): wait by the portal.
		if (level->dimension && level->dimension->id == Dimension::AQUARIUM) {
			const float hx = AquariumDimension::HubX + 3.0f, hy = AquariumDimension::HubY - 2.0f, hz = AquariumDimension::HubZ + 3.0f;
			const float dx = hx - x, dy = hy - y, dz = hz - z;
			if (dx * dx + dy * dy + dz * dz > 5 * 5) { tx = hx; ty = hy; tz = hz; hasTarget = true; }
		}
		attackId = 0;
		return;
	}

	// Whatever the owner just hit (in the last five seconds), it goes for.
	if (p->lastAttackedId != 0 && p->lastAttackedId != entityId
			&& level->getTime() - p->lastAttackedTime < SharedConstants::TicksPerSecond * 5)
		attackId = p->lastAttackedId;
	Entity* prey = attackId ? level->getEntity(attackId) : NULL;
	if (prey && (!prey->isAlive() || prey->isPlayer() || prey->distanceToSqr(this) > 20 * 20)) {
		prey = NULL;
		attackId = 0;
	}

	const float d2 = p->distanceToSqr(this);
	if (!prey && d2 > 12 * 12) {
		// Too far behind: there at once, by the owner.
		moveTo(p->x, p->y - p->heightOffset + 0.2f, p->z, yRot, xRot);
		xd = yd = zd = 0;
		return;
	}
	if (prey) {
		tx = prey->x; ty = prey->y; tz = prey->z;
		hasTarget = true;
		if (biteCooldown == 0 && prey->distanceToSqr(this) < 1.8f * 1.8f) {
			prey->hurt(this, 2);
			biteCooldown = SharedConstants::TicksPerSecond;
		}
	} else if (d2 > 3 * 3) {
		tx = p->x; ty = p->y - p->heightOffset + 0.5f; tz = p->z;
		hasTarget = true;
	}
}

void Axolotl::dryStep()
{
	// On land it walks after its target instead of flopping, and breathes.
	if (!hasTarget) return;
	float dx = tx - x, dz = tz - z;
	const float d = sqrtf(dx * dx + dz * dz);
	if (d < 1.0f) { hasTarget = false; return; }
	xd = dx / d * 0.12f;
	zd = dz / d * 0.12f;
	yRot = yBodyRot = -atan2f(dx, dz) * 180 / Mth::PI;
	if (horizontalCollision && onGround) yd = 0.42f;
}

void Axolotl::addAdditonalSaveData(CompoundTag* tag)
{
	super::addAdditonalSaveData(tag);
	tag->putString("Owner", owner);
}

void Axolotl::readAdditionalSaveData(CompoundTag* tag)
{
	super::readAdditionalSaveData(tag);
	owner = tag->getString("Owner");
}
