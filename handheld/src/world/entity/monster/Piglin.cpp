#include "Piglin.h"
#include "PigZombie.h"
#include "../EntityTypes.h"
#include "../item/ItemEntity.h"
#include "../player/Player.h"
#include "../player/Inventory.h"
#include "../../Difficulty.h"
#include "../../level/Level.h"
#include "../../level/tile/Tile.h"
#include "../../level/dimension/Dimension.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../../../SharedConstants.h"
#include "../projectile/Arrow.h"
#include "../../../util/Mth.h"
#include <cmath>

static const int DATA_PIGLIN_FLAGS = 16;	// bit 1: admiring a gold ingot

Piglin::Piglin(Level* level, bool brute)
:	super(level),
	brute(brute),
	admireTicks(0), angryAt(0), angryTicks(0), outsideTicks(0), pickupCheck(0),
	inFortress(false), fortressCheck(0)
{
	entityRendererId = ER_PIGLIN_RENDERER;
	textureName = brute ? "mob/piglin_brute.png" : "mob/piglin.png";
	entityData.define(DATA_PIGLIN_FLAGS, (SynchedEntityData::TypeChar) 0);
	runSpeed = 0.5f;
}

int Piglin::getEntityTypeId() const { return brute ? MobTypes::PiglinBrute : MobTypes::Piglin; }
int Piglin::getMaxHealth() { return brute ? 50 : 16; }

bool Piglin::canSpawn()
{
	return PathfinderMob::canSpawn() && level->difficulty > Difficulty::PEACEFUL;
}

bool Piglin::hasAxe() const
{
	// One in four, from the entity id: the same on every machine.
	return brute || ((unsigned int) entityId * 2654435761u >> 16) % 4 == 0;
}

bool Piglin::hasBow() const
{
	// Another one in four (Java's crossbow piglins): keeps its distance and
	// shoots. From the id too, so visitors draw the same weapon.
	return !brute && ((unsigned int) entityId * 2654435761u >> 16) % 4 == 1;
}

float Piglin::getWalkingSpeedModifier()
{
	// Charging a player, piglins break into a run (Java's piglins sprint at
	// their target); the brute at full tilt.
	float speed = super::getWalkingSpeedModifier();
	if (getAttackTarget() && !admiring()) speed *= brute ? 1.8f : 1.6f;
	return speed;
}

void Piglin::checkHurtTarget(Entity* target, float d)
{
	if (!hasBow()) { super::checkHurtTarget(target, d); return; }
	// The archer: shoots from up to 14 blocks, stands its ground to do it.
	if (d < 14) {
		const float dx = target->x - x, dz = target->z - z;
		if (attackTime == 0 && canSee(target)) {
			Arrow* arrow = new Arrow(level, this, 1);
			const float dy = (target->y + target->getHeadHeight() - 0.7f) - arrow->y;
			const float lift = Mth::sqrt(dx * dx + dz * dz) * 0.2f;
			level->playSound(this, "random.bow", 1.0f, 1 / (random.nextFloat() * 0.4f + 0.8f));
			level->addEntity(arrow);
			arrow->shoot(dx, dy + lift, dz, 1.6f, 12);
			attackTime = SharedConstants::TicksPerSecond * 2;
		}
		yRot = (float) (std::atan2(dz, dx) * Mth::RADDEG) - 90;
		holdGround = d < 8;
	}
}

bool Piglin::admiring() { return (entityData.getByte(DATA_PIGLIN_FLAGS) & 1) != 0; }

ItemInstance* Piglin::getCarriedItem()
{
	static ItemInstance held;
	if (admiring()) held = ItemInstance(Item::goldIngot);
	else if (hasBow()) held = ItemInstance(Item::bow);
	else held = ItemInstance(hasAxe() ? Item::hatchet_gold : Item::sword_gold);
	return &held;
}

int Piglin::getAttackDamage(Entity* target)
{
	// Java: a piglin's sword 8 (axe 9 on a brute's arm, 13 at hard). The
	// brute's gold axe hits three times as hard as it would (27): the
	// reason to fear a bastion.
	if (brute) return 27;
	return hasAxe() ? 7 : 6;
}

bool Piglin::wearsGold(Player* p)
{
	for (int i = 0; i < 4; ++i) {	// the four armour slots
		ItemInstance* a = p->getArmor(i);
		if (!a || a->isNull()) continue;
		if (a->id == Item::helmet_gold->id || a->id == Item::chestplate_gold->id
			|| a->id == Item::leggings_gold->id || a->id == Item::boots_gold->id) return true;
	}
	return false;
}

Entity* Piglin::findAttackTarget()
{
	if (admiring()) return NULL;
	if (angryAt) {
		Entity* e = level->getEntity(angryAt);
		if (e && e->isAlive()) return e;
		angryAt = 0;
	}
	Player* best = NULL;
	float bestD = 16 * 16;
	for (unsigned int i = 0; i < level->players.size(); ++i) {
		Player* p = level->players[i];
		if (!p->isAlive() || p->abilities.instabuild) continue;
		if (!brute && !inFortress && wearsGold(p)) continue;
		const float d = p->distanceToSqr(this);
		if (d < bestD && canSee(p)) { bestD = d; best = p; }
	}
	return best;
}

