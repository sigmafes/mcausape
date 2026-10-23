#ifndef NET_MINECRAFT_CLIENT_PARTICLE__NetherAmbientParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__NetherAmbientParticle_H__

#include "SuspendedParticle.h"

/** Java 1.16's Nether biome ambience: crimson spores (red, sinking), warped
    spores (blue, rising), the soul sand valley's ash (dark, drifting down)
    and the basalt deltas' white ash (pale flakes blown sideways). All are
    the same pixel speck as SuspendedParticle, tinted and moving their own
    way; Minecraft::tick makes them round the camera. */
class NetherAmbientParticle: public SuspendedParticle
{
	typedef SuspendedParticle super;
public:
	enum Kind { CrimsonSpore, WarpedSpore, Ash, WhiteAsh };

	NetherAmbientParticle(Level* level, float x, float y, float z, int kind)
	:	super(level, x, y, z)
	{
		float s = 1;
		switch (kind) {
		case CrimsonSpore: rCol = 0.9f; gCol = 0.1f;  bCol = 0.1f;  yd = -0.008f; s = 1.6f; break;
		case WarpedSpore:  rCol = 0.1f; gCol = 0.65f; bCol = 0.8f;  yd = 0.008f;  s = 1.6f; break;
		case Ash:          rCol = 0.35f; gCol = 0.33f; bCol = 0.3f; yd = -0.012f; s = 1.3f; break;
		default:           rCol = 0.9f; gCol = 0.9f;  bCol = 0.88f; yd = -0.02f;
			xd = 0.03f + sharedRandom.nextFloat() * 0.02f; zd = (sharedRandom.nextFloat() - 0.5f) * 0.02f; s = 1.8f; break;
		}
		size *= s;
		lifetime = 60 + sharedRandom.nextInt(80);
	}

	void tick() {
		// A gentle wobble, so they drift rather than slide.
		xd += (sharedRandom.nextFloat() - 0.5f) * 0.002f;
		zd += (sharedRandom.nextFloat() - 0.5f) * 0.002f;
		super::tick();
		if (level->getTile(Mth::floor(x), Mth::floor(y), Mth::floor(z)) != 0) remove();
	}
};

#endif
