#include "Hoglin.h"
#include "../../level/tile/PortalTile.h"
#include "../EntityTypes.h"
#include "../MobFactory.h"
#include "../player/Player.h"
#include "../player/Inventory.h"
#include "../../Difficulty.h"
#include "../../level/Level.h"
#include "../../level/tile/Tile.h"
#include "../../level/dimension/Dimension.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../../../nbt/CompoundTag.h"
#include "../../../SharedConstants.h"

static const int DATA_HOGLIN_FLAGS = 16;	// bit 1: a baby

Hoglin::Hoglin(Level* level, bool zoglin)
:	super(level),
	zoglin(zoglin),
	inLove(0), breedCooldown(0), outsideTicks(0), growTicks(0), fearCheck(0),
	afraid(false)
{
	entityRendererId = ER_HOGLIN_RENDERER;
	textureName = zoglin ? "mob/zoglin.png" : "mob/hoglin.png";
	entityData.define(DATA_HOGLIN_FLAGS, (SynchedEntityData::TypeChar) 0);
	attackDamage = 6;
	runSpeed = 0.6f;
	setSize(1.3965f, 1.4f);
}

int Hoglin::getEntityTypeId() const { return zoglin ? MobTypes::Zoglin : MobTypes::Hoglin; }
int Hoglin::getMaxHealth() { return 40; }

bool Hoglin::canSpawn()
{
	// Whatever the light (the Nether has none to speak of), not on peaceful.
	return PathfinderMob::canSpawn() && level->difficulty > Difficulty::PEACEFUL;
}

bool Hoglin::isBaby() { return (entityData.getByte(DATA_HOGLIN_FLAGS) & 1) != 0; }

void Hoglin::setBaby(bool baby)
{
	entityData.set(DATA_HOGLIN_FLAGS, (SynchedEntityData::TypeChar) (baby ? 1 : 0));
	growTicks = baby ? 20 * 60 * SharedConstants::TicksPerSecond : 0;
	attackDamage = baby ? 1 : 6;
	if (baby) setSize(0.7f, 0.7f); else setSize(1.3965f, 1.4f);
}

bool Hoglin::nearWarpedFungus()
{
	const int cx = Mth::floor(x), cy = Mth::floor(y), cz = Mth::floor(z);
	for (int dx = -7; dx <= 7; ++dx)
		for (int dy = -3; dy <= 3; ++dy)
			for (int dz = -7; dz <= 7; ++dz)
				{
					// Java's hoglins shun warped fungi and nether portals alike.
					const int t = level->getTile(cx + dx, cy + dy, cz + dz);
					if (t == Tile::warpedFungus->id || t == Tile::portalTile->id) return true;
				}
	return false;
}

Entity* Hoglin::findAttackTarget()
{
	if (afraid) return NULL;
	return super::findAttackTarget();
}

bool Hoglin::doHurtTarget(Entity* target)
{
	const bool hit = super::doHurtTarget(target);
	// The charge throws its target up and away (Java's hoglin knockback is
	// the strongest of any mob's): a real launch, not a nudge.
	if (hit && !isBaby()) {
		const float dx = target->x - x, dz = target->z - z;
		const float d = Mth::sqrt(dx * dx + dz * dz);
		target->yd = 0.9f + random.nextFloat() * 0.25f;
		if (d > 0.01f) { target->xd += dx / d * 1.1f; target->zd += dz / d * 1.1f; }
	}
	return hit;
}

void Hoglin::aiStep()
{
	super::aiStep();
	if (level->isClientSide || !isAlive()) return;

	if (growTicks > 0 && --growTicks == 0) setBaby(false);
	if (breedCooldown > 0) --breedCooldown;

	if (!zoglin) {
		// Warped fungus near: it keeps away and will not fight.
		if (++fearCheck >= 40) { fearCheck = 0; afraid = nearWarpedFungus(); }
		if (afraid) setAttackTarget(NULL);

		// Out of the Nether for 15 seconds: a zoglin.
		if (level->dimension && level->dimension->id != Dimension::NETHER) {
			if (++outsideTicks > 15 * SharedConstants::TicksPerSecond) {
				Hoglin* zog = new Hoglin(level, true);
				zog->moveTo(x, y, z, yRot, xRot);
				if (isBaby()) zog->setBaby(true);
				level->addEntity(zog);
				remove();
				return;
			}
		} else {
			outsideTicks = 0;
		}

		// In love: another hoglin in love close by makes a piglet.
		if (inLove > 0) {
			--inLove;
			for (unsigned int i = 0; i < level->entities.size(); ++i) {
				Entity* e = level->entities[i];
				if (e == this || e->getEntityTypeId() != MobTypes::Hoglin) continue;
				Hoglin* o = (Hoglin*) e;
				if (o->inLove <= 0 || o->isBaby() || o->distanceToSqr(this) > 8 * 8) continue;
				Hoglin* baby = new Hoglin(level, false);
				baby->setBaby(true);
				baby->moveTo((x + o->x) / 2, y, (z + o->z) / 2, yRot, 0);
				level->addEntity(baby);
				inLove = o->inLove = 0;
				breedCooldown = o->breedCooldown = 5 * 60 * SharedConstants::TicksPerSecond;
				break;
			}
		}
	}
}

bool Hoglin::interact(Player* player)
{
	ItemInstance* item = player->inventory->getSelected();
	if (!zoglin && item && !item->isNull() && item->id == Tile::crimsonFungus->id && !isBaby() && breedCooldown == 0 && inLove == 0) {
		if (!player->abilities.instabuild && --item->count <= 0)
			player->inventory->clearSlot(player->inventory->selected);
		if (!level->isClientSide) {
			inLove = 30 * SharedConstants::TicksPerSecond;
			for (int i = 0; i < 7; ++i)
				level->addParticle("reddust", x + (random.nextFloat() - 0.5f) * bbWidth, y + random.nextFloat() * bbHeight,
					z + (random.nextFloat() - 0.5f) * bbWidth, 0, 0, 0);
		}
		return true;
	}
	return super::interact(player);
}

int Hoglin::getDeathLoot() { return zoglin ? 0 : Item::porkChop_raw->id; }

void Hoglin::dropDeathLoot()
{
	if (zoglin || isBaby()) return;
	// Cooked if it died burning, as Java's hoglins drop.
	const int pork = 2 + random.nextInt(3), id = isOnFire() ? Item::porkChop_cooked->id : Item::porkChop_raw->id;
	for (int i = 0; i < pork; ++i) spawnAtLocation(id, 1);
	if (random.nextInt(2) == 0) spawnAtLocation(Item::leather->id, 1);
}

void Hoglin::addAdditonalSaveData(CompoundTag* tag)
{
	super::addAdditonalSaveData(tag);
	tag->putShort("Baby", isBaby() ? 1 : 0);
	tag->putInt("GrowTicks", growTicks);
}

void Hoglin::readAdditionalSaveData(CompoundTag* tag)
{
	super::readAdditionalSaveData(tag);
	if (tag->getShort("Baby")) { setBaby(true); growTicks = tag->getInt("GrowTicks"); }
}
