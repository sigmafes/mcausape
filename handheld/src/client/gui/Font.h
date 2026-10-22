#ifndef NET_MINECRAFT_CLIENT_GUI__Font_H__
#define NET_MINECRAFT_CLIENT_GUI__Font_H__

//package net.minecraft.client.gui;

#include <string>
#include <cctype>

#include "../renderer/gles.h"

class Textures;
class Options;

class Font
{
public:
    Font(Options* options, const std::string& name, Textures* textures);
	//Font(Options* options, const std::string& name, Textures* textures, int imgW, int imgH, int x, int y, int cols, int rows, unsigned char charOffset);
	
	void init(Options* options);
	void onGraphicsReset();

	void draw(const char* str, float x, float y, int color);
    void draw(const std::string& str, float x, float y, int color);
	void draw(const char* str, float x, float y, int color, bool darken);
    void draw(const std::string& str, float x, float y, int color, bool darken);
	void drawShadow(const std::string& str, float x, float y, int color);
	void drawShadow(const char* str, float x, float y, int color);
	void drawWordWrap(const std::string& str, float x, float y, float w, int col);

	int width(const std::string& str);

	/** Text with Java's & codes (docs: chat, signs, the motd -- never names):
	    &0-&f colours, &k scrambled, &l bold, &m struck through, &n
	    underlined, &o italic, &r back to plain. `color` is the colour before
	    any code, and its alpha is kept throughout. */
	void drawFormatted(const std::string& str, float x, float y, int color, bool darken = false);
	void drawFormattedShadow(const std::string& str, float x, float y, int color);
	/// How wide drawFormatted draws it (codes take no room, bold one more a letter).
	int widthFormatted(const std::string& str);
	/// The text with its & codes taken out.
	static std::string stripFormatting(const std::string& str);
	int height(const std::string& str);

	static std::string sanitize(const std::string& str);
private:
	void buildChar(unsigned char i, float x = 0, float y = 0, float slant = 0);
	void drawSlow(const std::string& str, float x, float y, int color, bool darken = false);
	void drawSlow(const char* str, float x, float y, int color, bool darken = false);
public:
	int fontTexture;
	int lineHeight;
	static const int DefaultLineHeight = 10;
private:
	int charWidths[256];
	float fcharWidths[256];
	int listPos;

	int index;
	int count;
	GLuint lists[1024];

	std::string fontName;
	Textures* _textures;

	Options* options;

	int _x, _y;
	int _cols;
	int _rows;
	unsigned char _charOffset;
};

#endif /*NET_MINECRAFT_CLIENT_GUI__Font_H__*/
