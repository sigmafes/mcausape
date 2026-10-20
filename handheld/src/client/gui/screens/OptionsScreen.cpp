#include "OptionsScreen.h"

#include "StartMenuScreen.h"
#include "DialogDefinitions.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"

#include "../components/OptionsPane.h"
#include "../components/ImageButton.h"
#include "../components/OptionsGroup.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/Textures.h"
#include "../../renderer/gles.h"
#include "../../skin/PlayerSkins.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../platform/web/WebBridge.h"
#include "../../../util/Mth.h"

OptionsScreen::OptionsScreen(ScreenId returnTo)
: btnClose(NULL),
  bHeader(NULL),
  currentOptionPane(NULL),
  selectedCategory(0),
  returnTo(returnTo),
  bImportSkin(NULL),
  bResetSkin(NULL),
  nameFocused(false),
  awaitingName(false),
  ticks(0),
  nameX(0), nameY(0), nameW(0), nameH(0),
  skinY(0) {
}

OptionsScreen::~OptionsScreen() {
	if(btnClose != NULL) {
		delete btnClose;
		btnClose = NULL;
	}
	if(bHeader != NULL) {
		delete bHeader,
		bHeader = NULL;
	}
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		if(*it != NULL) {
			delete *it;
			*it = NULL;
		}
	}
	for(std::vector<OptionsPane*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		if(*it != NULL) {
			delete *it;
			*it = NULL;
		}
	}
	categoryButtons.clear();
	delete bImportSkin;
	delete bResetSkin;
}

void OptionsScreen::init() {
	bHeader = new Touch::THeader(0, "Options");
	btnClose = new ImageButton(1, "");
	ImageDef def;
	def.name = "gui/touchgui.png";
	def.width = 34;
	def.height = 26;

	def.setSrc(IntRectangle(150, 0, (int)def.width, (int)def.height));
	btnClose->setImageDef(def, true);

	// The original tabs were Login/Game/Controls/Graphics, of which only the
	// first had any content. Profile is who you are to other players -- the
	// name over your head and the skin they see -- so it comes first.
	categoryButtons.push_back(new Touch::TButton(2, "Profile"));
	categoryButtons.push_back(new Touch::TButton(3, "Controls"));
	categoryButtons.push_back(new Touch::TButton(4, "Graphics"));
	categoryButtons.push_back(new Touch::TButton(5, "Sound"));
	buttons.push_back(bHeader);
	buttons.push_back(btnClose);
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		buttons.push_back(*it);
		tabButtons.push_back(*it);
	}

	bImportSkin = new Touch::TButton(10, "Import Skin");
	bResetSkin = new Touch::TButton(11, "Reset Skin");
	buttons.push_back(bImportSkin);
	buttons.push_back(bResetSkin);

	nameDraft = minecraft->options.username;
	// Whatever the browser did while this screen was closed -- Escape leaves
	// fullscreen without the game hearing of it.
	minecraft->options.fullscreen = WebBridge::isFullscreen();

	generateOptionScreens();

}
void OptionsScreen::setupPositions() {
	int buttonHeight = btnClose->height;
	btnClose->x = width - btnClose->width;
	btnClose->y = 0;
	int offsetNum = 1;
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		(*it)->x = 0;
		(*it)->y = offsetNum * buttonHeight;
		(*it)->selected = false;
		offsetNum++;
	}
	bHeader->x = 0;
	bHeader->y = 0;
	bHeader->width = width - btnClose->width;
	bHeader->height = btnClose->height;
	for(std::vector<OptionsPane*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		if(categoryButtons.size() > 0 && categoryButtons[0] != NULL) {
			(*it)->x = categoryButtons[0]->width;
			(*it)->y = bHeader->height;
			(*it)->width = width - categoryButtons[0]->width;
			(*it)->setupPositions();
		}
	}
	layoutProfile();
	// Not always 0: this also runs when the window is resized, and that should
	// not throw the player back to the first tab.
	selectCategory(selectedCategory);
}

void OptionsScreen::layoutProfile() {
	const int paneX = categoryButtons[0]->width;
	const int paneW = width - paneX;
	nameX = paneX + 12;
	nameY = bHeader->height + 20;
	nameW = Mth::Min(paneW - 24, 180);
	nameH = 18;
	skinY = nameY + nameH + 14;

	const int buttonY = skinY + 12 + 32 + 8;
	const int buttonW = Mth::Min((paneW - 24 - 6) / 2, 88);
	bImportSkin->x = nameX;
	bResetSkin->x = nameX + buttonW + 6;
	bImportSkin->y = bResetSkin->y = buttonY;
	bImportSkin->width = bResetSkin->width = buttonW;
	bImportSkin->height = bResetSkin->height = 22;
}

void OptionsScreen::render( int xm, int ym, float a ) {
	renderBackground();
	super::render(xm, ym, a);
	int xmm = xm * width / minecraft->width;
	int ymm = ym * height / minecraft->height - 1;
	if(currentOptionPane != NULL)
		currentOptionPane->render(minecraft, xmm, ymm);
	if (profileShown())
		renderProfile();
}

