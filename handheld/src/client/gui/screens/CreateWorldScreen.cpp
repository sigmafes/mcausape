#include "CreateWorldScreen.h"
#include "DialogDefinitions.h"
#include "ProgressScreen.h"
#include "ScreenChooser.h"
#include "../Font.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../platform/time.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../util/StringUtils.h"
#include "../../../world/Difficulty.h"
#include "../../../world/level/LevelSettings.h"
#include "../../../world/level/storage/LevelStorageSource.h"

#include <cstdio>
#include <set>

static const int MaxNameLength = 32;

static const char ILLEGAL_FILE_CHARACTERS[] = {
	'/', '\n', '\r', '\t', '\0', '\f', '`', '?', '*', '\\', '<', '>', '|', '\"', ':'
};

CreateWorldScreen::CreateWorldScreen()
:	touchInput(false),
	bMode(NULL), bDifficulty(NULL), bCheats(NULL), bDebug(NULL), bCreate(NULL), bCancel(NULL),
	name("New World"),
	creative(false),
	peaceful(false),
	cheats(false),
	debug(false),
	activeField(FIELD_NAME),
	awaitingInput(false),
	pendingField(FIELD_NAME),
	ticks(0),
	hasStarted(false),
	boxX(0), boxY(0), boxW(0), boxH(0), seedBoxY(0),
	descY1(0), descY2(0)
{
}

CreateWorldScreen::~CreateWorldScreen()
{
	delete bMode;
	delete bDifficulty;
	delete bCheats;
	delete bDebug;
	delete bCreate;
	delete bCancel;
}

void CreateWorldScreen::init()
{
	touchInput = minecraft->useTouchscreen();

	// Ids already taken, so the folder this world gets is a new one.
	LevelSummaryList levels;
	minecraft->getLevelSource()->getLevelList(levels);
	for (unsigned int i = 0; i < levels.size(); ++i)
		existingIds.push_back(levels[i].id);

	if (touchInput) {
		bMode       = new Touch::TButton(1, "");
		bDifficulty = new Touch::TButton(2, "");
		bCheats     = new Touch::TButton(3, "");
		bDebug      = new Touch::TButton(4, "");
		bCreate     = new Touch::TButton(5, "Create World");
		bCancel     = new Touch::TButton(6, "Cancel");
	} else {
		bMode       = new Button(1, "");
		bDifficulty = new Button(2, "");
		bCheats     = new Button(3, "");
		bDebug      = new Button(4, "");
		bCreate     = new Button(5, "Create World");
		bCancel     = new Button(6, "Cancel");
	}

	buttons.push_back(bMode);
	buttons.push_back(bDifficulty);
	buttons.push_back(bCheats);
	buttons.push_back(bDebug);
	buttons.push_back(bCreate);
	buttons.push_back(bCancel);

	tabButtons.push_back(bMode);
	tabButtons.push_back(bDifficulty);
	tabButtons.push_back(bCheats);
	tabButtons.push_back(bDebug);
	tabButtons.push_back(bCreate);
	tabButtons.push_back(bCancel);

	refreshLabels();
}

void CreateWorldScreen::refreshLabels()
{
	bMode->msg       = std::string("Game Mode: ") + (creative? "Creative" : "Survival");
	bDifficulty->msg = std::string("Difficulty: ") + (peaceful? "Peaceful" : "Normal");
	bCheats->msg     = std::string("Allow Cheats: ") + (cheats? "ON" : "OFF");
	bDebug->msg      = std::string("Debug Mode: ") + (debug? "ON" : "OFF");
	bCreate->active  = !Util::stringTrim(name).empty();
}

