#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__DialogDefinitions_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__DialogDefinitions_H__

class DialogDefinitions {
public:
	static const int DIALOG_CREATE_NEW_WORLD = 1;
	static const int DIALOG_NEW_CHAT_MESSAGE = 2;
	static const int DIALOG_MAINMENU_OPTIONS = 3;
	static const int DIALOG_RENAME_MP_WORLD  = 4;
	// Single-field text entry for CreateWorldScreen's own boxes, used only on
	// touch: there is no physical keyboard to type into a box the screen draws
	// itself, so a tap on either box borrows the platform's text dialog for one
	// field and hands the typed value back to the screen that is still on
	// screen underneath it.
	static const int DIALOG_CREATE_WORLD_NAME = 5;
	static const int DIALOG_CREATE_WORLD_SEED = 6;
	// The player name box in Options > Profile, on touch -- same reasoning.
	static const int DIALOG_PLAYER_NAME = 7;
    static const int DIALOG_DEMO_FEATURE_DISABLED = 98;
};

#endif /*#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__DialogDefinitions_H__*/
