#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__CreateWorldScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__CreateWorldScreen_H__

#include <string>
#include "../Screen.h"
#include "../components/Button.h"

// The Create New World screen: a name box, a seed box, four buttons that flip
// between two values each, and Create / Cancel. What it picks goes into the
// world's LevelSettings and from there into level.dat.
//
// Typing is the one thing that differs by platform. With a physical keyboard,
// the two boxes are drawn and filled in by this screen itself, the same as any
// other screen's keyPressed()/keyboardNewChar(). On touch there is no physical
// keyboard, so a tap on either box instead borrows the platform's own text
// dialog for that one field (see AppPlatform::createUserInput) -- the on-screen
// keyboard it brings up is not something a canvas-drawn box can summon on its
// own -- and the typed value comes back into the same field once the player is
// done. Everything else about the screen, including which field is which, is
// identical either way.
class CreateWorldScreen: public Screen
{
public:
	CreateWorldScreen();
	virtual ~CreateWorldScreen();

	virtual void init();
	virtual void setupPositions();
	virtual void tick();
	virtual void render(int xm, int ym, float a);
	virtual bool handleBackEvent(bool isDown);

protected:
	virtual void buttonClicked(Button* button);
	virtual void mouseClicked(int x, int y, int buttonNum);
	virtual void keyPressed(int eventKey);
	virtual void keyboardNewChar(char inputChar);

private:
	void createWorld();
	void refreshLabels();
	std::string makeLevelId() const;
	int resolveSeed() const;

	// True on a touch build: tapping a box opens the platform dialog for it
	// instead of taking the physical keyboard path.
	bool touchInput;

	Button* bMode;
	Button* bDifficulty;
	Button* bCheats;
	Button* bDebug;
	Button* bCreate;
	Button* bCancel;

	std::string name;
	std::string seed;
	bool creative;
	bool peaceful;
	bool cheats;
	bool debug;

	// Which box the physical keyboard types into (touch builds never use this --
	// they always know which box they just opened a platform dialog for).
	static const int FIELD_NAME = 0;
	static const int FIELD_SEED = 1;
	int activeField;

	// Set while a touch dialog for FIELD_NAME/FIELD_SEED is open, so tick() knows
	// which string to write the platform's answer into.
	bool awaitingInput;
	int pendingField;

	std::vector<std::string> existingIds;
	int ticks;
	bool hasStarted;

	// Where the boxes are, filled in by setupPositions().
	int boxX, boxY, boxW, boxH;
	int seedBoxY;
	int descY1, descY2;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__CreateWorldScreen_H__*/
