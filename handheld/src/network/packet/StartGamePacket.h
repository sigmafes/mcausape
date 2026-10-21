#ifndef NET_MINECRAFT_NETWORK_PACKET__StartGamePacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__StartGamePacket_H__

#include "../Packet.h"

class StartGamePacket : public Packet
{
public:
	long levelSeed;
	int levelGeneratorVersion;
	int gameType;

	int entityId;
	float x, y, z;
	/// Which dimension to build (Dimension::NORMAL or NETHER). Not in 0.6.1's
	/// packet: it is how a visitor is sent through a portal.
	int dimension;

	StartGamePacket()
	:	dimension(0)
	{
	}

	StartGamePacket(long seed, int levelGeneratorVersion, int gameType, int entityId, float x, float y, float z, int dimension = 0)
	:	levelSeed(seed),
		levelGeneratorVersion(levelGeneratorVersion),
		gameType(gameType),
		entityId(entityId),
		x(x),
		y(y),
		z(z),
		dimension(dimension)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_STARTGAME));

		bitStream->Write(levelSeed);
		bitStream->Write(levelGeneratorVersion);
		bitStream->Write(gameType);
		bitStream->Write(entityId);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(dimension);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(levelSeed);
		bitStream->Read(levelGeneratorVersion);
		bitStream->Read(gameType);
		bitStream->Read(entityId);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		bitStream->Read(dimension);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (StartGamePacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__StartGamePacket_H__*/
