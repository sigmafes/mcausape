#include "ServerCommands.h"
#include "ServerAccess.h"
#include "../client/Minecraft.h"
#include "../client/player/LocalPlayer.h"
#include "../network/ServerSideNetworkHandler.h"
#include "../network/packet/ExtPacket.h"
#include "../util/Mth.h"
#include "../world/item/Item.h"
#include "../world/item/ItemInstance.h"
#include "../world/entity/MobFactory.h"
#include "../world/entity/EntityTypes.h"
#include "../world/entity/player/Player.h"
#include "../world/entity/player/Inventory.h"
#include "../world/level/Level.h"
#include "../world/level/MobSpawner.h"
#include "../world/level/storage/LevelData.h"
#include "../world/level/LevelSettings.h"

#include <sstream>
#include <vector>

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
	{ "squid",          MobTypes::Squid },
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

/// "Flint and Steel" / "flint-and-steel" / "Flint_And_Steel" -> "flint_and_steel".
static std::string normalizeName(const std::string& in) {
	std::string out;
	for (unsigned int i = 0; i < in.size(); ++i) {
		const char c = in[i];
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
			out += c;
		else if (c >= 'A' && c <= 'Z')
			out += (char) (c + 32);
		else if (c == ' ' || c == '-' || c == '_') {
			if (!out.empty() && out[out.size() - 1] != '_') out += '_';
		}
	}
	while (!out.empty() && out[out.size() - 1] == '_') out.erase(out.size() - 1);
	return out;
}

/** The id of the item a /give name stands for, or 0 (none) / -1 (several fit).

    Names are the ones the game shows -- "Flint and Steel", "Diamond Block" --
    written in lower case with underscores, plus the "minecraft:" form and the
    internal names ("flintAndSteel"). "Block of Diamond" is also taken as
    "diamond_block", the way Minecraft spells every metal block. An exact match
    wins; otherwise a name that only one item starts with does. */
static int findItemByName(const std::string& typed, std::string& ambiguous) {
	std::string want = normalizeName(typed);
	if (want.compare(0, 10, "minecraft_") == 0 || want.compare(0, 10, "minecraft:") == 0)
		want = want.substr(10);
	// A ':' is dropped by normalizeName, so "minecraft:grass" arrives as "minecraftgrass".
	if (want.compare(0, 9, "minecraft") == 0 && want.size() > 9) {
		want = want.substr(9);
		if (!want.empty() && want[0] == '_') want.erase(0, 1);
	}
	if (want.empty()) return 0;

	int prefixHit = 0, prefixCount = 0;
	ambiguous.clear();
	for (int id = 1; id < Item::MAX_ITEMS; ++id) {
		Item* item = Item::items[id];
		if (!item) continue;

		std::vector<std::string> names;
		names.push_back(normalizeName(item->getName()));
		// "tile.stoneBrick" / "flintAndSteel" -> the part after the last dot.
		std::string internal = item->getDescriptionId();
		const std::string::size_type dot = internal.rfind('.');
		if (dot != std::string::npos) internal = internal.substr(dot + 1);
		names.push_back(normalizeName(internal));
		// camelCase internal names: break before each capital.
		std::string spaced;
		for (unsigned int i = 0; i < internal.size(); ++i) {
			if (internal[i] >= 'A' && internal[i] <= 'Z' && i > 0) spaced += '_';
			spaced += internal[i];
		}
		names.push_back(normalizeName(spaced));
		// "block_of_diamond" -> "diamond_block"
		const std::string pre = "block_of_";
		if (names[0].compare(0, pre.size(), pre) == 0)
			names.push_back(names[0].substr(pre.size()) + "_block");

		bool prefix = false;
		for (unsigned int n = 0; n < names.size(); ++n) {
			if (names[n].empty()) continue;
			if (names[n] == want) return id;
			if (names[n].compare(0, want.size(), want) == 0) prefix = true;
		}
		if (prefix) {
			++prefixCount;
			prefixHit = id;
			if (prefixCount <= 4) {
				if (!ambiguous.empty()) ambiguous += ", ";
				ambiguous += normalizeName(item->getName());
			}
		}
	}
	if (prefixCount == 1) return prefixHit;
	return prefixCount > 1 ? -1 : 0;
}


