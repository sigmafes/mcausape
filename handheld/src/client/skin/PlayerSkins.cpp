#include "PlayerSkins.h"
#include "../renderer/Textures.h"
#include "../renderer/gles.h"

#include <map>

int PlayerSkins::localSkin = 0;

namespace {

std::map<int, PlayerSkins::Skin> g_skins;
int g_nextId = 1;

inline unsigned char& alphaAt(std::vector<unsigned char>& px, int w, int x, int y)
{
	return px[(y * w + x) * 4 + 3];
}

/// Minecraft draws every base-layer pixel opaque whatever the file says; a
/// stray transparent pixel there would otherwise be a hole in the player.
void setOpaque(std::vector<unsigned char>& px, int w, int x0, int y0, int x1, int y1)
{
	for (int y = y0; y < y1; ++y)
		for (int x = x0; x < x1; ++x)
			alphaAt(px, w, x, y) = 255;
}

/** The classic-skin hat rule, as Minecraft applies it: a hat area with no
    transparent pixel at all was painted solid by an editor that knew nothing of
    hats (often plain black), so the whole hat is dropped rather than drawn as
    a box round the head. One transparent pixel means it was meant. */
void classicHatRule(std::vector<unsigned char>& px, int w, int x0, int y0, int x1, int y1)
{
	for (int y = y0; y < y1; ++y)
		for (int x = x0; x < x1; ++x)
			if (alphaAt(px, w, x, y) < 128)
				return;
	for (int y = y0; y < y1; ++y)
		for (int x = x0; x < x1; ++x)
			alphaAt(px, w, x, y) = 0;
}

/** Slim (3-pixel) arms leave the fourth column of each arm's back empty (the
    right arm's at x 54-55, the left's at x 46-47). Nothing in the file says
    which model it is, so this is the same test the community skin tools use.

    "Empty" is transparent in a properly exported skin, but plenty of slim
    skins in circulation have those columns painted solid black instead --
    an editor that filled the unused area. Read as wide, the arm then grows a
    black stripe, so black counts as empty too. Both arms have to agree for
    the black case, since one black column alone could be a design. */
bool emptyRegion(std::vector<unsigned char>& px, int w, int x0, int y0, int x1, int y1, bool& transparent, bool& black)
{
	transparent = true;
	black = true;
	for (int y = y0; y < y1; ++y)
		for (int x = x0; x < x1; ++x) {
			const unsigned char* p = &px[(y * w + x) * 4];
			if (p[3] != 0) transparent = false;
			if (p[0] || p[1] || p[2] || p[3] < 200) black = false;
		}
	return transparent || black;
}

bool looksSlim(std::vector<unsigned char>& px, int w)
{
	bool rt, rb, lt, lb;
	emptyRegion(px, w, 54, 20, 56, 32, rt, rb);
	if (rt)
		return true;
	emptyRegion(px, w, 46, 52, 48, 64, lt, lb);
	return rb && (lt || lb);
}

void upload(Textures* textures, PlayerSkins::Skin& skin)
{
	GLuint id = 0;
	glGenTextures(1, &id);
	textures->bind(id);
	glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D2(GL_TEXTURE_2D, 0, GL_RGBA, skin.w, skin.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &skin.rgba[0]);
	skin.glId = id;
}

} // namespace

int PlayerSkins::create(int w, int h, const unsigned char* rgba)
{
	if (!rgba || w != 64 || (h != 32 && h != 64))
		return 0;

	Skin skin;
	skin.w = w;
	skin.h = h;
	skin.glId = 0;
	skin.rgba.assign(rgba, rgba + w * h * 4);

	if (h == 32) {
		skin.model = MODEL_CLASSIC;
		setOpaque(skin.rgba, w, 0, 0, 32, 16);
		setOpaque(skin.rgba, w, 0, 16, 64, 32);
		classicHatRule(skin.rgba, w, 32, 0, 64, 16);
	} else {
		// Before the base layer is made opaque: that would fill the very
		// columns this looks at.
		skin.model = looksSlim(skin.rgba, w) ? MODEL_SLIM : MODEL_WIDE;
		setOpaque(skin.rgba, w, 0, 0, 32, 16);
		setOpaque(skin.rgba, w, 0, 16, 64, 32);
		setOpaque(skin.rgba, w, 16, 48, 48, 64);
	}

	const int id = g_nextId++;
	g_skins[id] = skin;
	return id;
}

void PlayerSkins::release(int id)
{
	std::map<int, Skin>::iterator it = g_skins.find(id);
	if (it == g_skins.end())
		return;
	if (it->second.glId)
		glDeleteTextures(1, &it->second.glId);
	g_skins.erase(it);
}

void PlayerSkins::releaseAllBut(int keep)
{
	std::vector<int> ids;
	for (std::map<int, Skin>::iterator it = g_skins.begin(); it != g_skins.end(); ++it)
		if (it->first != keep)
			ids.push_back(it->first);
	for (unsigned int i = 0; i < ids.size(); ++i)
		release(ids[i]);
}

const PlayerSkins::Skin* PlayerSkins::get(int id)
{
	if (id <= 0)
		return 0;
	std::map<int, Skin>::const_iterator it = g_skins.find(id);
	return it == g_skins.end() ? 0 : &it->second;
}

void PlayerSkins::bind(Textures* textures, int id)
{
	std::map<int, Skin>::iterator it = g_skins.find(id);
	if (it == g_skins.end())
		return;
	if (!it->second.glId)
		upload(textures, it->second);
	else
		textures->bind(it->second.glId);
}

void PlayerSkins::onGraphicsReset()
{
	// The context that owned the old names is gone; the next bind uploads anew.
	for (std::map<int, Skin>::iterator it = g_skins.begin(); it != g_skins.end(); ++it)
		it->second.glId = 0;
}
