#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__CreateWorldScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__CreateWorldScreen_H__

#include <string>
#include "../Screen.h"
#include "../components/Button.h"

// The Create New World screen of the keyboard-and-mouse builds: a name box, four
// buttons that flip between two values each, and Create / Cancel. What it picks
// goes into the world's LevelSettings and from there into level.dat.
//
// The touch menus keep the platform's own dialog, because a phone's on-screen
// keyboard cannot type into a box this screen draws itself.
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
	virtual void keyPressed(int eventKey);
	virtual void keyboardNewChar(char inputChar);

private:
	void createWorld();
	void refreshLabels();
	std::string makeLevelId() const;

	Button bMode;
	Button bDifficulty;
	Button bCheats;
	Button bDebug;
	Button bCreate;
	Button bCancel;

	std::string name;
	bool creative;
	bool peaceful;
	bool cheats;
	bool debug;

	std::vector<std::string> existingIds;
	int ticks;
	bool hasStarted;

	// Where the name box is, filled in by setupPositions().
	int boxX, boxY, boxW, boxH;
	int descY1, descY2;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__CreateWorldScreen_H__*/
