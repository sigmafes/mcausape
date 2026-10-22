#include "Pufferfish.h"
#include "../EntityTypes.h"
#include "../EntityRendererId.h"
#include "../player/Player.h"
#include "../../effect/MobEffect.h"
#include "../../level/Level.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../../item/DyePowderItem.h"

Pufferfish::Pufferfish(Level* level)
:	super(level, false),
	puffState(0),
	inflateCounter(0),
	deflateTimer(0)
{
	entityRendererId = ER_PUFFERFISH_RENDERER;
	textureName = "mob/pufferfish.png";
	setPuff(0);
}

int Pufferfish::getEntityTypeId() const { return MobTypes::Pufferfish; }

int Pufferfish::getDeathLoot() { return Item::pufferfish->id; }

void Pufferfish::dropDeathLoot()
{
	spawnAtLocation(Item::pufferfish->id, 1);
	if (random.nextInt(20) == 0)
		spawnAtLocation(new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 0);
}

void Pufferfish::setPuff(int state)
{
	puffState = state;
	// Java: 0.7 a side, scaled 0.5 / 0.7 / 1.0 with the puff.
	const float s = state == 0 ? 0.5f : (state == 1 ? 0.7f : 1.0f);
	setSize(0.7f * s, 0.7f * s);
}

void Pufferfish::aiStep()
{
	super::aiStep();

	// A player (not in creative) within two blocks.
	Player* near = NULL;
	AABB around = bb.grow(2, 2, 2);
	for (unsigned int i = 0; i < level->players.size(); ++i) {
		Player* p = level->players[i];
		if (p->isAlive() && !p->abilities.instabuild && p->bb.intersects(around)) { near = p; break; }
	}

	if (near) {
		deflateTimer = 0;
		if (puffState == 0) setPuff(1);
		else if (puffState == 1 && ++inflateCounter > 40) setPuff(2);
	} else if (puffState > 0) {
		inflateCounter = 0;
		if (++deflateTimer > 60) {
			deflateTimer = 0;
			setPuff(puffState - 1);
		}
	}

	// The sting, on the host only (a visitor hears of it by the hurt and
	// the effect it sends).
	if (level->isClientSide || puffState == 0 || !isAlive()) return;
	for (unsigned int i = 0; i < level->players.size(); ++i) {
		Player* p = level->players[i];
		if (!p->isAlive() || p->abilities.instabuild || !p->bb.intersects(bb.grow(0.1f, 0.1f, 0.1f))) continue;
		if (p->hurt(this, 1 + puffState))
			p->addEffect(MobEffects::Poison, MobEffects::PufferPoisonTicks);
	}
}