void CreateWorldScreen::setupPositions()
{
	const int gap = 8;
	int colW = (width - 2 * gap - 24) / 2;
	if (colW > 150) colW = 150;
	const int left  = width / 2 - gap / 2 - colW;
	const int right = width / 2 + gap / 2;

	boxW = colW * 2 + gap;
	// The menu is laid out in GUI units, and a 854x480 window is only about 295x165
	// of them, so everything here is packed to fit that.
	boxH = 16;
	boxX = left;
	boxY = 24;
	seedBoxY = boxY + boxH + 16;

	const int rowH = 18;
	const int row1 = seedBoxY + boxH + 16;
	const int row2 = row1 + rowH + 14;
	int rowCreate = row2 + rowH + 16;
	if (rowCreate > height - 24) rowCreate = height - 24;
	descY1 = row1 + rowH + 2;
	descY2 = row2 + rowH + 2;

	bMode->x = left;        bMode->y = row1;
	bDifficulty->x = right; bDifficulty->y = row1;
	bCheats->x = left;      bCheats->y = row2;
	bDebug->x = right;      bDebug->y = row2;
	bCreate->x = left;      bCreate->y = rowCreate;
	bCancel->x = right;     bCancel->y = rowCreate;

	Button* all[] = { bMode, bDifficulty, bCheats, bDebug, bCreate, bCancel };
	for (int i = 0; i < 6; ++i) {
		all[i]->width = colW;
		all[i]->height = rowH;
	}
}

void CreateWorldScreen::tick()
{
	++ticks;

	if (!awaitingInput)
		return;

	// -2 (USERINPUT_NOTINITED) means still waiting; the concrete constant lives
	// in a platform-specific header this shared screen has no business
	// including, so this matches it the same way the rest of the game's UI
	// code already does.
	const int status = minecraft->platform()->getUserInputStatus();
	if (status <= -1)
		return;

	if (status == 1) {
		StringVector sv = minecraft->platform()->getUserInput();
		if (!sv.empty()) {
			if (pendingField == FIELD_NAME)
				name = sv[0];
			else
				seed = sv[0];
		}
	}
	awaitingInput = false;
	refreshLabels();
}

std::string CreateWorldScreen::makeLevelId() const
{
	// Trim, drop what a folder name cannot hold, then add '-' until it is new.
	std::string id = Util::stringTrim(name);
	for (unsigned int i = 0; i < sizeof(ILLEGAL_FILE_CHARACTERS) / sizeof(char); ++i)
		id = Util::stringReplace(id, std::string(1, ILLEGAL_FILE_CHARACTERS[i]), "");
	if (id.empty())
		id = "no_name";

	std::set<std::string> taken(existingIds.begin(), existingIds.end());
	while (taken.find(id) != taken.end())
		id += "-";
	return id;
}

int CreateWorldScreen::resolveSeed() const
{
	std::string seedString = Util::stringTrim(seed);
	if (seedString.empty())
		return getEpochTimeS();

	int value;
	if (sscanf(seedString.c_str(), "%d", &value) > 0)
		return value;
	return Util::hashCode(seedString);
}

void CreateWorldScreen::createWorld()
{
	if (hasStarted)
		return;
	const std::string levelName = Util::stringTrim(name);
	if (levelName.empty())
		return;

	const std::string levelId = makeLevelId();
	LevelSettings settings(resolveSeed(), creative? GameType::Creative : GameType::Survival,
						   peaceful? Difficulty::PEACEFUL : Difficulty::NORMAL, cheats, debug);

	minecraft->selectLevel(levelId, levelName, settings);
	minecraft->hostMultiplayer();
	minecraft->setScreen(new ProgressScreen());
	hasStarted = true;
}

void CreateWorldScreen::buttonClicked(Button* button)
{
	if (hasStarted)
		return;

	if (button == bMode)       creative = !creative;
	if (button == bDifficulty) peaceful = !peaceful;
	if (button == bCheats)     cheats = !cheats;
	if (button == bDebug)      debug = !debug;
	refreshLabels();

	if (button == bCreate)
		createWorld();
	if (button == bCancel)
		minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
}

void CreateWorldScreen::mouseClicked(int x, int y, int buttonNum)
{
	Screen::mouseClicked(x, y, buttonNum);

	if (hasStarted || buttonNum != MouseAction::ACTION_LEFT)
		return;

	const bool inName = x >= boxX && x < boxX + boxW && y >= boxY && y < boxY + boxH;
	const bool inSeed = x >= boxX && x < boxX + boxW && y >= seedBoxY && y < seedBoxY + boxH;
	if (!inName && !inSeed)
		return;

	activeField = inName? FIELD_NAME : FIELD_SEED;

	if (touchInput && !awaitingInput) {
		pendingField = activeField;
		awaitingInput = true;
		const std::string current = (pendingField == FIELD_NAME)? name : seed;
		minecraft->platform()->createUserInput(
			pendingField == FIELD_NAME? DialogDefinitions::DIALOG_CREATE_WORLD_NAME
			                          : DialogDefinitions::DIALOG_CREATE_WORLD_SEED,
			current);
	}
}

