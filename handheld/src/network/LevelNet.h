#ifndef NET_MINECRAFT_NETWORK__LevelNet_H__
#define NET_MINECRAFT_NETWORK__LevelNet_H__

#include "RakNetInstance.h"
#include "Packet.h"
#include "../raknet/RakPeerInterface.h"
#include "../world/level/Level.h"
#include "../world/entity/player/Player.h"

#include <vector>

/** What a Level sends its packets through when a hosted world has more than one
    Level running (a visitor in the Nether while the host is in the Overworld).

    Everything a level broadcasts -- entity movement, explosions, the time --
    goes to `raknetInstance->send(packet)`, which is "to everyone connected".
    With one level that is right; with two, a player on the other side of a
    portal would be sent the wrong world's entities. This sends only to the
    players who are in its level (and those about to be: `pending`, the players
    the host is still loading the level for).

    Everything else is the real instance's. */
class LevelNet: public IRakNetInstance
{
public:
	LevelNet(IRakNetInstance* base, Level* level, std::vector<Player*>* pending)
	:	base(base), level(level), pending(pending) {}

	virtual RakNet::RakPeerInterface* getPeer() { return base->getPeer(); }
	virtual bool isMyLocalGuid(const RakNet::RakNetGUID& guid) { return base->isMyLocalGuid(guid); }
	virtual bool isServer() { return base->isServer(); }

	virtual void send(Packet& packet)
	{
		// One send per player, through the real instance: it is what decides how
		// a packet travels (entity positions go unreliable and low priority on the
		// web, or the host's queue fills up and the visitor sees the world minutes late).
		const RakNet::RakNetGUID me = base->getPeer()->GetMyGUID();
		for (unsigned int i = 0; i < level->players.size(); ++i)
			if (!(level->players[i]->owner == me)) base->send(level->players[i]->owner, packet);
		if (pending) {
			for (unsigned int i = 0; i < pending->size(); ++i)
				if ((*pending)[i]->level == level) base->send((*pending)[i]->owner, packet);
		}
	}
	virtual void send(const RakNet::RakNetGUID& guid, Packet& packet) { base->send(guid, packet); }

	virtual void send(Packet* packet) { send(*packet); delete packet; }
	virtual void send(const RakNet::RakNetGUID& guid, Packet* packet) { base->send(guid, packet); }

private:
	IRakNetInstance* base;
	Level* level;
	std::vector<Player*>* pending;
};

#endif /*NET_MINECRAFT_NETWORK__LevelNet_H__*/
