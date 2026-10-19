#ifndef NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Fireball_H__
#define NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Fireball_H__

//package net.minecraft.world.entity->projectile;

#include "Throwable.h"
#include "../Mob.h"
#include "../../Facing.h"
#include "../../level/Level.h"
#include "../../level/tile/Tile.h"
#include "../../level/tile/FireTile.h"
#include "../../phys/HitResult.h"

// The fireballs of ghasts (big, they explode) and blazes (small, they set fire to
// where they land). Ported from the Console Edition's Fireball and SmallFireball.
// A fireball never hits the mob that fired it: that is what the owner is for.
class Fireball: public Throwable
{
	typedef Throwable super;
public:
	Fireball(Level* level)
	:	super(level),
		isSmall(false)
	{
		setup();
	}

	Fireball(Level* level, Mob* owner, float x, float y, float z, bool isSmall)
	:	super(level, x, y, z),
		isSmall(isSmall)
	{
		ownerId = owner->entityId;
		setup();
	}

	virtual int getEntityTypeId() const {
		return EntityTypes::IdFireball;
	}

	void onHit(const HitResult& res) {
		if (level->isClientSide)
			return;

		if (res.type == ENTITY) {
			res.entity->hurt(this, isSmall? 5 : 6);
		}

		if (isSmall) {
			// Where it lands, a small fireball lights a fire in the free block in front.
			if (res.type != ENTITY) {
				int tx = res.x, ty = res.y, tz = res.z;
				switch (res.f) {
					case Facing::UP:    ty++; break;
					case Facing::DOWN:  ty--; break;
					case Facing::NORTH: tz--; break;
					case Facing::SOUTH: tz++; break;
					case Facing::EAST:  tx++; break;
					case Facing::WEST:  tx--; break;
				}
				if (level->isEmptyTile(tx, ty, tz))
					level->setTile(tx, ty, tz, Tile::fire->id);
			}
		} else {
			level->explode(this, x, y, z, 1.0f, true);
		}

		remove();
	}

protected:
	float getGravity() {
		return 0;
	}

private:
	void setup() {
		if (isSmall) {
			entityRendererId = ER_SMALLFIREBALL_RENDERER;
			setSize(5 / 16.0f, 5 / 16.0f);
		} else {
			entityRendererId = ER_FIREBALL_RENDERER;
			setSize(1, 1);
		}
	}

	bool isSmall;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Fireball_H__*/
