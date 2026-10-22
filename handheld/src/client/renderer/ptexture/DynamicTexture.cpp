#include "DynamicTexture.h"
#include "FireFrames.h"
#include "KelpFrames.h"

#include <cstring>
#include "../Textures.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../world/level/tile/PortalTile.h"
#include <cmath>
#include "../../../util/Mth.h"

//
// DynamicTexture
//
DynamicTexture::DynamicTexture(int tex_)
:   tex(tex_),
	replicate(1)
{
	memset(pixels, 0, 16*16*4);
}

void DynamicTexture::bindTexture(Textures* tex) {
	tex->loadAndBindTexture("terrain.png");
}

//
// WaterTexture
// I was thinking of adding something simple (a simple frame copy from a
// "still water image sequence") every n:th tick for calm water, and shifting
// the rows of a texture for the running water. I might do that, but I got
// impressed over the java code, so I will try that first.. and I suspect they
// wont mix very good.
/*
WaterTexture::WaterTexture()
:   super(Tile::water->tex),
	_tick(0),
	_frame(0)
{
}

void WaterTexture::tick() {
}
*/

WaterTexture::WaterTexture()
:   super(Tile::water->tex),
	_tick(0),
	_frame(0)
{
	current = new float[16*16];
	next = new float[16*16];
	heat = new float[16*16];
	heata = new float[16*16];

	for (int i = 0; i < 256; ++i) {
		current[i] = 0;
		next[i] = 0;
		heat[i] = 0;
		heata[i] = 0;
	}
}

WaterTexture::~WaterTexture() {
	delete[] current;
	delete[] next;
	delete[] heat;
	delete[] heata;
}

void WaterTexture::tick()
{
	for (int x = 0; x < 16; x++)
		for (int y = 0; y < 16; y++) {
			float pow = 0;
			for (int xx = x - 1; xx <= x + 1; xx++) {
				int xi = (xx) & 15;
				int yi = (y) & 15;
				pow += current[xi + yi * 16];
			}
			next[x + y * 16] = pow / 3.3f + heat[x + y * 16] * 0.8f;
		}

	for (int x = 0; x < 16; x++)
		for (int y = 0; y < 16; y++) {
			heat[x + y * 16] += heata[x + y * 16] * 0.05f;

			if (heat[x + y * 16] < 0) heat[x + y * 16] = 0;
			heata[x + y * 16] -= 0.1f;
			if (Mth::random() < 0.05f) {
				heata[x + y * 16] = 0.5f;
			}
		}

	float* tmp = next;
	next = current;
	current = tmp;

	for (int i = 0; i < 256; i++) {
		float pow = current[i];
		if (pow > 1) pow = 1;
		if (pow < 0) pow = 0;

		float pp = pow * pow;

		int r = (int) (32 + pp * 32);
		int g = (int) (50 + pp * 64);
		int b = (int) (255);
		int a = (int) (146 + pp * 50);

		//if (anaglyph3d) {
		//	int rr = (r * 30 + g * 59 + b * 11) / 100;
		//	int gg = (r * 30 + g * 70) / (100);
		//	int bb = (r * 30 + b * 70) / (100);

		//	r = rr;
		//	g = gg;
		//	b = bb;
		//}

		pixels[i * 4 + 0] = r;
		pixels[i * 4 + 1] = g;
		pixels[i * 4 + 2] = b;
		pixels[i * 4 + 3] = a;
	}
}

