#ifndef NET_MINECRAFT_NETWORK_PACKET__PlayerSkinPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__PlayerSkinPacket_H__

#include "../Packet.h"
#include <vector>

/** One player's custom skin, as raw RGBA (see PlayerSkins).

    Not part of 0.6.1's protocol: added so a skin chosen in the profile is what
    everyone else sees too. A client sends its own once its world exists and
    again when it changes; the host stamps it with the sender's entity id,
    keeps it on that player, and passes it on -- to everyone already in the
    world now, and to each newcomer when they join. An empty image means the
    player went back to the default skin.

    Up to 16KB, so it goes reliable but below the gameplay packets: RakNet
    splits it, and nothing waits on it. */
class PlayerSkinPacket: public Packet
{
public:
	PlayerSkinPacket()
	:	entityId(0), w(0), h(0)
	{
		init();
	}

	PlayerSkinPacket(int entityId, int w, int h, const unsigned char* rgba)
	:	entityId(entityId), w(0), h(0)
	{
		init();
		if (rgba && isSkinSize(w, h)) {
			this->w = w;
			this->h = h;
			this->rgba.assign(rgba, rgba + w * h * 4);
		}
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_PLAYERSKIN));
		bitStream->Write(entityId);
		bitStream->Write((unsigned short) w);
		bitStream->Write((unsigned short) h);
		if (!rgba.empty())
			bitStream->Write((const char*) &rgba[0], (unsigned int) rgba.size());
	}

	void read(RakNet::BitStream* bitStream)
	{
		unsigned short rw = 0, rh = 0;
		bitStream->Read(entityId);
		bitStream->Read(rw);
		bitStream->Read(rh);
		w = h = 0;
		rgba.clear();
		// Anything that is not a skin size is read as "default skin" rather
		// than trusted for how many bytes to pull off the stream.
		if (!isSkinSize(rw, rh))
			return;
		std::vector<unsigned char> data(rw * rh * 4);
		if (!bitStream->Read((char*) &data[0], (unsigned int) data.size()))
			return;
		w = rw;
		h = rh;
		rgba.swap(data);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (PlayerSkinPacket*) this);
	}

	bool hasSkin() const { return !rgba.empty(); }

	int entityId;
	int w, h;
	std::vector<unsigned char> rgba;

private:
	void init()
	{
		priority = MEDIUM_PRIORITY;
		reliability = RELIABLE_ORDERED;
	}

	static bool isSkinSize(int w, int h)
	{
		return w == 64 && (h == 32 || h == 64);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__PlayerSkinPacket_H__*/