bool Piglin::hurt(Entity* source, int dmg)
{
	const bool hit = super::hurt(source, dmg);
	if (hit && source && source->isPlayer()) {
		angryAt = source->entityId;
		angryTicks = 30 * SharedConstants::TicksPerSecond;
		if (admiring()) { admireTicks = 0; entityData.set(DATA_PIGLIN_FLAGS, (SynchedEntityData::TypeChar) 0); }
	}
	return hit;
}

void Piglin::startAdmiring()
{
	admireTicks = 6 * SharedConstants::TicksPerSecond;
	entityData.set(DATA_PIGLIN_FLAGS, (SynchedEntityData::TypeChar) 1);
	setAttackTarget(NULL);
}

bool Piglin::interact(Player* player)
{
	ItemInstance* item = player->inventory->getSelected();
	if (!brute && item && !item->isNull() && item->id == Item::goldIngot->id && !admiring() && angryAt == 0) {
		if (!player->abilities.instabuild && --item->count <= 0)
			player->inventory->clearSlot(player->inventory->selected);
		if (!level->isClientSide) startAdmiring();
		return true;
	}
	return super::interact(player);
}

/** The barter table asked for: rare -- a diamond, an obsidian, 1-3 iron
    ingots, a magma cream, a Potion of Fire Resistance; common -- 2-4 gravel, 2-4 soul sand, 5-10
    netherrack, 1-2 bones, 1-2 gunpowder. One roll in eleven is rare. */
void Piglin::barter()
{
	int id, count;
	// One roll in 45 is a Potion of Fire Resistance, the rarest offer.
	const int roll = random.nextInt(45);
	switch (roll) {
		case 0: id = Item::emerald->id; count = 1; break;	// the diamond (0.6 calls it emerald)
		case 1: id = Tile::obsidian->id; count = 1; break;
		case 2: id = Item::ironIngot->id; count = 1 + random.nextInt(3); break;
		case 3: id = Item::magmaCream->id; count = 1; break;
		case 44: id = Item::fireResistancePotion->id; count = 1; break;
		default:
			switch ((roll - 4) % 5) {
				case 0: id = Tile::gravel->id; count = 2 + random.nextInt(3); break;
				case 1: id = Tile::hellSand->id; count = 2 + random.nextInt(3); break;
				case 2: id = Tile::netherrack->id; count = 5 + random.nextInt(6); break;
				case 3: id = Item::bone->id; count = 1 + random.nextInt(2); break;
				default: id = Item::sulphur->id; count = 1 + random.nextInt(2); break;
			}
	}
	ItemEntity* drop = spawnAtLocation(new ItemInstance(id, count, 0), 1.0f);
	if (drop) {
		// Tossed a little forward, as Java's piglin throws its offer.
		drop->xd = -Mth::sin(yRot * Mth::DEGRAD) * 0.2f;
		drop->zd = Mth::cos(yRot * Mth::DEGRAD) * 0.2f;
		drop->yd = 0.2f;
	}
}

void Piglin::aiStep()
{
	super::aiStep();
	if (level->isClientSide || !isAlive()) return;

	if (angryTicks > 0 && --angryTicks == 0) angryAt = 0;

	// In a fortress, gold or no gold, it attacks (the bastions' garrison
	// moved in). Judged by what it stands on, every two seconds.
	if (--fortressCheck <= 0) {
		fortressCheck = 40;
		inFortress = false;
		const int fx = Mth::floor(x), fy = Mth::floor(bb.y0), fz = Mth::floor(z);
		for (int dy = 1; dy <= 3 && !inFortress; ++dy) {
			const int t = level->getTile(fx, fy - dy, fz);
			inFortress = t == Tile::netherBrick->id || t == Tile::netherFence->id || t == Tile::stairs_netherBricks->id
				|| t == Tile::polishedBlackstoneBricks->id || t == Tile::crackedPolishedBlackstoneBricks->id;
		}
	}

	if (admireTicks > 0) {
		// Stands looking at the gold.
		xd = zd = 0;
		setAttackTarget(NULL);
		if (--admireTicks == 0) {
			entityData.set(DATA_PIGLIN_FLAGS, (SynchedEntityData::TypeChar) 0);
			barter();
		}
	} else if (!brute && angryAt == 0 && ++pickupCheck >= 10) {
		// A gold ingot on the ground at its feet: picked up and admired.
		pickupCheck = 0;
		for (unsigned int i = 0; i < level->entities.size(); ++i) {
			Entity* e = level->entities[i];
			if (!e->isItemEntity() || e->removed) continue;
			ItemEntity* ie = (ItemEntity*) e;
			if (ie->item.id != Item::goldIngot->id || ie->distanceToSqr(this) > 2.0f * 2.0f) continue;
			if (--ie->item.count <= 0) ie->remove();
			startAdmiring();
			break;
		}
	}

	// Out of the Nether for 15 seconds: zombified.
	if (level->dimension && level->dimension->id != Dimension::NETHER) {
		if (++outsideTicks > 15 * SharedConstants::TicksPerSecond) {
			PigZombie* z = new PigZombie(level);
			z->moveTo(x, y, this->z, yRot, xRot);
			level->addEntity(z);
			remove();
		}
	} else {
		outsideTicks = 0;
	}
}

void Piglin::dropDeathLoot()
{
	// Java: its weapon now and then (8.5%).
	if (random.nextInt(12) == 0)
		spawnAtLocation(hasBow() ? Item::bow->id : hasAxe() ? Item::hatchet_gold->id : Item::sword_gold->id, 1);
	if (hasBow() && random.nextInt(2) == 0) spawnAtLocation(Item::arrow->id, 1 + random.nextInt(2));
}
