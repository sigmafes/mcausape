#ifndef NET_MINECRAFT_NETWORK_PACKET__ExtPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ExtPacket_H__

#include "../Packet.h"

/** The messages this port adds to 0.6.1's protocol, in one packet so each new
    one is a Kind and not another packet class. Always from the host to a
    visitor (a visitor asks for things by chat: a line starting with '/' is a
    command the host runs, see ServerCommands).

    a, b, c and text mean what the Kind says. */
class ExtPacket: public Packet
{
public:
	enum Kind {
		/// a: 1 creative, 0 survival. The host changed this player's game mode.
		GameMode = 1,
		/// text: the names of everyone in the world, one per line.
		PlayerList = 2,
		/// a, b, c: item id, count, data. Put it in this player's inventory.
		Give = 3,
		/// text: why. The host is about to close the connection.
		Kick = 4,
	};

	ExtPacket() : kind(0), a(0), b(0), c(0) {}
	ExtPacket(int kind, int a = 0, int b = 0, int c = 0, const std::string& text = "")
	:	kind(kind), a(a), b(b), c(c), text(text) {}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_EXT));
		bitStream->Write(kind);
		bitStream->Write(a);
		bitStream->Write(b);
		bitStream->Write(c);
		bitStream->Write(RakNet::RakString(text.c_str()));
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(kind);
		bitStream->Read(a);
		bitStream->Read(b);
		bitStream->Read(c);
		RakNet::RakString s;
		bitStream->Read(s);
		text = s.C_String();
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (ExtPacket*)this);
	}

	int kind, a, b, c;
	std::string text;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ExtPacket_H__*/
