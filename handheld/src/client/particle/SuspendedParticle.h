#ifndef NET_MINECRAFT_CLIENT_PARTICLE__SuspendedParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__SuspendedParticle_H__

#include "../../util/Mth.h"
#include "../../world/level/Level.h"
#include "Particle.h"

// Java's underwater particle (SuspendedParticle): a speck of a pixel or two
// hanging in the water, drifting a little, gone in a few seconds. The fog
// tints it; the Aquarium makes them round the camera (Minecraft::tick).
class SuspendedParticle: public Particle
{
	typedef Particle super;
public:
	SuspendedParticle(Level* level, float x, float y, float z)
	:	super(level, x, y, z, 0, 0, 0)
	{
		rCol = 0.75f;
		gCol = 0.95f;
		bCol = 1.0f;
		tex = 7;	// the smallest of the smoke frames: a dot
		setSize(0.01f, 0.01f);
		size = size * (sharedRandom.nextFloat() * 0.4f + 0.2f);
		xd = (sharedRandom.nextFloat() * 2 - 1) * 0.004f;
		yd = (sharedRandom.nextFloat() * 2 - 1) * 0.004f;
		zd = (sharedRandom.nextFloat() * 2 - 1) * 0.004f;
		lifetime = 20 + sharedRandom.nextInt(60);
	}

	void tick() {
		xo = x;
		yo = y;
		zo = z;
		x += xd;
		y += yd;
		z += zd;
		if (age++ >= lifetime) remove();
	}
};

#endif
