#ifndef NET_MINECRAFT_CLIENT_SKIN__PlayerSkins_H__
#define NET_MINECRAFT_CLIENT_SKIN__PlayerSkins_H__

#include <vector>

class Textures;

/** Custom player skins, in the layouts Minecraft itself uses.

    Two image sizes are accepted, and they map onto three models:
    - 64x32, the classic layout 0.6.1 was drawn for: the left arm and leg reuse
      the right ones mirrored, and the only outer layer is the hat.
    - 64x64, the layout the Java edition moved to in 1.8: the left limbs have
      their own texture, and every part has an outer layer (hat, jacket,
      sleeves, trouser legs). With arms either 4 pixels wide ("wide", Steve) or
      3 ("slim", Alex) -- the image does not say which, so it is read off the
      pixels the same way the community tools do.

    A skin is an id into this registry. 0 always means "no custom skin": the
    player is drawn exactly as before, with mob/char.png. The textures are
    uploaded here rather than through Textures' name map so a graphics reset
    can re-upload them -- nothing else knows these pixels. */
class PlayerSkins
{
public:
	enum Model { MODEL_CLASSIC = 1, MODEL_WIDE = 2, MODEL_SLIM = 3 };

	struct Skin
	{
		int w, h;
		Model model;
		std::vector<unsigned char> rgba;
		unsigned int glId;
	};

	/// Validates and cleans up an RGBA image. Returns a new skin id, or 0 when
	/// the image is not a skin (wrong size, or no data).
	static int create(int w, int h, const unsigned char* rgba);
	static void release(int id);
	/// Drops every skin but one -- the local one, when a multiplayer game ends.
	static void releaseAllBut(int keep);
	static const Skin* get(int id);

	/// Binds the skin's texture, uploading it first if needed.
	static void bind(Textures* textures, int id);
	static void onGraphicsReset();

	/// The skin from the player's profile, 0 for none.
	static int localSkin;

	static const int MaxBytes = 64 * 64 * 4;
};

#endif /*NET_MINECRAFT_CLIENT_SKIN__PlayerSkins_H__*/
