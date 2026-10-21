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

#include <sstream>
#include <vector>

static const int MaxChatLength = 100;

ChatScreen::ChatScreen()
:	ticks(0)
{
}

void ChatScreen::init() {
	// The T that opened the chat also queued its letter as text; drop it so the
	// chat line starts empty.
	while (Keyboard::nextTextChar())
		;
}

void ChatScreen::render(int xm, int ym, float a)
{
	++ticks;

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
