#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__

#include <string>
#include "../Screen.h"

// The chat line: type a message and press Enter. A message starting with '/' is
// a command (see ChatScreen::runCommand); anything else is shown in the chat.
// Opened with T in every mode; the commands themselves need Allow Cheats.
class ChatScreen: public Screen
{
public:
	ChatScreen();
	virtual ~ChatScreen() {}

	void init();
	void render(int xm, int ym, float a);

protected:
	void buttonClicked(Button* button) {}
	void keyPressed(int eventKey);
	void keyboardNewChar(char inputChar);

private:
	void send();
	void runCommand(const std::string& line);
	void feedback(const std::string& text);

	std::string text;
	int ticks;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__*/
