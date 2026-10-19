#include "Screen.h"
#include "components/Button.h"
#include "components/TextBox.h"
#include "../Minecraft.h"
#include "../renderer/Tesselator.h"
#include "../sound/SoundEngine.h"
#include "../../platform/input/Keyboard.h"
#include "../../platform/input/Mouse.h"
#include "../renderer/Textures.h"
#include "../renderer/gles.h"
#include "../../platform/time.h"
#include "../../util/Mth.h"

Screen::Screen()
:   passEvents(false),
	clickedButton(NULL),
	tabButtonIndex(0),
	width(1),
	height(1),
	minecraft(NULL),
	font(NULL)
{
}

void Screen::render( int xm, int ym, float a )
{
	for (unsigned int i = 0; i < buttons.size(); i++) {
		Button* button = buttons[i];
		button->render(minecraft, xm, ym);
	}
}

void Screen::init( Minecraft* minecraft, int width, int height )
{
	//particles = /*new*/ GuiParticles(minecraft);
	this->minecraft = minecraft;
	this->font = minecraft->font;
	this->width = width;
	this->height = height;
	init();
	setupPositions();
	updateTabButtonSelection();
}

void Screen::init()
{
}

void Screen::setSize( int width, int height )
{
	this->width = width;
	this->height = height;
	setupPositions();
}

bool Screen::handleBackEvent( bool isDown )
{
	return false;
}

void Screen::updateEvents()
{
	if (passEvents)
		return;

	while (Mouse::next())
		mouseEvent();

	while (Keyboard::next())
		keyboardEvent();
	while (Keyboard::nextTextChar())
		keyboardTextEvent();
}

void Screen::mouseEvent()
{
	const MouseAction& e = Mouse::getEvent();
	if (e.action == MouseAction::ACTION_WHEEL) {
		mouseWheel(e.dy);
		return;
	}
	if (!e.isButton())
		return;

	if (Mouse::getEventButtonState()) {
		int xm = e.x * width / minecraft->width;
		int ym = e.y * height / minecraft->height - 1;
		mouseClicked(xm, ym, Mouse::getEventButton());
	} else {
		int xm = e.x * width / minecraft->width;
		int ym = e.y * height / minecraft->height - 1;
		mouseReleased(xm, ym, Mouse::getEventButton());
	}
}

void Screen::keyboardEvent()
{
	if (Keyboard::getEventKeyState()) {
		//if (Keyboard.getEventKey() == Keyboard.KEY_F11) {
		//    minecraft->toggleFullScreen();
		//    return;
		//}
		keyPressed(Keyboard::getEventKey());
	}
}
void Screen::keyboardTextEvent()
{
	keyboardNewChar(Keyboard::getChar());
}
void Screen::renderBackground()
{
	renderBackground(0);
}

void Screen::renderBackground( int vo )
{
	if (minecraft->isLevelGenerated()) {
		fillGradient(0, 0, width, height, 0xc0101010, 0xd0101010);
	} else {
		renderDirtBackground(vo);
	}
}

// The backdrop of the menus outside the game: the six faces of a cube (Java
// Edition's panorama_0..5, in data/images/custom) seen from the inside while the
// camera turns slowly, as on that game's title screen, and out of focus.
//
// The faces go front, right, back, left, up, down; each quad below lists its
// corners with the texture coordinate they carry (x, y, z, u, v), laid out so
// neighbouring edges meet.
//
// The blur is a trick of resolution: the cube is drawn into a small corner of the
// frame buffer (BlurSize x BlurSize), copied into a texture of that size, and that
// texture is stretched over the whole screen with linear filtering. Every texel
// then covers many pixels and the result is smooth. No render target is needed.
//
// Returns false if any face is missing, so the caller can fall back to dirt.
static const int BlurSize = 128;

