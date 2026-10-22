#include "ChatScreen.h"
#include "../Font.h"
#include "../Gui.h"
#include "../../Minecraft.h"
#include "../../../network/packet/ChatPacket.h"
#include "../../../network/RakNetInstance.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../util/Mth.h"
#include "../../../util/StringUtils.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/ItemInstance.h"
#include "../../../world/entity/MobFactory.h"
#include "../../../world/entity/EntityTypes.h"
#include "../../player/LocalPlayer.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/level/MobSpawner.h"
#include "../../../world/level/storage/LevelData.h"
#include "../../../world/level/LevelSettings.h"
#include "../../../network/ServerSideNetworkHandler.h"
#include "../../../server/ServerCommands.h"
#include "DialogDefinitions.h"
#include "../../../AppPlatform.h"

#include <sstream>
#include <vector>

static const int MaxChatLength = 100;

#if defined(MC_WASM)
#include <emscripten.h>
// The page's invisible input (shell.html, #chatBridge): it only brings up the
// phone's keyboard; the line is drawn here, as on a PC.
EM_JS(void, mcpe_chat_open, (), { if (window.mcpeChatBridge) window.mcpeChatBridge.open(); });
EM_JS(void, mcpe_chat_close, (), { if (window.mcpeChatBridge) window.mcpeChatBridge.close(); });
EM_JS(int, mcpe_chat_state, (), { return window.mcpeChatBridge ? window.mcpeChatBridge.state() : 2; });
EM_JS(char*, mcpe_chat_value, (), {
	var s = window.mcpeChatBridge ? window.mcpeChatBridge.value() : '';
	var n = lengthBytesUTF8(s) + 1, p = _malloc(n);
	stringToUTF8(s, p, n);
	return p;
});
#endif

ChatScreen::ChatScreen()
:	ticks(0),
	awaitingDialog(false),
	touchKeyboard(false)
{
}

void ChatScreen::init() {
	// The T that opened the chat also queued its letter as text; drop it so the
	// chat line starts empty.
	while (Keyboard::nextTextChar())
		;
	// A phone has no keyboard to type into the strip below; the page's text
	// dialog brings up the phone's own (see OptionsScreen's name box).
	if (minecraft->useTouchscreen()) {
#if defined(MC_WASM)
		// Not the page's dialog box any more: the keyboard, and the line drawn
		// in the game like on a PC.
		touchKeyboard = true;
		mcpe_chat_open();
#else
		awaitingDialog = true;
		minecraft->platform()->createUserInput(DialogDefinitions::DIALOG_NEW_CHAT_MESSAGE, "");
#endif
	}
}

void ChatScreen::render(int xm, int ym, float a)
{
	++ticks;

	if (awaitingDialog) {
		// -2 while it is still open; 1 sent, 0 cancelled.
		const int status = minecraft->platform()->getUserInputStatus();
		if (status > -1) {
			awaitingDialog = false;
			StringVector sv;
			if (status == 1) sv = minecraft->platform()->getUserInput();
			if (!sv.empty()) {
				text = sv[0];
				send();	// closes the screen
			} else {
				minecraft->setScreen(NULL);
			}
		}
		return;
	}

#if defined(MC_WASM)
	if (touchKeyboard) {
		char* raw = mcpe_chat_value();
		std::string typed = raw ? raw : "";
		free(raw);
		// The game's font is one byte a letter: keep what it can draw.
		text.clear();
		for (unsigned int i = 0; i < typed.size() && (int) text.size() < MaxChatLength; ++i)
			if (typed[i] >= 32 && typed[i] <= 126) text += typed[i];
		const int state = mcpe_chat_state();
		if (state == 1) { touchKeyboard = false; send(); return; }
		if (state == 2) { touchKeyboard = false; minecraft->setScreen(NULL); return; }
	}
#endif

	// A strip along the bottom: the typed line, with a blinking cursor.
	const int bottom = height - 2;
	fill(2, bottom - 12, width - 2, bottom, 0x80000000);
	std::string shown = "> " + text;
	if ((ticks / 10) % 2 == 0)
		shown += "_";
	drawString(minecraft->font, shown, 4, bottom - 10, 0xffe0e0e0);
}

void ChatScreen::keyPressed(int eventKey)
{
	if (eventKey == Keyboard::KEY_ESCAPE) {
		minecraft->setScreen(NULL);
		return;
	}
	if (eventKey == Keyboard::KEY_RETURN) {
		send();
		return;
	}
	if (eventKey == Keyboard::KEY_BACKSPACE) {
		if (!text.empty())
			text.erase(text.size() - 1);
	}
}

void ChatScreen::keyboardNewChar(char inputChar)
{
	if (inputChar < 32 || inputChar > 126)
		return;
	if ((int) text.size() >= MaxChatLength)
		return;
	text += inputChar;
}

void ChatScreen::send()
{
	std::string line = Util::stringTrim(text);
	text.clear();

	if (!line.empty()) {
		if (line[0] == '/')
			runCommand(line.substr(1));
		else
			sendChat(line);
	}

	// Last, because this closes the screen.
	minecraft->setScreen(NULL);
}

/** A line of chat, as "<name> line" for everyone in the world.

    The host is the one place that knows who is who, so a visitor sends only the
    line and the host puts the name on it (see ServerSideNetworkHandler's
    ChatPacket handler) -- a client cannot speak as someone else -- and sends it
    on to everyone, the sender included. The host builds its own and sends it. */
void ChatScreen::sendChat(const std::string& line)
{
	if (minecraft->isOnlineClient()) {
		ChatPacket packet(line, false);
		minecraft->raknetInstance->send(packet);
		return;
	}
	const std::string msg = "<" + minecraft->player->name + "> " + line;
	minecraft->gui.addMessage(msg);
	if (minecraft->netCallback && minecraft->raknetInstance) {
		ChatPacket packet(msg, false);
		minecraft->raknetInstance->send(packet);
	}
}

void ChatScreen::feedback(const std::string& msg)
{
	minecraft->gui.addMessage(msg);
}

void ChatScreen::runCommand(const std::string& line)
{
	// A visitor's commands are run by the host, which knows who may give them.
	if (minecraft->isOnlineClient()) {
		ChatPacket packet("/" + line, false);
		minecraft->raknetInstance->send(packet);
		return;
	}

	if (!minecraft->player) {
		feedback("No hay jugador en el mundo.");
		return;
	}

	ServerSideNetworkHandler* net = minecraft->netCallback
		? static_cast<ServerSideNetworkHandler*>(minecraft->netCallback)
		: NULL;
	ServerCommands::run(minecraft, net, minecraft->player, line);
}
