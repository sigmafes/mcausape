#ifndef NET_MINECRAFT_CLIENT_RENDERER_PTEXTURE__DynamicTexture_H__
#define NET_MINECRAFT_CLIENT_RENDERER_PTEXTURE__DynamicTexture_H__

#include <vector>

class Textures;

class DynamicTexture
{
public:
    int tex;
	int replicate;
    unsigned char pixels[16*16*4];

    DynamicTexture(int tex_);
	virtual ~DynamicTexture() {}

	virtual void tick() = 0;
    virtual void bindTexture(Textures* tex);
};

class WaterTexture: public DynamicTexture
{
    typedef DynamicTexture super;
    int _tick;
	int _frame;

	float* current;
	float* next;
	float* heat;
	float* heata;

public:
    WaterTexture();
	~WaterTexture();

    void tick();
};

class WaterSideTexture: public DynamicTexture
{
	typedef DynamicTexture super;
	int _tick;
	int _frame;
	int _tickCount;

	float* current;
	float* next;
	float* heat;
	float* heata;

public:
	WaterSideTexture();
	~WaterSideTexture();

	void tick();
};

// The portal's swirl. Not a copy of any image: 32 frames of a slow spiral are
// worked out once, and tick() plays them in a loop over the portal's atlas slot.
class PortalTexture: public DynamicTexture
{
	typedef DynamicTexture super;
	int _tick;
	unsigned char frames[32][16*16*4];

public:
	/// The Nether portal's swirl, or with `aquarium` the Aquarium portal's,
	/// the same spirals in cyan, for the tile `tex`.
	PortalTexture(int tex = -1, bool aquarium = false);

	void tick();
};

// The fire: 32 frames taken from a Java Edition fire picture, played in a loop. The
// fire block shows two layers (atlas cells tex and tex + 16), so there are two
// of these, a few frames apart.
class FireTexture: public DynamicTexture
{
	typedef DynamicTexture super;
	int _tick;
	int _offset;

public:
	FireTexture(int tex, int frameOffset);

	void tick();
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_PTEXTURE__DynamicTexture_H__*/