//
// PortalTexture
//
PortalTexture::PortalTexture(int tex, bool aquarium)
:	super(tex >= 0 ? tex : Tile::portalTile->tex),
	_tick(0)
{
	for (int i = 0; i < 32; i++) {
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 16; y++) {
				// Two spirals wound in opposite directions, wrapped so the tile
				// repeats without a seam, and brightened with a little noise.
				float br = 0;
				for (int ii = 0; ii < 2; ii++) {
					float xo = ii * 8.0f;
					float yo = ii * 8.0f;
					float xd = (x - xo) / 16.0f * 2;
					float yd = (y - yo) / 16.0f * 2;
					if (xd < -1) xd += 2;
					if (xd >= 1) xd -= 2;
					if (yd < -1) yd += 2;
					if (yd >= 1) yd -= 2;
					float dd = xd * xd + yd * yd;
					float br2 = atan2f(yd, xd) + ((i / 32.0f * Mth::PI * 2) - dd * 10 + ii * 2) * (ii * 2 - 1);
					br2 = (Mth::sin(br2) + 1) / 2;
					br2 /= (dd + 1);
					br += br2 * 0.5f;
				}
				br += Mth::random() * 0.1f;
				if (br > 1) br = 1;

				// Violet: blue carries it, red follows, green stays low.
				int r = (int) (br * br * 120 + br * 50 + 60);
				int g = (int) (br * br * br * br * 90 + 10);
				int b = (int) (br * 100 + 155);
				int a = (int) (br * 100 + 140);
				if (aquarium) {
					// Cyan leaning green: green and blue carry it, red stays low.
					r = (int) (br * br * br * br * 90 + 10);
					g = (int) (br * 90 + 150);
					b = (int) (br * br * 80 + br * 40 + 120);
				}

				unsigned char* p = &frames[i][(x + y * 16) * 4];
				p[0] = (unsigned char) r;
				p[1] = (unsigned char) g;
				p[2] = (unsigned char) b;
				p[3] = (unsigned char) a;
			}
		}
	}
}

void PortalTexture::tick() {
	_tick++;
	memcpy(pixels, frames[_tick & 31], 16 * 16 * 4);
}

//
// WaterSideTexture
//
WaterSideTexture::WaterSideTexture()
:   super(Tile::water->tex + 1),
	_tick(0),
	_frame(0),
	_tickCount(0)
{
	replicate = 2;

	current = new float[16*16];
	next = new float[16*16];
	heat = new float[16*16];
	heata = new float[16*16];

	for (int i = 0; i < 256; ++i) {
		current[i] = 0;
		next[i] = 0;
		heat[i] = 0;
		heata[i] = 0;
	}
}

WaterSideTexture::~WaterSideTexture() {
	delete[] current;
	delete[] next;
	delete[] heat;
	delete[] heata;
}

void WaterSideTexture::tick() {
	++_tickCount;
	for (int x = 0; x < 16; x++)
		for (int y = 0; y < 16; y++) {
			float pow = 0;
			for (int xx = y - 2; xx <= y; xx++) {
				int xi = (x) & 15;
				int yi = (xx) & 15;
				pow += current[xi + yi * 16];
			}
			next[x + y * 16] = pow / 3.2f + heat[x + y * 16] * 0.8f;
		}

	for (int x = 0; x < 16; x++)
		for (int y = 0; y < 16; y++) {
			heat[x + y * 16] += heata[x + y * 16] * 0.05f;

			if (heat[x + y * 16] < 0) heat[x + y * 16] = 0;
			heata[x + y * 16] -= 0.3f;
			if (Mth::random() < 0.2) {
				heata[x + y * 16] = 0.5f;
			}
		}
	float* tmp = next;
	next = current;
	current = tmp;

	for (int i = 0; i < 256; i++) {
		float pow = current[(i - _tickCount * 16) & 255];
		if (pow > 1) pow = 1;
		if (pow < 0) pow = 0;

		float pp = pow * pow;

		int r = (int) (32 + pp * 32);
		int g = (int) (50 + pp * 64);
		int b = (int) (255);
		int a = (int) (146 + pp * 50);

		//if (anaglyph3d) {
		//	int rr = (r * 30 + g * 59 + b * 11) / 100;
		//	int gg = (r * 30 + g * 70) / (100);
		//	int bb = (r * 30 + b * 70) / (100);

		//	r = rr;
		//	g = gg;
		//	b = bb;
		//}

		pixels[i * 4 + 0] = r;
		pixels[i * 4 + 1] = g;
		pixels[i * 4 + 2] = b;
		pixels[i * 4 + 3] = a;
	}
}

//
// FireTexture
//
FireTexture::FireTexture(int tex, int frameOffset)
:	super(tex),
	_tick(0),
	_offset(frameOffset)
{
}

void FireTexture::tick() {
	_tick++;
	memcpy(pixels, FIRE_FRAMES[(_tick + _offset) % FIRE_FRAME_COUNT], 16 * 16 * 4);
}

KelpTexture::KelpTexture(int tex, bool tip)
:	super(tex),
	_tick(0),
	_tip(tip)
{
}

void KelpTexture::tick() {
	_tick++;
	const int frame = (_tick / 3) % KELP_FRAME_COUNT;
	memcpy(pixels, _tip ? KELP_TIP_FRAMES[frame] : KELP_STALK_FRAMES[frame], 16 * 16 * 4);
}
