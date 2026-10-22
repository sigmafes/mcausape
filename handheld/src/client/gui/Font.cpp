#include "Font.h"

//#include "SharedConstants.h"
#include "../Options.h"
#include "../renderer/Textures.h"
#include "../renderer/Tesselator.h"
#include "../renderer/gles.h"
#include "../../util/Mth.h"
#include <cstring>
#include <vector>
#include <cctype>

Font::Font( Options* options, const std::string& name, Textures* textures )
:	options(options),
	fontTexture(0),
	fontName(name),
	index(0),
	count(0),
	_textures(textures),
	_x(0), _y(0),
	_cols(16), _rows(16),
	_charOffset(0),
	lineHeight(DefaultLineHeight)
{
	init(options);
}


//Font::Font( Options* options, const std::string& name, Textures* textures, int imgW, int imgH, int x, int y, int cols, int rows, unsigned char charOffset )
//:	options(options),
//	fontTexture(0),
//	fontName(name),
//	index(0),
//	count(0),
//	_textures(textures),
//	_x(x), _y(y),
//	_cols(cols), _rows(rows),
//	_charOffset(charOffset)
//{
//	init(options);
//}

void Font::onGraphicsReset()
{
	init(options);
}

void Font::init( Options* options )
{
	TextureId fontTexture = _textures->loadTexture(fontName);
	const TextureData* tex = _textures->getTemporaryTextureData(fontTexture);

	if (!tex)
		return;

	unsigned char* rawPixels = tex->data;

	const int numChars = _rows * _cols;
	for (int i = 0; i < numChars; i++) {
		int xt = i % _cols;
		int yt = i / _cols;

		int x = 7;
		for (; x >= 0; x--) {
			int xPixel = _x + xt * 8 + x;
			bool emptyColumn = true;
			for (int y = 0; y < 8 && emptyColumn; y++) {
				int yPixel = _y + (yt * 8 + y) * tex->w;
				unsigned char pixelalpha = rawPixels[(xPixel + yPixel) << 2];
				if (pixelalpha > 0) emptyColumn = false;
			}
			if (!emptyColumn) {
				break;
			}
		}

		if (i == ' ') x = 4 - 2;
		charWidths[i] = x + 2;
		fcharWidths[i] = (float) charWidths[i];
	}

#ifdef USE_VBO
	return; // this <1
#endif

#ifndef USE_VBO
	listPos = glGenLists(256 + 32);

	Tesselator& t = Tesselator::instance;
	for (int i = 0; i < 256; i++) {
		glNewList(listPos + i, GL_COMPILE);
		// @attn @huge @note: This is some dangerous code right here / Aron, added ^1
		t.begin();
		buildChar(i);
		t.end(false, -1);

		glTranslatef2((GLfloat)charWidths[i], 0.0f, 0.0f);
		glEndList();
	}

	for (int i = 0; i < 32; i++) {
		int br = ((i >> 3) & 1) * 0x55;
		int r = ((i >> 2) & 1) * 0xaa + br;
		int g = ((i >> 1) & 1) * 0xaa + br;
		int b = ((i >> 0) & 1) * 0xaa + br;
		if (i == 6) {
			r += 0x55;
		}
		bool darken = i >= 16;

		if (options->anaglyph3d) {
			int cr = (r * 30 + g * 59 + b * 11) / 100;
			int cg = (r * 30 + g * 70) / (100);
			int cb = (r * 30 + b * 70) / (100);

			r = cr;
			g = cg;
			b = cb;
		}

		// color = r << 16 | g << 8 | b;
		if (darken) {
			r /= 4;
			g /= 4;
			b /= 4;
		}

		glNewList(listPos + 256 + i, GL_COMPILE);
		glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
		glEndList();
	}
#endif
}

void Font::drawShadow( const std::string& str, float x, float y, int color )
{
	draw(str, x + 1, y + 1, color, true);
	draw(str, x, y, color);
}
void Font::drawShadow( const char* str, float x, float y, int color )
{
	draw(str, x + 1, y + 1, color, true);
	draw(str, x, y, color);
}

void Font::draw( const std::string& str, float x, float y, int color )
{
	draw(str, x, y, color, false);
}

void Font::draw( const char* str, float x, float y, int color )
{
	draw(str, x, y, color, false);
}

void Font::draw( const char* str, float x, float y, int color, bool darken )
{
#ifdef USE_VBO
	drawSlow(str, x, y, color, darken);
#endif
}