void OptionsScreen::renderProfile() {
	Font* font = minecraft->font;
	const bool touch = minecraft->useTouchscreen();

	// The name box, drawn the way CreateWorldScreen draws its own.
	drawString(font, "Player name", nameX, nameY - 11, 0xffa0a0a0);
	const int frame = nameFocused ? 0xffffffff : 0xffa0a0a0;
	fill(nameX - 1, nameY - 1, nameX + nameW + 1, nameY + nameH + 1, frame);
	fill(nameX, nameY, nameX + nameW, nameY + nameH, 0xff000000);
	std::string shown = nameDraft;
	if (nameFocused && (ticks / 10) % 2 == 0) shown += "_";
	drawString(font, shown, nameX + 4, nameY + (nameH - 8) / 2, 0xffe0e0e0);
	if (!nameFocused)
		drawString(font, touch ? "Tap to change" : "Click to change",
		           nameX + nameW + 6, nameY + (nameH - 8) / 2, 0xff606060);

	drawString(font, "Skin", nameX, skinY, 0xffa0a0a0);
	renderSkinFace(nameX, skinY + 12, 32);

	const PlayerSkins::Skin* skin = PlayerSkins::get(PlayerSkins::localSkin);
	std::string what = "Default";
	if (skin) {
		what = (skin->h == 32) ? "Custom, 64x32" : "Custom, 64x64";
		if (skin->model == PlayerSkins::MODEL_SLIM) what += ", slim arms";
	}
	drawString(font, what, nameX + 40, skinY + 16, 0xffe0e0e0);
	if (WebBridge::supported())
		drawString(font, "PNG, 64x64 or 64x32", nameX + 40, skinY + 30, 0xff808080);
}

/// The front of the head with the hat over it -- enough to tell at a glance
/// which skin is in use, without drawing the whole player.
void OptionsScreen::renderSkinFace(int x, int y, int size) {
	const int id = PlayerSkins::localSkin;
	const PlayerSkins::Skin* skin = PlayerSkins::get(id);
	float texH = 32.0f;
	if (skin) {
		PlayerSkins::bind(minecraft->textures, id);
		texH = (float) skin->h;
	} else {
		minecraft->textures->loadAndBindTexture("mob/char.png");
	}

	glEnable2(GL_BLEND);
	glEnable2(GL_ALPHA_TEST);
	glColor4f2(1, 1, 1, 1);
	const float u = 1.0f / 64.0f, v = 1.0f / texH;
	const float x0 = (float) x, y0 = (float) y, x1 = (float) (x + size), y1 = (float) (y + size);
	Tesselator& t = Tesselator::instance;
	for (int layer = 0; layer < 2; ++layer) {
		const float su = (layer == 0) ? 8.0f : 40.0f;	// face, then hat
		t.begin();
		t.vertexUV(x0, y1, blitOffset, su * u,        16 * v);
		t.vertexUV(x1, y1, blitOffset, (su + 8) * u,  16 * v);
		t.vertexUV(x1, y0, blitOffset, (su + 8) * u,  8 * v);
		t.vertexUV(x0, y0, blitOffset, su * u,        8 * v);
		t.draw();
	}
	glDisable2(GL_ALPHA_TEST);
}

void OptionsScreen::removed()
{
}
void OptionsScreen::close() {
	commitName();
	minecraft->reloadOptions();
	minecraft->screenChooser.setScreen(returnTo);
}

void OptionsScreen::buttonClicked( Button* button ) {
	if(button == btnClose) {
		close();
	} else if (button == bImportSkin) {
		// The picker answers later, through mcpe_skin_set.
		WebBridge::pickSkin();
	} else if (button == bResetSkin) {
		WebBridge::resetSkin();
		minecraft->setLocalSkin(0, 0, 0);
	} else if(button->id > 1 && button->id < 7) {
		// This is a category button
		int categoryButton = button->id - categoryButtons[0]->id;
		selectCategory(categoryButton);
	}
}

void OptionsScreen::selectCategory( int index ) {
	selectedCategory = index;
	if (!profileShown() && nameFocused) {
		commitName();
		nameFocused = false;
	}
	// Hidden on every other tab, and everywhere off the web: there is no file
	// picker to open without a browser around the game.
	const bool skinButtons = profileShown() && WebBridge::supported();
	bImportSkin->visible = bImportSkin->active = skinButtons;
	bResetSkin->visible = bResetSkin->active = skinButtons;

	int currentIndex = 0;
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		if(index == currentIndex) {
			(*it)->selected = true;
		} else {
			(*it)->selected = false;
		}
		currentIndex++;
	}
	if(index < (int)optionPanes.size())
		currentOptionPane = optionPanes[index];
}

