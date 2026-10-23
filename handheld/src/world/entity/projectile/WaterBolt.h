#ifndef NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__WaterBolt_H__
#define NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__WaterBolt_H__

#include "Throwable.h"
#include "../Mob.h"
#include "../../level/Level.h"
#include "../../phys/HitResult.h"

/// The aquatic blaze's shot: a bolt of water that hurts (4) and sets nothing
/// alight, keeps its speed under water (where a Throwable slows to a stop)
/// and leaves a trail of bubbles. Gone after five seconds.
class WaterBolt: public Throwable
{
	typedef Throwable super;
public:
	WaterBolt(Level* level) : super(level) { setup(); }
	WaterBolt(Level* level, Mob* owner, float x, float y, float z)
	:	super(level, x, y, z)
	{
		ownerId = owner->entityId;
		setup();
	}

	int getEntityTypeId() const { return EntityTypes::IdWaterBolt; }

	void tick() {
		const bool wet = isInWater();
		super::tick();
		// Throwable takes 20% a tick in water; give it back, less air's 1%.
		if (wet) { xd *= 0.99f / 0.80f; yd *= 0.99f / 0.80f; zd *= 0.99f / 0.80f; }
		for (int i = 0; i < 3; ++i)
			level->addParticle("bubble", x - xd * i / 3.0f, y - yd * i / 3.0f, z - zd * i / 3.0f, 0, 0.02f, 0);
		if (++age > 100 && !level->isClientSide) remove();
	}

	void onHit(const HitResult& res) {
		if (level->isClientSide) return;
		if (res.type == ENTITY) res.entity->hurt(this, 4);
		remove();
	}

protected:
	float getGravity() { return 0; }

private:
	void setup() {
		age = 0;
		entityRendererId = ER_WATERBOLT_RENDERER;
		setSize(5 / 16.0f, 5 / 16.0f);
	}
	int age;
};

#endif
