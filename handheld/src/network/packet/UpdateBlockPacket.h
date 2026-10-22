#ifndef NET_MINECRAFT_NETWORK_PACKET__UpdateBlockPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__UpdateBlockPacket_H__

#include "../Packet.h"
#include "../../world/level/Level.h"

class UpdateBlockPacket : public Packet
{
public:

	int x, z;
	unsigned char y;
	unsigned char blockId;
	unsigned char blockData;

	/// blockData's 4 bits are the block's own data; this one, above them, says
	/// water shares the block (Level::isWaterlogged).
	static const int WaterloggedBit = 0x10;
	static int dataWithWater(Level* level, int x, int y, int z) {
		return level->getData(x, y, z) | (level->isWaterlogged(x, y, z) ? WaterloggedBit : 0);
	}

	UpdateBlockPacket()
	{
	}

	UpdateBlockPacket(int _x, int _y, int _z, int _blockId, int _blockData)
	{
		x = _x;
		y = (unsigned char)(_y & 0xff);
		z = _z;
		blockId = (unsigned char)(_blockId & 0xff);
		blockData = (unsigned char)(_blockData & 0xff);
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_UPDATEBLOCK));

		bitStream->Write(x);
		bitStream->Write(z);
		bitStream->Write(y);
		bitStream->Write(blockId);
		bitStream->Write(blockData);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(x);
		bitStream->Read(z);
		bitStream->Read(y);
		bitStream->Read(blockId);
		bitStream->Read(blockData);

	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (UpdateBlockPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__UpdateBlockPacket_H__*/
