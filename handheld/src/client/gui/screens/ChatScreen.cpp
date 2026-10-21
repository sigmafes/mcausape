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

#include <sstream>
#include <vector>

static const int MaxChatLength = 100;

// The names a /summon can take, with the mob each one stands for.
struct SummonName {
	const char* name;
	int mobType;
};
static const SummonName SUMMON_NAMES[] = {
	{ "zombie",         MobTypes::Zombie },
	{ "skeleton",       MobTypes::Skeleton },
	{ "spider",         MobTypes::Spider },
	{ "pigzombie",      MobTypes::PigZombie },
	{ "creeper",        MobTypes::Creeper },
	{ "witherskeleton", MobTypes::WitherSkeleton },
	{ "blaze",          MobTypes::Blaze },
	{ "ghast",          MobTypes::Ghast },
	{ "magmacube",      MobTypes::MagmaCube },
	{ "pig",            MobTypes::Pig },
	{ "cow",            MobTypes::Cow },
	{ "chicken",        MobTypes::Chicken },
	{ "sheep",          MobTypes::Sheep },
};

static std::vector<std::string> splitWords(const std::string& line) {
	std::vector<std::string> words;
	std::istringstream in(line);
	std::string w;
	while (in >> w)
		words.push_back(w);
	return words;
}

static bool parseInt(const std::string& s, int& out) {
	std::istringstream in(s);
	in >> out;
	return !in.fail() && in.eof();
}

static std::string lower(std::string s) {
	for (unsigned int i = 0; i < s.size(); ++i)
		s[i] = (char) tolower(s[i]);
	return s;
}

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
	std::vector<std::string> words = splitWords(line);
	if (words.empty())
		return;

	std::string cmd = lower(words[0]);

	if (!minecraft->areCheatsAllowed()) {
		feedback("Los comandos estan desactivados: activa Allow Cheats al crear el mundo.");
		return;
	}

	LocalPlayer* player = minecraft->player;
	if (!player) {
		feedback("No hay jugador en el mundo.");
		return;
	}

	if (cmd == "summon") {
		if (words.size() < 2) {
			feedback("Uso: /summon <mob>");
			return;
		}
		std::string name = lower(words[1]);
		int mobType = -1;
		for (unsigned int i = 0; i < sizeof(SUMMON_NAMES) / sizeof(SummonName); ++i)
			if (name == SUMMON_NAMES[i].name) mobType = SUMMON_NAMES[i].mobType;
		if (mobType < 0) {
			feedback("Mob desconocido: " + words[1]);
			return;
		}

		// Three blocks in front of the player.
		float rad = player->yRot * Mth::DEGRAD;
		float sx = player->x - Mth::sin(rad) * 3;
		float sz = player->z + Mth::cos(rad) * 3;

		Mob* mob = MobFactory::CreateMob(mobType, minecraft->level);
		if (!mob) return;
		if (MobSpawner::addMob(minecraft->level, mob, sx, player->y, sz, player->yRot, 0, true))
			feedback("Invocado: " + name);
		else
			delete mob;
		return;
	}

	if (cmd == "give") {
		int id = 0, count = 1, aux = 0;
		if (words.size() < 2 || !parseInt(words[1], id)) {
			feedback("Uso: /give <id> [cantidad] [dato]");
			return;
		}
		if (words.size() >= 3 && (!parseInt(words[2], count) || count < 1)) {
			feedback("La cantidad debe ser un numero mayor que 0.");
			return;
		}
		if (words.size() >= 4 && !parseInt(words[3], aux)) {
			feedback("El dato debe ser un numero.");
			return;
		}
		if (count > 64) count = 64;
		if (id <= 0 || id >= Item::MAX_ITEMS || Item::items[id] == NULL) {
			feedback("Id de item desconocido: " + words[1]);
			return;
		}

		if (!player->inventory->add(new ItemInstance(id, count, aux)))
			feedback("El inventario esta lleno.");
		else
			feedback("Entregado: id " + words[1] + " x" + (words.size() >= 3 ? words[2] : std::string("1")));
		return;
	}

	if (cmd == "gamemode") {
		if (words.size() < 2) {
			feedback("Uso: /gamemode <0|1|survival|creative>");
			return;
		}
		std::string mode = lower(words[1]);
		bool creative;
		if (mode == "0" || mode == "s" || mode == "survival")
			creative = false;
		else if (mode == "1" || mode == "c" || mode == "creative")
			creative = true;
		else {
			feedback("Modo desconocido: " + words[1]);
			return;
		}

		minecraft->setIsCreativeMode(creative);
		// Saved with the world, so the mode is the same the next time it loads.
		minecraft->level->getLevelData()->setGameType(creative ? GameType::Creative : GameType::Survival);
		feedback(std::string("Modo de juego: ") + (creative ? "creativo" : "supervivencia"));
		return;
	}

	feedback("Comando desconocido: /" + words[0]);
}