void Font::draw( const std::string& str, float x, float y, int color, bool darken )
{
#ifdef USE_VBO
	drawSlow(str, x, y, color, darken);
	return;
#endif

	if (str.empty()) return;

	if (darken) {
		int oldAlpha = color & 0xff000000;
		color = (color & 0xfcfcfc) >> 2;
		color += oldAlpha;
	}

	_textures->loadAndBindTexture(fontName);
	float r = ((color >> 16) & 0xff) / 255.0f;
	float g = ((color >> 8) & 0xff) / 255.0f;
	float b = ((color) & 0xff) / 255.0f;
	float a = ((color >> 24) & 0xff) / 255.0f;
	if (a == 0) a = 1;
	glColor4f2(r, g, b, a);

	static const std::string hex("0123456789abcdef");

	index = 0;
	glPushMatrix2();
	glTranslatef2((GLfloat)x, (GLfloat)y, 0.0f);
	for (unsigned int i = 0; i < str.length(); i++) {
		while (str.length() > i + 1 && str[i] == '§') {
			int cc = hex.find((char)tolower(str[i + 1]));
			if (cc < 0 || cc > 15) cc = 15;
			lists[index++] = listPos + 256 + cc + (darken ? 16 : 0);

			if (index == 1024) {
				count = index;
				index = 0;
#ifndef USE_VBO
				glCallLists(count, GL_UNSIGNED_INT, lists);
#endif
				count = 1024;
			}

			i += 2;
		}

		if (i < str.length()) {
			//int ch = SharedConstants.acceptableLetters.indexOf(str.charAt(i));
			char ch = str[i];
			if (ch >= 0) {
				//ib.put(listPos + ch + 32);
				lists[index++] = listPos + ch;
			}
		}

		if (index == 1024) {
			count = index;
			index = 0;
#ifndef USE_VBO
			glCallLists(count, GL_UNSIGNED_INT, lists);
#endif
			count = 1024;
		}
	}
	count = index;
	index = 0;
#ifndef USE_VBO
	glCallLists(count, GL_UNSIGNED_INT, lists);
#endif
	glPopMatrix2();
}

int Font::width( const std::string& str )
{
	int maxLen = 0;
	int len = 0;

	for (unsigned int i = 0; i < str.length(); i++) {
		if (str[i] == '§') {
			i++;
		} else {
			//int ch = SharedConstants.acceptableLetters.indexOf(str.charAt(i));
			//if (ch >= 0) {
			//    len += charWidths[ch + 32];
			//}
			if (str[i] == '\n') {
				if (len > maxLen) maxLen = len;
				len = 0;
			}
			else {
				int charWidth = charWidths[ (unsigned char) str[i] ];
				len += charWidth;
			}
		}
	}
	return maxLen>len? maxLen : len;
}

int Font::height( const std::string& str ) {
	int h = 0;
	bool hasLine = false;
	for (unsigned int i = 0; i < str.length(); ++i) {
		if (str[i] == '\n') hasLine = true;
		else {
			if (hasLine) h += lineHeight;
			hasLine = false;
		}
	}
	return h;
}

std::string Font::sanitize( const std::string& str )
{
	std::string sanitized(str.length() + 1, 0);
	int j = 0;

	for (unsigned int i = 0; i < str.length(); i++) {
		if (str[i] == '§') {
			i++;
			//} else if (SharedConstants.acceptableLetters.indexOf(str.charAt(i)) >= 0) {
		} else {
			sanitized[j++] = str[i];
		}
	}
	return sanitized.erase(j);
}

void Font::drawWordWrap( const std::string& str, float x, float y, float w, int col )
{
	char* cstr = new char[str.length() + 1];
	strncpy(cstr, str.c_str(), str.length());
	cstr[str.length()] = 0;

	const char* lims = " \n\t\r";
	char* ptok = strtok(cstr, lims);

	std::vector<std::string> words;
	while (ptok != NULL) {
		words.push_back( ptok );
		ptok = strtok(NULL, lims);
	}

	delete[] cstr;

	int pos = 0;
	while (pos < (int)words.size()) {
		std::string line = words[pos++] + " ";
		while (pos < (int)words.size() && width(line + words[pos]) < w) {
			line += words[pos++] + " ";
		}
		drawShadow(line, x, y, col);
		y += lineHeight;
	}
}

