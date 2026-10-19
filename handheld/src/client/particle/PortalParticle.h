#ifndef NET_MINECRAFT_CLIENT_PARTICLE__PortalParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__PortalParticle_H__

//package net.minecraft.client.particle;

#include "Particle.h"
#include "../renderer/Tesselator.h"
#include "../../world/level/Level.h"

// The purple specks that stream into a portal. Each one starts where the
// portal tile spawned it (xa/ya/za are the offset it travels over its life),
// eases along that path, and shrinks away at the end, as in 1.0.
class PortalParticle: public Particle
{
	typedef Particle super;
	float oSize;
	float xStart, yStart, zStart;
public:
	PortalParticle(Level* level, float x, float y, float z, float xa, float ya, float za)
	:	super(level, x, y, z, xa, ya, za)
	{
		xd = xa;
		yd = ya;
		zd = za;
		xStart = this->x = x;
		yStart = this->y = y;
		zStart = this->z = z;

		float br = sharedRandom.nextFloat() * 0.6f + 0.4f;
		oSize = size = sharedRandom.nextFloat() * 0.2f + 0.5f;
		rCol = gCol = bCol = 1.0f * br;
		gCol *= 0.3f;
		rCol *= 0.9f;

		lifetime = (int) (Mth::random() * 10) + 40;
		noPhysics = true;
		// Row 0 of particles.png is the generic speck sequence.
		tex = (int) (Mth::random() * 8);
	}

	void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
		float s = (age + a) / lifetime;
		s = 1 - s;
		s = s * s;
		s = 1 - s;
		size = oSize * s;
		super::render(t, a, xa, ya, za, xa2, za2);
	}

	float getBrightness(float a) {
		float br = super::getBrightness(a);
		float pos = (float) age / lifetime;
		pos = pos * pos * pos * pos;
		return br * (1 - pos) + pos;
	}

	void tick() {
		xo = x;
		yo = y;
		zo = z;

		float pos = (float) age / lifetime;
		float a = pos;
		pos = -pos + pos * pos * 2;
		pos = 1 - pos;
		x = xStart + xd * pos;
		y = yStart + yd * pos + (1 - a);
		z = zStart + zd * pos;

		if (age++ >= lifetime) remove();
	}
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__PortalParticle_H__*/
