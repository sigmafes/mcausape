#ifndef NET_MINECRAFT_NETWORK_PACKET__PlayerDataPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__PlayerDataPacket_H__

#include "../Packet.h"
#include "../../nbt/NbtIo.h"
#include "../../nbt/CompoundTag.h"
#include "../../util/RakDataIO.h"
#include <string>

/** A visitor's player as the world's save would hold it -- position,
    inventory, armour, health, dimension -- as NBT bytes. Not in 0.6.1, where a
    visitor's inventory lived only in its own tab and was gone when it left.

    Visitor to host, every few seconds and on leaving: the host keeps the
    latest under the player's name in the world folder. Host to visitor, once
    on joining (empty if the name is new): what to start with. */
class PlayerDataPacket: public Packet
{
public:
	PlayerDataPacket() : entityId(0) {}
	PlayerDataPacket(int entityId, const std::string& bytes) : entityId(entityId), bytes(bytes) {}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_PLAYERDATA));
		bitStream->Write(entityId);
		bitStream->Write((int) bytes.size());
		if (!bytes.empty())
			bitStream->Write(bytes.data(), (unsigned int) bytes.size());
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		int size = 0;
		bitStream->Read(size);
		bytes.clear();
		// A player's tag is a few KB; anything far past that is not one.
		if (size <= 0 || size > 256 * 1024) return;
		bytes.resize(size);
		if (!bitStream->Read(&bytes[0], (unsigned int) size)) bytes.clear();
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (PlayerDataPacket*)this);
	}

	static std::string toBytes(CompoundTag* tag)
	{
		RakNet::BitStream stream;
		RakDataOutput out(stream);
		NbtIo::write(tag, &out);
		return std::string((const char*) stream.GetData(), stream.GetNumberOfBytesUsed());
	}

	/// NULL for empty or unreadable bytes; the caller deletes it (deleteChildren, then delete).
	static CompoundTag* fromBytes(const std::string& bytes)
	{
		if (bytes.empty()) return NULL;
		RakNet::BitStream stream((unsigned char*) bytes.data(), (unsigned int) bytes.size(), false);
		RakDataInput in(stream);
		return NbtIo::read(&in);
	}

	int entityId;
	std::string bytes;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__PlayerDataPacket_H__*/
