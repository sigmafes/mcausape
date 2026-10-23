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

static const int DATA_PIGLIN_FLAGS = 16;	// bit 1: admiring a gold ingot

Piglin::Piglin(Level* level, bool brute)
:	super(level),
	brute(brute),
	admireTicks(0), angryAt(0), angryTicks(0), outsideTicks(0), pickupCheck(0)
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

bool Piglin::admiring() { return (entityData.getByte(DATA_PIGLIN_FLAGS) & 1) != 0; }

ItemInstance* Piglin::getCarriedItem()
{
	static ItemInstance held;
	if (admiring()) held = ItemInstance(Item::goldIngot);
	else held = ItemInstance(hasAxe() ? Item::hatchet_gold : Item::sword_gold);
	return &held;
}

int Piglin::getAttackDamage(Entity* target)
{
	// Java: a piglin's sword 8 (axe 9 on a brute's arm, 13 at hard).
	if (brute) return 9;
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
		if (!brute && wearsGold(p)) continue;
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
    ingots, a magma cream; common -- 2-4 gravel, 2-4 soul sand, 5-10
    netherrack, 1-2 bones, 1-2 gunpowder. One roll in eleven is rare. */
void Piglin::barter()
{
	int id, count;
	const int roll = random.nextInt(44);
	switch (roll) {
		case 0: id = Item::emerald->id; count = 1; break;	// the diamond (0.6 calls it emerald)
		case 1: id = Tile::obsidian->id; count = 1; break;
		case 2: id = Item::ironIngot->id; count = 1 + random.nextInt(3); break;
		case 3: id = Item::magmaCream->id; count = 1; break;
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
		spawnAtLocation(hasAxe() ? Item::hatchet_gold->id : Item::sword_gold->id, 1);
}
