#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include "ScreenChooser.h"

class ImageButton;
class OptionsPane;

class OptionsScreen: public Screen
{
	typedef Screen super;
	void init();

	void generateOptionScreens();

public:
	/** `returnTo` is where the close button goes. Reaching the options from
	    the pause menu and being dropped back at the title screen would mean
	    leaving the world, so the caller says where it came from. */
	OptionsScreen(ScreenId returnTo = SCREEN_STARTMENU);
	~OptionsScreen();
	void setupPositions();
	void buttonClicked( Button* button );
	void render(int xm, int ym, float a);
	void removed();
	void selectCategory(int index);

	virtual void mouseClicked( int x, int y, int buttonNum );
	virtual void mouseReleased( int x, int y, int buttonNum );
	virtual void tick();
	virtual void keyPressed(int eventKey);
	virtual void keyboardNewChar(char inputChar);
	virtual bool handleBackEvent(bool isDown);
	virtual void mouseWheel(int dy);
private:
	void close();
	// A tab taller than the screen scrolls: the wheel, or a vertical drag
	// (a finger, or the mouse). A press in the pane is held back until it is
	// clear what it is: a drag up or down scrolls, a sideways one is a
	// slider's (handed over then), and a tap is passed on at release.
	void scrollTo(int s);
	int maxScroll();
	int scrollY;
	bool pressActive;
	int pressMode;	// 0 undecided, 1 scrolling, 2 handed to the pane
	int pressX, pressY, pressScroll;

	// The Profile tab is drawn by this screen rather than built from an
	// OptionsGroup: a text box and two action buttons are not options, and
	// the panes only know how to show those.
	void layoutProfile();
	void renderProfile();
	void renderSkinFace(int x, int y, int size);
	void commitName();
	bool hasProfile() const { return returnTo == SCREEN_STARTMENU; }
	bool profileShown() const { return hasProfile() && selectedCategory == ProfileCategory; }

	static const int ProfileCategory = 0;
	static const int MaxNameLength = 16;

	Touch::TButton* bImportSkin;
	Touch::TButton* bResetSkin;
	std::string nameDraft;
	bool nameFocused;
	bool awaitingName;	// the touch dialog for the name is open
	int ticks;
	int nameX, nameY, nameW, nameH;
	int skinY;

	Touch::THeader* bHeader;
	ImageButton* btnClose;
	std::vector<Touch::TButton*> categoryButtons;
	std::vector<OptionsPane*> optionPanes;
	OptionsPane* currentOptionPane;
	int selectedCategory;
	ScreenId returnTo;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__*/