static void reply(Minecraft* mc, ServerSideNetworkHandler* net, Player* to, const std::string& msg)
{
	if (net)
		net->sendMessage(to, msg);
	else if (to == (Player*) mc->player)
		mc->gui.addMessage(msg);
}

static bool nameIsHost(Minecraft* mc, const std::string& name)
{
	return mc->player && ServerAccess::key(mc->player->name) == ServerAccess::key(name);
}

struct HelpEntry {
	const char* name;
	bool everyone;	///< usable without being the host or an operator
	const char* usage;
	const char* text;
};

static const HelpEntry HELP[] = {
	{ "help",      true,  "/help [comando]", "Lista los comandos, o explica uno." },
	{ "register",  true,  "/register <contrasena>", "Con /auth activo: crea tu cuenta con ese nombre (minimo 4 caracteres)." },
	{ "login",     true,  "/login <contrasena>", "Con /auth activo: inicia sesion. Sin hacerlo no puedes moverte, mirar ni abrir el inventario, y a los 30 s te expulsa." },
	{ "kill",      true,  "/kill [jugador]", "Te mata. Con un nombre mata a ese jugador (solo anfitrion y operadores)." },
	{ "list",      false, "/list", "Muestra quien esta en el mundo." },
	{ "op",        false, "/op <jugador>", "Da permisos de operador: usar comandos e ignorar /grief off." },
	{ "deop",      false, "/deop <jugador>", "Quita los permisos de operador." },
	{ "whitelist", false, "/whitelist [add|remove|list] [jugador]", "/whitelist sola la activa o desactiva; activa, solo entran los jugadores de la lista y los operadores." },
	{ "auth",      false, "/auth <enable|disable>", "Obliga a los visitantes a registrarse o iniciar sesion con contrasena, para que nadie use su nombre." },
	{ "pvp",       false, "/pvp <on|off>", "Permite o impide que los jugadores se hagan dano entre si." },
	{ "grief",     false, "/grief <on|off>", "Con off, los visitantes que no son operadores no pueden romper ni colocar bloques, usar cofres, hornos o mesas, ni danar jugadores o criaturas." },
	{ "cheats",    false, "/cheats", "Habilita /summon, /give y /gamemode en este mundo. No se puede deshacer." },
	{ "gamemode",  false, "/gamemode <s|c> [jugador] | /gamemode world <s|c>", "Cambia a supervivencia (s) o creativo (c), a ti o a otro jugador. Con world, fija el modo con el que entran los jugadores de ahora en adelante, sin tocar el del anfitrion. Necesita /cheats." },
	{ "give",      false, "/give <id o nombre> [cantidad] [dato]", "Te da un objeto. Necesita /cheats." },
	{ "summon",    false, "/summon <mob>", "Invoca una criatura delante de ti. Necesita /cheats." },
};

static void help(Minecraft* mc, ServerSideNetworkHandler* net, Player* to, bool privileged, const std::string& topic)
{
	const unsigned int count = sizeof(HELP) / sizeof(HELP[0]);
	if (!topic.empty()) {
		std::string want = topic;
		if (!want.empty() && want[0] == '/') want = want.substr(1);
		for (unsigned int i = 0; i < count; ++i) {
			if (want != HELP[i].name) continue;
			reply(mc, net, to, std::string(HELP[i].usage));
			reply(mc, net, to, std::string(HELP[i].text) + (HELP[i].everyone ? "" : " (solo anfitrion y operadores)"));
			return;
		}
		reply(mc, net, to, "No existe /" + want + ". Usa /help para ver la lista.");
		return;
	}
	std::string line = "Comandos:";
	for (unsigned int i = 0; i < count; ++i) {
		if (!privileged && !HELP[i].everyone) continue;
		line += std::string(" /") + HELP[i].name;
	}
	reply(mc, net, to, line);
	reply(mc, net, to, "Usa /help <comando> para saber que hace cada uno.");
}

