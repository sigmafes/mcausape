#ifndef NET_MINECRAFT_PLATFORM_WEB__WebBridge_H__
#define NET_MINECRAFT_PLATFORM_WEB__WebBridge_H__

#include <string>

/** The things the game asks of the page it runs in.

    Fullscreen, file pickers and downloads are browser features with no
    equivalent in 0.6.1's platform layer, and the page used to offer them as
    HTML buttons drawn over the game. They are the game's own menu items now,
    so the game needs a way to ask for them: this. Every call lands in a
    window.mcpe* object in shell.html.

    The browser only grants fullscreen and opens a file picker from inside a
    user gesture. A click reaches the game a frame after the browser saw it,
    which is still inside the few seconds of "transient activation" a gesture
    leaves behind, so calling these from a button handler works.

    Off the web every call is a no-op, and the menu items that use them check
    supported() first and are not shown. */
namespace WebBridge
{
	/// False everywhere but a browser.
	bool supported();

	bool fullscreenSupported();
	bool isFullscreen();
	void setFullscreen(bool on);

	/// Opens the file picker for a skin PNG. The answer comes back later, by
	/// way of mcpe_skin_set() in main_sdl.h -- or never, if the player cancels.
	void pickSkin();
	/// Forgets the saved skin. The game drops its own copy separately.
	void resetSkin();
	/// Hands the skin saved by an earlier session to the game, if there is one.
	void loadSavedSkin();

	/// Downloads one world, by its folder name, as a .zip.
	void exportWorld(const std::string& levelId);
	/// Opens the file picker for world .zips to add.
	void importWorlds();
	/// True once, after an import has written new worlds: the world list is
	/// stale and should be read again.
	bool takeWorldsChanged();

	/// A line of text over the game for a few seconds -- the results of the
	/// above, which finish long after the click that started them.
	void toast(const std::string& text);
}

#endif /*NET_MINECRAFT_PLATFORM_WEB__WebBridge_H__*/