void Font::drawSlow( const std::string& str, float x, float y, int color, bool darken /*= false*/ ) {
	drawSlow(str.c_str(), x, y, color, darken);
}
void Font::drawSlow( const char* str, float x, float y, int color, bool darken /*= false*/ )
{
	if (!str) return;

	if (darken) {
		int oldAlpha = color & 0xff000000;
		color = (color & 0xfcfcfc) >> 2;
		color += oldAlpha;
	}

	_textures->loadAndBindTexture(fontName);

	// The glyph atlas is black wherever there is no letter, and only its alpha
	// cuts that out. Whether blending is on was left to whoever drew last, so
	// text drawn straight after something that turned it off -- the world list,
	// a button -- came out as a black box with letters in it.
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Tesselator& t = Tesselator::instance;
	t.begin();
	int alpha = (0xff000000 & color) >> 24;
	if (!alpha) alpha = 0xff;
	t.color((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff, alpha);
	
	t.addOffset((float)x, (float)y, 0);
	float xOffset = 0;
	float yOffset = 0;

	while (unsigned char ch = *(str++)) {
		if (ch == '\n') {
			xOffset = 0;
			yOffset += lineHeight;
		} else {
			buildChar(ch, xOffset, yOffset);
			xOffset += fcharWidths[ch];
		}
	}
	t.draw();
	t.addOffset(-(float)x, -(float)y, 0);
}

void Font::buildChar( unsigned char i, float x /*= 0*/, float y /*=0*/, float slant /*=0*/ )
{
	Tesselator& t = Tesselator::instance;

	//i -= _charOffset;
	//int ix = (i % _cols) * 8 + _x;
	//int iy = (i / _cols) * 8 + _y;
	float ix = (float)((i & 15) * 8);
	float iy = (float)((i >> 4) * 8);
	float s = 7.99f;

	float uo = (0.0f) / 128.0f;
	float vo = (0.0f) / 128.0f;

	t.vertexUV(x, y + s, 0, ix / 128.0f + uo, (iy + s) / 128.0f + vo);
	t.vertexUV(x + s, y + s, 0, (ix + s) / 128.0f + uo, (iy + s) / 128.0f + vo);
	t.vertexUV(x + s + slant, y, 0, (ix + s) / 128.0f + uo, iy / 128.0f + vo);
	t.vertexUV(x + slant, y, 0, ix / 128.0f + uo, iy / 128.0f + vo);
}

// Java's sixteen text colours (ChatFormatting).
static const int FormatColors[16] = {
	0x000000, 0x0000aa, 0x00aa00, 0x00aaaa, 0xaa0000, 0xaa00aa, 0xffaa00, 0xaaaaaa,
	0x555555, 0x5555ff, 0x55ff55, 0x55ffff, 0xff5555, 0xff55ff, 0xffff55, 0xffffff
};

/// An underline or strike-through to draw under drawFormatted's letters.
struct FormatLine { float x0, x1, y; int rgb; };

static int formatCode(char c) {
	c = (char) tolower(c);
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + c - 'a';
	if (c == 'k' || c == 'l' || c == 'm' || c == 'n' || c == 'o' || c == 'r') return 100 + c;
	return -1;
}

/// "&zN" (N a colour): a gradient, letter by letter, to the next "&zN".
/// Returns the colour index, or -1 if str[i] does not start one.
static int gradientCode(const std::string& str, unsigned int i) {
	if (str[i] != '&' || i + 2 >= str.size() || tolower(str[i + 1]) != 'z') return -1;
	const int c = formatCode(str[i + 2]);
	return c >= 0 && c < 16 ? c : -1;
}

std::string Font::stripFormatting(const std::string& str) {
	std::string out;
	for (unsigned int i = 0; i < str.size(); ++i) {
		if (gradientCode(str, i) >= 0) { i += 2; continue; }
		if (str[i] == '&' && i + 1 < str.size() && formatCode(str[i + 1]) >= 0) { ++i; continue; }
		out += str[i];
	}
	return out;
}

int Font::widthFormatted(const std::string& str) {
	int w = 0;
	bool bold = false;
	for (unsigned int i = 0; i < str.size(); ++i) {
		if (gradientCode(str, i) >= 0) { bold = false; i += 2; continue; }
		if (str[i] == '&' && i + 1 < str.size()) {
			const int code = formatCode(str[i + 1]);
			if (code >= 0) {
				if (code < 16 || code == 100 + 'r') bold = false;
				else if (code == 100 + 'l') bold = true;
				++i;
				continue;
			}
		}
		w += charWidths[(unsigned char) str[i]] + (bold ? 1 : 0);
	}
	return w;
}

void Font::drawFormattedShadow(const std::string& str, float x, float y, int color) {
	drawFormatted(str, x + 1, y + 1, color, true);
	drawFormatted(str, x, y, color, false);
}

void Font::drawFormatted(const std::string& str, float x, float y, int color, bool darken)
{
	const int alpha = ((color >> 24) & 0xff) ? ((color >> 24) & 0xff) : 0xff;
	int rgb = color & 0xffffff;
	bool scrambled = false, bold = false, strike = false, underline = false, italic = false;

	std::vector<FormatLine> lines;

	_textures->loadAndBindTexture(fontName);
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.addOffset(x, y, 0);
	float cx = 0;
	// A running &z gradient: from gradFrom to gradTo over gradCount letters.
	int gradFrom = 0, gradTo = 0, gradCount = 0, gradIndex = 0;
	for (unsigned int i = 0; i < str.size(); ++i) {
		const int grad = gradientCode(str, i);
		if (grad >= 0) {
			scrambled = bold = strike = underline = italic = false;
			if (gradCount > 0) {
				// The closing code: the text after it keeps the end colour.
				rgb = FormatColors[grad];
				gradCount = 0;
			} else {
				// Find the closing &z and count the letters up to it.
				int letters = 0;
				unsigned int j = i + 3;
				int end = -1;
				for (; j < str.size(); ++j) {
					if ((end = gradientCode(str, j)) >= 0) break;
					if (str[j] == '&' && j + 1 < str.size() && formatCode(str[j + 1]) >= 0) { ++j; continue; }
					++letters;
				}
				rgb = FormatColors[grad];
				if (end >= 0 && letters > 0) {
					// Unclosed, it is just the colour, as "&N" would be.
					gradFrom = FormatColors[grad];
					gradTo = FormatColors[end];
					gradCount = letters;
					gradIndex = 0;
				}
			}
			i += 2;
			continue;
		}
		if (str[i] == '&' && i + 1 < str.size()) {
			const int code = formatCode(str[i + 1]);
			if (code >= 0) {
				if (code < 16) {
					// A colour ends the styles, as in Java.
					rgb = FormatColors[code];
					gradCount = 0;
					scrambled = bold = strike = underline = italic = false;
				} else switch (code - 100) {
					case 'k': scrambled = true; break;
					case 'l': bold = true; break;
					case 'm': strike = true; break;
					case 'n': underline = true; break;
					case 'o': italic = true; break;
					case 'r': gradCount = 0; rgb = color & 0xffffff; scrambled = bold = strike = underline = italic = false; break;
				}
				++i;
				continue;
			}
		}
		unsigned char ch = (unsigned char) str[i];
		// &k: a random letter of the same width, every frame.
		if (scrambled && ch != ' ') {
			for (int tries = 0; tries < 16; ++tries) {
				unsigned char r = (unsigned char) (33 + Mth::random() * 94);
				if (charWidths[r] == charWidths[ch]) { ch = r; break; }
			}
		}
		if (gradCount > 0) {
			const float f = gradCount > 1 ? (float) gradIndex / (gradCount - 1) : 0.0f;
			int mixed = 0;
			for (int sh = 0; sh <= 16; sh += 8) {
				const int a0 = (gradFrom >> sh) & 0xff, a1 = (gradTo >> sh) & 0xff;
				mixed |= ((int) (a0 + (a1 - a0) * f + 0.5f) & 0xff) << sh;
			}
			rgb = mixed;
			++gradIndex;
		}
		int c = rgb;
		if (darken) c = (c & 0xfcfcfc) >> 2;
		t.color((c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff, alpha);
		const float slant = italic ? 1.0f : 0.0f;
		buildChar(ch, cx, 0, slant);
		if (bold) buildChar(ch, cx + 1, 0, slant);
		const float w = fcharWidths[ch] + (bold ? 1 : 0);
		if (underline) { FormatLine l = { cx, cx + w, 8, c }; lines.push_back(l); }
		if (strike)    { FormatLine l = { cx, cx + w, 3.5f, c }; lines.push_back(l); }
		cx += w;
	}
	t.draw();
	if (!lines.empty()) {
		glDisable2(GL_TEXTURE_2D);
		t.begin();
		for (unsigned int i = 0; i < lines.size(); ++i) {
			const FormatLine& l = lines[i];
			t.color((l.rgb >> 16) & 0xff, (l.rgb >> 8) & 0xff, l.rgb & 0xff, alpha);
			t.vertex(l.x0, l.y + 1, 0);
			t.vertex(l.x1, l.y + 1, 0);
			t.vertex(l.x1, l.y, 0);
			t.vertex(l.x0, l.y, 0);
		}
		t.draw();
		glEnable2(GL_TEXTURE_2D);
	}
	t.addOffset(-x, -y, 0);
}