void ServerCommands::run(Minecraft* mc, ServerSideNetworkHandler* net, Player* sender, const std::string& line)
{
	std::vector<std::string> words = splitWords(line);
	if (words.empty())
		return;

	const std::string cmd = lower(words[0]);
	const bool isHost = (sender == (Player*) mc->player);

	if (!sender) {
		return;
	}

	// For everyone: what they can type, and what each command does.
	if (cmd == "help") {
		help(mc, net, sender, isHost || (net && net->isOperator(sender)), words.size() >= 2 ? lower(words[1]) : "");
		return;
	}

	// /kill: anyone may end themselves; someone else takes the host or an op.
	if (cmd == "kill") {
		Player* victim = sender;
		if (words.size() >= 2) {
			if (!isHost && !(net && net->isOperator(sender))) {
				reply(mc, net, sender, "Solo el anfitrion y los operadores pueden matar a otro jugador.");
				return;
			}
			victim = NULL;
			if (net) victim = net->findPlayerByName(words[1]);
			if (!victim && mc->player && mc->player->name == words[1]) victim = (Player*) mc->player;
			if (!victim) { reply(mc, net, sender, "No esta en el mundo: " + words[1]); return; }
		}
		// Straight to no health, past armour and creative's invulnerability,
		// as Java's /kill. A visitor's client hears it from ServerPlayer::tick.
		victim->health = 0;
		victim->die(NULL);
		if (victim != sender) reply(mc, net, sender, victim->name + " ha muerto.");
		return;
	}

	// Every command is for the host and whoever the host has made an operator.
	if (!isHost && !(net && net->isOperator(sender))) {
		reply(mc, net, sender, "No tienes permiso para usar comandos.");
		return;
	}

	if (cmd == "list") {
		std::string names;
		if (net) {
			std::vector<Player*> all = net->allPlayers();
			for (unsigned int i = 0; i < all.size(); ++i) {
				if (!names.empty()) names += ", ";
				names += all[i]->name;
			}
		} else if (mc->player) names = mc->player->name;
		reply(mc, net, sender, "Conectados: " + names);
		return;
	}

	// Who is in charge of the server, not of the game: no Allow Cheats needed.
	if (cmd == "op" || cmd == "deop") {
		if (words.size() < 2) {
			reply(mc, net, sender, "Uso: /" + cmd + " <jugador>");
			return;
		}
		if (!net) { reply(mc, net, sender, "Solo disponible al alojar un mundo."); return; }
		const std::string& who = words[1];
		if (!ServerAccess::validName(who)) { reply(mc, net, sender, "Nombre no valido: " + who); return; }
		if (cmd == "op") {
			if (nameIsHost(mc, who)) reply(mc, net, sender, who + " ya es el anfitrion.");
			else if (net->access().addOp(who)) {
				reply(mc, net, sender, who + " ahora es operador.");
				if (Player* p = net->findPlayerByName(who)) net->sendMessage(p, "Ahora eres operador: puedes usar comandos.");
				net->applyRules();
			} else reply(mc, net, sender, who + " ya es operador.");
		} else {
			if (nameIsHost(mc, who)) reply(mc, net, sender, "El anfitrion siempre tiene permisos.");
			else if (net->access().removeOp(who)) {
				reply(mc, net, sender, who + " ya no es operador.");
				if (Player* p = net->findPlayerByName(who)) net->sendMessage(p, "Ya no eres operador.");
				net->applyRules();
			} else reply(mc, net, sender, who + " no era operador.");
		}
		return;
	}

	if (cmd == "whitelist") {
		if (!net) { reply(mc, net, sender, "Solo disponible al alojar un mundo."); return; }
		ServerAccess& acc = net->access();
		if (words.size() == 1) {
			acc.setWhitelist(!acc.whitelistOn());
			net->whitelistChanged();
			reply(mc, net, sender, acc.whitelistOn() ? "Whitelist activada: solo entran los jugadores de la lista." : "Whitelist desactivada.");
			return;
		}
		const std::string sub = lower(words[1]);
		if (sub == "list") {
			std::string names;
			for (unsigned int i = 0; i < acc.whitelisted().size(); ++i) {
				if (!names.empty()) names += ", ";
				names += acc.whitelisted()[i];
			}
			reply(mc, net, sender, std::string("Whitelist ") + (acc.whitelistOn() ? "(activa): " : "(apagada): ") + (names.empty() ? "vacia" : names));
			return;
		}
		if ((sub == "add" || sub == "remove") && words.size() >= 3) {
			const std::string& who = words[2];
			if (!ServerAccess::validName(who)) { reply(mc, net, sender, "Nombre no valido: " + who); return; }
			if (sub == "add") {
				if (acc.addWhitelisted(who)) reply(mc, net, sender, who + " agregado a la whitelist.");
				else reply(mc, net, sender, who + " ya estaba en la whitelist.");
			} else {
				if (acc.removeWhitelisted(who)) {
					reply(mc, net, sender, who + " quitado de la whitelist.");
					// Taking someone off while the list is on also sends them out.
					net->enforceWhitelist();
				} else reply(mc, net, sender, who + " no estaba en la whitelist.");
			}
			return;
		}
		reply(mc, net, sender, "Uso: /whitelist | /whitelist add <jugador> | /whitelist remove <jugador> | /whitelist list");
		return;
	}

	// Cheats can be turned on for a world made without them, never off again.
	if (cmd == "cheats") {
		LevelData* data = mc->level->getLevelData();
		if (data->getCheats()) {
			reply(mc, net, sender, "Los cheats ya estaban habilitados.");
			return;
		}
		data->setCheats(true);
		for (unsigned int i = 0; i < mc->otherLevels.size(); ++i) mc->otherLevels[i]->getLevelData()->setCheats(true);
		mc->level->saveLevelData();
		reply(mc, net, sender, "Cheats habilitados en este mundo. No se pueden deshabilitar.");
		return;
	}

	if (cmd == "pvp" || cmd == "grief") {
		if (!net) { reply(mc, net, sender, "Solo disponible al alojar un mundo."); return; }
		const std::string sub = words.size() >= 2 ? lower(words[1]) : "";
		if (sub != "on" && sub != "off") {
			reply(mc, net, sender, "Uso: /" + cmd + " <on|off>");
			return;
		}
		const bool on = (sub == "on");
		if (cmd == "pvp") {
			net->access().setPvp(on);
			net->applyRules();
			reply(mc, net, sender, on ? "PvP activado." : "PvP desactivado.");
		} else {
			net->access().setGrief(on);
			net->applyRules();
			reply(mc, net, sender, on
				? "Interaccion de visitantes activada."
				: "Interaccion de visitantes desactivada: no pueden romper ni colocar bloques, usar mesas, hornos, cofres ni cortapiedras, ni danar animales o enemigos.");
		}
		return;
	}

	if (cmd == "auth") {
		if (!net) { reply(mc, net, sender, "Solo disponible al alojar un mundo."); return; }
		const std::string sub = words.size() >= 2 ? lower(words[1]) : "";
		if (sub != "enable" && sub != "disable") {
			reply(mc, net, sender, "Uso: /auth <enable|disable>");
			return;
		}
		net->access().setAuth(sub == "enable");
		if (sub == "enable") {
			reply(mc, net, sender, "Autenticacion activada: los jugadores usan /register <contrasena> y /login <contrasena>.");
			net->requireAuthFromEveryone();
		} else {
			reply(mc, net, sender, "Autenticacion desactivada.");
			net->releaseAuthFromEveryone();
		}
		return;
	}

	// From here on it is the game's own commands.
	if (!mc->areCheatsAllowed()) {
		reply(mc, net, sender, "Los comandos estan desactivados: usa /cheats para habilitarlos (no se pueden deshabilitar despues).");
		return;
	}

	if (cmd == "summon") {
		if (words.size() < 2) {
			reply(mc, net, sender, "Uso: /summon <mob>");
			return;
		}
		std::string name = lower(words[1]);
		int mobType = -1;
		for (unsigned int i = 0; i < sizeof(SUMMON_NAMES) / sizeof(SummonName); ++i)
			if (name == SUMMON_NAMES[i].name) mobType = SUMMON_NAMES[i].mobType;
		if (mobType < 0) {
			reply(mc, net, sender, "Mob desconocido: " + words[1]);
			return;
		}

		// Three blocks in front of the player, in the world the player is in.
		float rad = sender->yRot * Mth::DEGRAD;
		float sx = sender->x - Mth::sin(rad) * 3;
		float sz = sender->z + Mth::cos(rad) * 3;

		Mob* mob = MobFactory::CreateMob(mobType, sender->level);
		if (!mob) return;
		if (MobSpawner::addMob(sender->level, mob, sx, sender->y, sz, sender->yRot, 0, true))
			reply(mc, net, sender, "Invocado: " + name);
		else
			delete mob;
		return;
	}

	if (cmd == "give") {
		int id = 0, count = 1, aux = 0;
		if (words.size() < 2) {
			reply(mc, net, sender, "Uso: /give <id o nombre> [cantidad] [dato]");
			return;
		}
		// A number is an id; anything else is looked up by name.
		if (!parseInt(words[1], id)) {
			std::string options;
			id = findItemByName(words[1], options);
			if (id == -1) {
				reply(mc, net, sender, "Varios items coinciden: " + options);
				return;
			}
			if (id == 0) {
				reply(mc, net, sender, "Item desconocido: " + words[1]);
				return;
			}
		}
		if (words.size() >= 3 && (!parseInt(words[2], count) || count < 1)) {
			reply(mc, net, sender, "La cantidad debe ser un numero mayor que 0.");
			return;
		}
		if (words.size() >= 4 && !parseInt(words[3], aux)) {
			reply(mc, net, sender, "El dato debe ser un numero.");
			return;
		}
		if (count > 64) count = 64;
		if (id <= 0 || id >= Item::MAX_ITEMS || Item::items[id] == NULL) {
			reply(mc, net, sender, "Id de item desconocido: " + words[1]);
			return;
		}

		std::ostringstream msg;
		msg << "Entregado: " << Item::items[id]->getName() << " (id " << id << ") x" << count;
		if (isHost) {
			if (!sender->inventory->add(new ItemInstance(id, count, aux))) {
				reply(mc, net, sender, "El inventario esta lleno.");
				return;
			}
		} else {
			// A visitor's inventory is its own client's: it is asked to add it.
			ExtPacket give(ExtPacket::Give, id, count, aux);
			net->sendExt(sender, give);
		}
		reply(mc, net, sender, msg.str());
		return;
	}

	if (cmd == "gamemode") {
		if (words.size() < 2) {
			reply(mc, net, sender, "Uso: /gamemode <s|c> [jugador] o /gamemode world <s|c>");
			return;
		}
		// The world's mode, for whoever joins from now on: the host's own
		// mode (the one above) is the host's alone.
		if (lower(words[1]) == "world") {
			if (!net) { reply(mc, net, sender, "Solo disponible al alojar un mundo."); return; }
			const std::string m = words.size() >= 3 ? lower(words[2]) : "";
			int wm = -1;
			if (m == "0" || m == "s" || m == "survival") wm = 0;
			else if (m == "1" || m == "c" || m == "creative") wm = 1;
			if (wm < 0) { reply(mc, net, sender, "Uso: /gamemode world <s|c>"); return; }
			net->access().setWorldMode(wm);
			reply(mc, net, sender, std::string("Modo del mundo: ") + (wm ? "creativo" : "supervivencia")
				+ ". Los jugadores que entren desde ahora empiezan asi.");
			return;
		}
		std::string mode = lower(words[1]);
		bool creative;
		if (mode == "0" || mode == "s" || mode == "survival")
			creative = false;
		else if (mode == "1" || mode == "c" || mode == "creative")
			creative = true;
		else {
			reply(mc, net, sender, "Modo desconocido: " + words[1]);
			return;
		}

		Player* target = sender;
		if (words.size() >= 3) {
			target = net ? net->findPlayerByName(words[2]) : NULL;
			if (!target) {
				reply(mc, net, sender, "Jugador no encontrado: " + words[2]);
				return;
			}
		}

		const char* modeName = creative ? "creativo" : "supervivencia";
		if (target == (Player*) mc->player) {
			mc->setIsCreativeMode(creative);
			// Saved with the world, so the mode is the same the next time it loads.
			mc->level->getLevelData()->setGameType(creative ? GameType::Creative : GameType::Survival);
		} else {
			net->setPlayerGameMode(target, creative);
		}
		reply(mc, net, sender, std::string(target == sender ? "Modo de juego: " : target->name + ": modo de juego ") + modeName);
		if (target != sender)
			reply(mc, net, target, std::string("Tu modo de juego ahora es ") + modeName + ".");
		return;
	}

	reply(mc, net, sender, "Comando desconocido: /" + words[0]);
}
