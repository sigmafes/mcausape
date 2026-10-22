#ifndef NET_MINECRAFT_CLIENT_PARTICLE__SpellParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__SpellParticle_H__

#include "../../util/Mth.h"
#include "../../world/level/Level.h"
#include "Particle.h"

// The swirl a mob under a potion effect gives off (Java's mobSpell; the
// Console Edition's SpellParticle): it rises slowly, shrinking through the
// eight spell pictures in row 8 of particles.png, in the effects' colour.
class SpellParticle: public Particle
{
	typedef Particle super;
public:
	SpellParticle(Level* level, float x, float y, float z, float r, float g, float b)
	:	super(level, x, y, z, 0, 0, 0)
	{
		yd *= 0.2f;
		xd *= 0.1f;
		zd *= 0.1f;
		size *= 0.75f;
		rCol = r;
		gCol = g;
		bCol = b;
		lifetime = (int) (8 / (Mth::random() * 0.8f + 0.2f));
		tex = 8 * 16 + 7;
	}

	void tick() {
		xo = x;
		yo = y;
		zo = z;
		if (age++ >= lifetime) remove();
		tex = 8 * 16 + (7 - age * 8 / lifetime);
		yd += 0.004f;
		move(xd, yd, zd);
		if (y == yo) {
			xd *= 1.1f;
			zd *= 1.1f;
		}
		xd *= 0.96f;
		yd *= 0.96f;
		zd *= 0.96f;
		if (onGround) {
			xd *= 0.7f;
			zd *= 0.7f;
		}
	}
};

#endif
