#ifndef NET_MINECRAFT_SERVER__ServerCommands_H__
#define NET_MINECRAFT_SERVER__ServerCommands_H__

#include <string>

class Minecraft;
class ServerSideNetworkHandler;
class Player;

/** The chat commands, run on the host for whoever typed them: the host's own
    line goes straight in, a visitor's arrives as a chat line starting with '/'
    (see ServerSideNetworkHandler's ChatPacket handler). Answers go back to
    `sender` only. */
namespace ServerCommands
{
	/// `line` is the command without its '/'. `net` is the host's network
	/// handler, or NULL when there is none (then only the game's own commands work).
	void run(Minecraft* mc, ServerSideNetworkHandler* net, Player* sender, const std::string& line);
}

#endif /*NET_MINECRAFT_SERVER__ServerCommands_H__*/