void OptionsScreen::generateOptionScreens() {
	// One pane per category button, in the same order. Profile's stays empty;
	// see renderProfile.
	for (int i = 0; i < (int)categoryButtons.size(); ++i)
		optionPanes.push_back(new OptionsPane());
	OptionsPane** panes = &optionPanes[1];

	// Controls. Which ones are worth showing depends on what the player is
	// holding: a phone has no mouse to invert, and a desktop browser has no
	// touch pad to split off.
	if (minecraft->useTouchscreen()) {
		panes[0]->createOptionsGroup("options.group.touch")
			.addOptionItem(&Options::Option::SENSITIVITY, minecraft);
	} else {
		panes[0]->createOptionsGroup("options.group.mouse")
			.addOptionItem(&Options::Option::SENSITIVITY, minecraft)
			.addOptionItem(&Options::Option::INVERT_MOUSE, minecraft);
	}

	// Graphics. Render distance matters most here -- this is a browser game
	// running a fixed-function renderer, so being able to trade view range for
	// frame rate is the difference between playable and not on a weak machine.
	OptionsGroup& graphics = panes[1]->createOptionsGroup("options.group.graphics")
		.addOptionItem(&Options::Option::RENDER_DISTANCE, minecraft)
		.addOptionItem(&Options::Option::GRAPHICS, minecraft)
		.addOptionItem(&Options::Option::AMBIENT_OCCLUSION, minecraft);
	// The browser's fullscreen, where the page used to have a button for it.
	// Not offered where there is none to have: an iPhone gives fullscreen to
	// <video> only, and off the web there is no page to fill.
	if (WebBridge::fullscreenSupported())
		graphics.addOptionItem(&Options::Option::FULLSCREEN, minecraft);
	panes[1]->createOptionsGroup("options.group.camera")
		.addOptionItem(&Options::Option::VIEW_BOBBING, minecraft)
		.addOptionItem(&Options::Option::THIRD_PERSON, minecraft);

	// Sound. Music is deliberately absent: the streamed music assets are not
	// in the compiled-in sound bank, so a music slider would control nothing.
	panes[2]->createOptionsGroup("options.group.volume")
		.addOptionItem(&Options::Option::SOUND, minecraft);
}

void OptionsScreen::mouseClicked( int x, int y, int buttonNum ) {
	if(currentOptionPane != NULL)
		currentOptionPane->mouseClicked(minecraft, x, y, buttonNum);
	super::mouseClicked(x, y, buttonNum);

	if (!profileShown() || buttonNum != MouseAction::ACTION_LEFT)
		return;
	const bool inName = x >= nameX && x < nameX + nameW && y >= nameY && y < nameY + nameH;
	if (!inName) {
		if (nameFocused) commitName();
		nameFocused = false;
		return;
	}
	if (minecraft->useTouchscreen()) {
		// No keyboard to type into a box the game draws; borrow the page's.
		if (!awaitingName) {
			awaitingName = true;
			minecraft->platform()->createUserInput(DialogDefinitions::DIALOG_PLAYER_NAME, nameDraft);
		}
	} else {
		nameFocused = true;
	}
}

void OptionsScreen::commitName() {
	const std::string name = Options::sanitizeUsername(nameDraft);
	nameDraft = name;
	if (name == minecraft->options.username)
		return;
	minecraft->options.username = name;
	minecraft->options.save();
}

void OptionsScreen::keyPressed(int eventKey) {
	// Not Screen's Escape, which drops every screen and goes back to a game
	// that may not exist -- and would skip saving what was set here.
	if (eventKey == Keyboard::KEY_ESCAPE) {
		close();
		return;
	}
	if (nameFocused) {
		if (eventKey == Keyboard::KEY_BACKSPACE && !nameDraft.empty())
			nameDraft.erase(nameDraft.size() - 1);
		if (eventKey == Keyboard::KEY_RETURN) {
			commitName();
			nameFocused = false;
		}
		// Typing goes to the box, not to the tab-walking keys.
		return;
	}
	super::keyPressed(eventKey);
}

void OptionsScreen::keyboardNewChar(char inputChar) {
	if (!nameFocused || inputChar < 32 || inputChar > 126)
		return;
	if ((int) nameDraft.size() < MaxNameLength)
		nameDraft += inputChar;
}

bool OptionsScreen::handleBackEvent(bool isDown) {
	if (!isDown)
		close();
	return true;
}

void OptionsScreen::mouseReleased( int x, int y, int buttonNum ) {
	if(currentOptionPane != NULL)
		currentOptionPane->mouseReleased(minecraft, x, y, buttonNum);
	super::mouseReleased(x, y, buttonNum);
}

void OptionsScreen::tick() {
	++ticks;
	if (awaitingName) {
		// -2 is "still open"; see CreateWorldScreen::tick.
		const int status = minecraft->platform()->getUserInputStatus();
		if (status > -1) {
			awaitingName = false;
			if (status == 1) {
				StringVector sv = minecraft->platform()->getUserInput();
				if (!sv.empty()) {
					nameDraft = sv[0];
					commitName();
				}
			}
		}
	}
	if(currentOptionPane != NULL)
		currentOptionPane->tick(minecraft);
	super::tick();
}