bool CreateWorldScreen::handleBackEvent(bool isDown)
{
	if (!isDown)
		minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
	return true;
}

void CreateWorldScreen::keyPressed(int eventKey)
{
	// Not Screen::keyPressed: its Escape closes every screen to the game, and
	// there is no game behind this one.
	if (eventKey == Keyboard::KEY_ESCAPE) {
		minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
		return;
	}

	std::string& field = (activeField == FIELD_NAME)? name : seed;

	if (eventKey == Keyboard::KEY_BACKSPACE) {
		if (!field.empty()) field.erase(field.size() - 1);
		refreshLabels();
		return;
	}
	// No Tab-switches-field: a browser tab takes Tab for its own focus
	// navigation before a canvas ever sees it, so a click on the other box
	// (see mouseClicked) is the only reliable way to move between them.
	if (eventKey == Keyboard::KEY_RETURN)
		createWorld();
}

void CreateWorldScreen::keyboardNewChar(char inputChar)
{
	if (touchInput)
		return;
	if (inputChar < 32 || inputChar > 126)
		return;

	std::string& field = (activeField == FIELD_NAME)? name : seed;
	if ((int)field.size() >= MaxNameLength)
		return;
	field += inputChar;
	refreshLabels();
}

void CreateWorldScreen::render(int xm, int ym, float a)
{
	renderDirtBackground(0);
	glEnable2(GL_BLEND);

	drawCenteredString(minecraft->font, "Create New World", width / 2, 8, 0xffffffff);

	// The name box and the seed box: a light frame round a black field, with a
	// blinking cursor on whichever one has focus.
	drawString(minecraft->font, "World Name", boxX, boxY - 11, 0xffa0a0a0);
	fill(boxX - 1, boxY - 1, boxX + boxW + 1, boxY + boxH + 1, 0xffa0a0a0);
	fill(boxX, boxY, boxX + boxW, boxY + boxH, 0xff000000);
	std::string shownName = name;
	if (activeField == FIELD_NAME && (ticks / 10) % 2 == 0) shownName += "_";
	drawString(minecraft->font, shownName, boxX + 4, boxY + (boxH - 8) / 2, 0xffe0e0e0);

	drawString(minecraft->font, "Seed (blank for random)", boxX, seedBoxY - 11, 0xffa0a0a0);
	fill(boxX - 1, seedBoxY - 1, boxX + boxW + 1, seedBoxY + boxH + 1, 0xffa0a0a0);
	fill(boxX, seedBoxY, boxX + boxW, seedBoxY + boxH, 0xff000000);
	std::string shownSeed = seed;
	if (activeField == FIELD_SEED && (ticks / 10) % 2 == 0) shownSeed += "_";
	drawString(minecraft->font, shownSeed, boxX + 4, seedBoxY + (boxH - 8) / 2, 0xffe0e0e0);

	drawString(minecraft->font, "Will be saved in: " + makeLevelId(), boxX, seedBoxY + boxH + 3, 0xff808080);

	// What each choice means, under the button that makes it.
	const int grey = 0xffa0a0a0;
	drawString(minecraft->font, creative? "Unlimited items, flying" : "Mobs, health, resources", bMode->x, descY1, grey);
	drawString(minecraft->font, peaceful? "No hostile mobs" : "Hostile mobs attack you", bDifficulty->x, descY1, grey);
	drawString(minecraft->font, cheats? "Commands: /time, /give" : "Commands are disabled", bCheats->x, descY2, grey);
	drawString(minecraft->font, debug? "Developer keybinds on" : "Developer keybinds off", bDebug->x, descY2, grey);

	Screen::render(xm, ym, a);
	glDisable2(GL_BLEND);
}
