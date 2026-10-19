#include "CreateWorldScreen.h"
#include "ProgressScreen.h"
#include "ScreenChooser.h"
#include "../Font.h"
#include "../../Minecraft.h"
#include "../../../platform/time.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../util/StringUtils.h"
#include "../../../world/Difficulty.h"
#include "../../../world/level/LevelSettings.h"
#include "../../../world/level/storage/LevelStorageSource.h"

#include <set>

static const int MaxNameLength = 32;

static const char ILLEGAL_FILE_CHARACTERS[] = {
	'/', '\n', '\r', '\t', '\0', '\f', '`', '?', '*', '\\', '<', '>', '|', '\"', ':'
};

CreateWorldScreen::CreateWorldScreen()
:	bMode      (1, ""),
	bDifficulty(2, ""),
	bCheats    (3, ""),
	bDebug     (4, ""),
	bCreate    (5, "Create World"),
	bCancel    (6, "Cancel"),
	name("New World"),
	creative(false),
	peaceful(false),
	cheats(false),
	debug(false),
	ticks(0),
	hasStarted(false),
	boxX(0), boxY(0), boxW(0), boxH(0),
	descY1(0), descY2(0)
{
}

CreateWorldScreen::~CreateWorldScreen()
{
}

void CreateWorldScreen::init()
{
	// Ids already taken, so the folder this world gets is a new one.
	LevelSummaryList levels;
	minecraft->getLevelSource()->getLevelList(levels);
	for (unsigned int i = 0; i < levels.size(); ++i)
		existingIds.push_back(levels[i].id);

	buttons.push_back(&bMode);
	buttons.push_back(&bDifficulty);
	buttons.push_back(&bCheats);
	buttons.push_back(&bDebug);
	buttons.push_back(&bCreate);
	buttons.push_back(&bCancel);

	refreshLabels();
}

void CreateWorldScreen::refreshLabels()
{
	bMode.msg       = std::string("Game Mode: ") + (creative? "Creative" : "Survival");
	bDifficulty.msg = std::string("Difficulty: ") + (peaceful? "Peaceful" : "Normal");
	bCheats.msg     = std::string("Allow Cheats: ") + (cheats? "ON" : "OFF");
	bDebug.msg      = std::string("Debug Mode: ") + (debug? "ON" : "OFF");
	bCreate.active  = !Util::stringTrim(name).empty();
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
	boxY = 28;

	const int rowH = 18;
	const int row1 = boxY + boxH + 16;
	const int row2 = row1 + rowH + 14;
	int rowCreate = row2 + rowH + 16;
	if (rowCreate > height - 24) rowCreate = height - 24;
	descY1 = row1 + rowH + 2;
	descY2 = row2 + rowH + 2;

	bMode.x = left;        bMode.y = row1;
	bDifficulty.x = right; bDifficulty.y = row1;
	bCheats.x = left;      bCheats.y = row2;
	bDebug.x = right;      bDebug.y = row2;
	bCreate.x = left;      bCreate.y = rowCreate;
	bCancel.x = right;     bCancel.y = rowCreate;

	Button* all[] = { &bMode, &bDifficulty, &bCheats, &bDebug, &bCreate, &bCancel };
	for (int i = 0; i < 6; ++i) {
		all[i]->width = colW;
		all[i]->height = rowH;
	}
}

void CreateWorldScreen::tick()
{
	++ticks;
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

void CreateWorldScreen::createWorld()
{
	if (hasStarted)
		return;
	const std::string levelName = Util::stringTrim(name);
	if (levelName.empty())
		return;

	const std::string levelId = makeLevelId();
	LevelSettings settings(getEpochTimeS(), creative? GameType::Creative : GameType::Survival,
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

	if (button == &bMode)       creative = !creative;
	if (button == &bDifficulty) peaceful = !peaceful;
	if (button == &bCheats)     cheats = !cheats;
	if (button == &bDebug)      debug = !debug;
	refreshLabels();

	if (button == &bCreate)
		createWorld();
	if (button == &bCancel)
		minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
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
	if (eventKey == Keyboard::KEY_BACKSPACE) {
		if (!name.empty()) name.erase(name.size() - 1);
		refreshLabels();
		return;
	}
	if (eventKey == Keyboard::KEY_RETURN)
		createWorld();
}

void CreateWorldScreen::keyboardNewChar(char inputChar)
{
	if (inputChar < 32 || inputChar > 126)
		return;
	if ((int)name.size() >= MaxNameLength)
		return;
	name += inputChar;
	refreshLabels();
}

void CreateWorldScreen::render(int xm, int ym, float a)
{
	renderDirtBackground(0);
	glEnable2(GL_BLEND);

	drawCenteredString(minecraft->font, "Create New World", width / 2, 8, 0xffffffff);
	drawString(minecraft->font, "World Name", boxX, boxY - 11, 0xffa0a0a0);

	// The name box: a light frame round a black field, with a blinking cursor.
	fill(boxX - 1, boxY - 1, boxX + boxW + 1, boxY + boxH + 1, 0xffa0a0a0);
	fill(boxX, boxY, boxX + boxW, boxY + boxH, 0xff000000);
	std::string shown = name;
	if ((ticks / 10) % 2 == 0) shown += "_";
	drawString(minecraft->font, shown, boxX + 4, boxY + (boxH - 8) / 2, 0xffe0e0e0);
	drawString(minecraft->font, "Will be saved in: " + makeLevelId(), boxX, boxY + boxH + 3, 0xff808080);

	// What each choice means, under the button that makes it.
	const int grey = 0xffa0a0a0;
	drawString(minecraft->font, creative? "Unlimited items, flying" : "Mobs, health, resources", bMode.x, descY1, grey);
	drawString(minecraft->font, peaceful? "No hostile mobs" : "Hostile mobs attack you", bDifficulty.x, descY1, grey);
	drawString(minecraft->font, cheats? "Commands: /time, /give" : "Commands are disabled", bCheats.x, descY2, grey);
	drawString(minecraft->font, debug? "Developer keybinds on" : "Developer keybinds off", bDebug.x, descY2, grey);

	Screen::render(xm, ym, a);
	glDisable2(GL_BLEND);
}
