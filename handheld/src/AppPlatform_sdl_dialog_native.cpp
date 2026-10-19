#include "AppPlatform_sdl.h"
#include "client/gui/screens/DialogDefinitions.h"

/** Text entry for the native Windows build.

    The web build asks in an overlay in the page and macOS in a Cocoa sheet; this
    one has neither, so for now it answers on the player's behalf: a new world is
    called "World" with a random seed (the world list makes the name unique), and
    the chat and rename dialogs are cancelled. A proper in-game text box is the
    next thing this file should grow into, and is what a real build needs. */

void AppPlatform_sdl::showDialog(int dialogId)
{
	_userInput.clear();
	_dialogFields = 0;

	if (dialogId == DialogDefinitions::DIALOG_CREATE_NEW_WORLD) {
		_userInput.push_back("World");
		_userInput.push_back("");
		_userInputStatus = USERINPUT_OK;
	} else {
		_userInputStatus = USERINPUT_CANCEL;
	}
}

int AppPlatform_sdl::getUserInputStatus()
{
	// Reading the status consumes it, as the other targets do.
	const int status = _userInputStatus;
	_userInputStatus = USERINPUT_NOTINITED;
	return status;
}

StringVector AppPlatform_sdl::getUserInput()
{
	return _userInput;
}