static bool renderPanorama(Minecraft* mc, int guiWidth, int guiHeight) {
	static const char* const FaceNames[6] = {
		"custom/Panorama_0_JE1.png", "custom/Panorama_1_JE1.png", "custom/Panorama_2_JE1.png",
		"custom/Panorama_3_JE1.png", "custom/Panorama_4_JE1.png", "custom/Panorama_5_JE1.png"
	};
	static const float Quads[6][4][5] = {
		// 0 front (z = -1)
		{ {-1,  1, -1, 0, 0}, {-1, -1, -1, 0, 1}, { 1, -1, -1, 1, 1}, { 1,  1, -1, 1, 0} },
		// 1 right (x = +1)
		{ { 1,  1, -1, 0, 0}, { 1, -1, -1, 0, 1}, { 1, -1,  1, 1, 1}, { 1,  1,  1, 1, 0} },
		// 2 back (z = +1)
		{ { 1,  1,  1, 0, 0}, { 1, -1,  1, 0, 1}, {-1, -1,  1, 1, 1}, {-1,  1,  1, 1, 0} },
		// 3 left (x = -1)
		{ {-1,  1,  1, 0, 0}, {-1, -1,  1, 0, 1}, {-1, -1, -1, 1, 1}, {-1,  1, -1, 1, 0} },
		// 4 up (y = +1), its bottom edge against the front face
		{ {-1,  1,  1, 0, 0}, {-1,  1, -1, 0, 1}, { 1,  1, -1, 1, 1}, { 1,  1,  1, 1, 0} },
		// 5 down (y = -1), its top edge against the front face
		{ {-1, -1, -1, 0, 0}, {-1, -1,  1, 0, 1}, { 1, -1,  1, 1, 1}, { 1, -1, -1, 1, 0} }
	};

	// A face has to load before anything is drawn, or half a cube would show.
	for (int i = 0; i < 6; ++i)
		if (mc->textures->loadTexture(FaceNames[i]) == Textures::InvalidId)
			return false;

	// The texture the small picture is copied into. Textures makes it (so its own
	// record of what is bound stays right); it is linear and clamped, and blank
	// until the first copy. Made again if the Textures object was replaced.
	static TextureId blurTex = Textures::InvalidId;
	static Textures* blurOwner = NULL;
	static bool facesFiltered = false;
	if (blurOwner != mc->textures || blurTex == Textures::InvalidId) {
		static unsigned char blank[BlurSize * BlurSize * 3];
		TextureData d;
		d.w = BlurSize;
		d.h = BlurSize;
		d.data = blank;
		d.numBytes = sizeof(blank);
		d.transparent = false;
		d.memoryHandledExternally = true;
		blurTex = mc->textures->assignTexture("##panorama_blur", d);
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		blurOwner = mc->textures;
		facesFiltered = false;
	}
	if (!facesFiltered) {
		// The faces load with nearest filtering. Drawn at half their size that
		// would shimmer as the camera turns, so smooth them.
		for (int i = 0; i < 6; ++i) {
			mc->textures->loadAndBindTexture(FaceNames[i]);
			glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		facesFiltered = true;
	}

	// Java turns it 0.1 degrees a tick (2 a second) and nods between 20 degrees
	// below the horizon and 25 either side of that.
	const float time  = getTimeS();
	const float yaw   = time * 2.0f;
	const float pitch = Mth::sin(time / 20.0f) * 25.0f + 20.0f;

	glDisable2(GL_FOG);
	glDisable2(GL_DEPTH_TEST);
	glDisable2(GL_CULL_FACE);
	glDisable2(GL_BLEND);
	const bool scissored = glIsEnabled(GL_SCISSOR_TEST) != 0;
	if (scissored) glDisable2(GL_SCISSOR_TEST);
	glColor4f2(1, 1, 1, 1);

	Tesselator& t = Tesselator::instance;

	// 1) The cube, small, in the corner of the frame buffer. The aspect ratio is the
	// screen's, not the square's: the stretch back to the screen undoes it.
	glViewport(0, 0, BlurSize, BlurSize);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(85.0f, (float) mc->width / (float) mc->height, 0.05f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glRotatef(pitch, 1, 0, 0);
	glRotatef(yaw, 0, 1, 0);
	for (int i = 0; i < 6; ++i) {
		mc->textures->loadAndBindTexture(FaceNames[i]);
		t.begin();
		t.color(0xffffff);
		for (int v = 0; v < 4; ++v) {
			const float* q = Quads[i][v];
			t.vertexUV(q[0], q[1], q[2], q[3], q[4]);
		}
		t.draw();
	}
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	// 2) Into the texture.
	mc->textures->bind(blurTex);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, BlurSize, BlurSize);

	// 3) Back to the whole frame, and the texture over all of it. Four draws, each
	// moved by half a texel and averaged, take the diamond pattern out of the
	// linear filter. The picture came from a bottom-up frame buffer, so v is flipped.
	glViewport(0, 0, mc->width, mc->height);
	if (scissored) glEnable2(GL_SCISSOR_TEST);
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	const float px = 0.5f / BlurSize;
	for (int pass = 0; pass < 4; ++pass) {
		const float du = (pass & 1) ? px : -px;
		const float dv = (pass & 2) ? px : -px;
		const int alpha = (int) (255.0f / (pass + 1) + 0.5f);
		mc->textures->bind(blurTex);
		t.begin();
		t.color(255, 255, 255, alpha);
		t.vertexUV(0, (float)guiHeight, 0, 0 + du, 0 + dv);
		t.vertexUV((float)guiWidth, (float)guiHeight, 0, 1 + du, 0 + dv);
		t.vertexUV((float)guiWidth, 0, 0, 1 + du, 1 + dv);
		t.vertexUV(0, 0, 0, 0 + du, 1 + dv);
		t.draw();
	}
	glDisable2(GL_BLEND);
	return true;
}
void Screen::renderDirtBackground( int vo )
{
	// The menus outside the game get the rotating panorama; dirt is the fallback.
	if (renderPanorama(minecraft, width, height)) return;

	//glDisable2(GL_LIGHTING);
	glDisable2(GL_FOG);
	Tesselator& t = Tesselator::instance;
	minecraft->textures->loadAndBindTexture("gui/background.png");
	glColor4f2(1, 1, 1, 1);
	float s = 32;
	float fvo = (float) vo;
	t.begin();
	t.color(0x404040);
	t.vertexUV(0, (float)height, 0, 0, height / s + fvo);
	t.vertexUV((float)width, (float)height, 0, width / s, (float)height / s + fvo);
	t.vertexUV((float)width, 0, 0, (float)width / s, 0 + fvo);
	t.vertexUV(0, 0, 0, 0, 0 + fvo);
	t.draw();
}

bool Screen::isPauseScreen()
{
	return true;
}

bool Screen::isErrorScreen()
{
	return false;
}

bool Screen::isInGameScreen()
{
	return true;
}

bool Screen::closeOnPlayerHurt() {
    return false;
}

void Screen::keyPressed( int eventKey )
{
	if (eventKey == Keyboard::KEY_ESCAPE) {
		minecraft->setScreen(NULL);
		//minecraft->grabMouse();
	}
	if (minecraft->useTouchscreen())
		return;

	// "Tabbing" the buttons (walking with keys)
	const int tabButtonCount = tabButtons.size();
	if (!tabButtonCount)
		return;

	Options& o = minecraft->options;
	if (eventKey == o.keyMenuNext.key)
		if (++tabButtonIndex == tabButtonCount) tabButtonIndex = 0;
	if (eventKey == o.keyMenuPrevious.key)
		if (--tabButtonIndex == -1) tabButtonIndex = tabButtonCount-1;
	if (eventKey == o.keyMenuOk.key) {
		Button* button = tabButtons[tabButtonIndex];
		if (button->active) {
			minecraft->soundEngine->playUI("random.click", 1, 1);
			buttonClicked(button);
		}
	}

	updateTabButtonSelection();
}

void Screen::updateTabButtonSelection()
{
	if (minecraft->useTouchscreen())
		return;

	for (unsigned int i = 0; i < tabButtons.size(); ++i)
		tabButtons[i]->selected = (i == tabButtonIndex);
}

void Screen::mouseClicked( int x, int y, int buttonNum )
{
	if (buttonNum == MouseAction::ACTION_LEFT) {
		for (unsigned int i = 0; i < buttons.size(); ++i) {
			Button* button = buttons[i];
            //LOGI("Hit-testing button: %p\n", button);
			if (button->clicked(minecraft, x, y)) {
                button->setPressed();

                //LOGI("Hit-test successful: %p\n", button);
				clickedButton = button;
/*
#if !defined(ANDROID) && !defined(__APPLE__) //if (!minecraft->isTouchscreen()) {
					minecraft->soundEngine->playUI("random.click", 1, 1);
					buttonClicked(button);
#endif }
*/
			}
		}
	}
}

void Screen::mouseReleased( int x, int y, int buttonNum )
{
	//LOGI("b_id: %d, (%p), text: %s\n", buttonNum, clickedButton, clickedButton?clickedButton->msg.c_str():"<null>");
	if (!clickedButton || buttonNum != MouseAction::ACTION_LEFT) return;

#if 1
//#if defined(ANDROID) || defined(__APPLE__) //if (minecraft->isTouchscreen()) {
		for (unsigned int i = 0; i < buttons.size(); ++i) {
			Button* button = buttons[i];
			if (clickedButton == button && button->clicked(minecraft, x, y)) {
				buttonClicked(button);
				minecraft->soundEngine->playUI("random.click", 1, 1);
				clickedButton->released(x, y);
			}
		}
# else //	} else {
		clickedButton->released(x, y);
#endif // }
	clickedButton = NULL;
}

bool Screen::renderGameBehind() {
	return true;
}

bool Screen::hasClippingArea( IntRectangle& out )
{
	return false;
}

void Screen::lostFocus() {
	for(std::vector<TextBox*>::iterator it = textBoxes.begin(); it != textBoxes.end(); ++it) {
		TextBox* tb = *it;
		tb->loseFocus(minecraft);
	}
}

void Screen::toGUICoordinate( int& x, int& y ) {
	x = x * width / minecraft->width;
	y = y * height / minecraft->height - 1;
}
