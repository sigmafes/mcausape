#ifndef NET_MINECRAFT_CLIENT_PARTICLE__SuspendedParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__SuspendedParticle_H__

#include "../../util/Mth.h"
#include "../../world/level/Level.h"
#include "Particle.h"
#include "../renderer/Tesselator.h"

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
		setSize(0.01f, 0.01f);
		size = size * (sharedRandom.nextFloat() * 0.4f + 0.2f);
		xd = (sharedRandom.nextFloat() * 2 - 1) * 0.004f;
		yd = (sharedRandom.nextFloat() * 2 - 1) * 0.004f;
		zd = (sharedRandom.nextFloat() * 2 - 1) * 0.004f;
		lifetime = 20 + sharedRandom.nextInt(60);
	}

	// A single pixel, not a picture: the quad samples one white texel of
	// particles.png (59, 0) and takes the particle's colour, a block's
	// pixel across (1/16), as Java's underwater specks look.
	void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
		const float u = 59.5f / 128.0f, v = 0.5f / 128.0f;
		const float r = 0.5f / 16.0f;
		const float px = (float) (xo + (x - xo) * a - xOff);
		const float py = (float) (yo + (y - yo) * a - yOff);
		const float pz = (float) (zo + (z - zo) * a - zOff);
		const float br = getBrightness(a);
		t.color(rCol * br, gCol * br, bCol * br);
		t.vertexUV(px - xa * r - xa2 * r, py - ya * r, pz - za * r - za2 * r, u, v);
		t.vertexUV(px - xa * r + xa2 * r, py + ya * r, pz - za * r + za2 * r, u, v);
		t.vertexUV(px + xa * r + xa2 * r, py + ya * r, pz + za * r + za2 * r, u, v);
		t.vertexUV(px + xa * r - xa2 * r, py - ya * r, pz + za * r - za2 * r, u, v);
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
