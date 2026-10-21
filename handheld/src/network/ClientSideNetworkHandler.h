#ifndef _MINECRAFT_NETWORK_CLIENTSIDENETWORKHANDLER_H_
#define _MINECRAFT_NETWORK_CLIENTSIDENETWORKHANDLER_H_


#include "NetEventCallback.h"
#include "../raknet/RakNetTypes.h"
#include "../world/level/LevelConstants.h"

#include <vector>

class Minecraft;
class Level;
class IRakNetInstance;

struct SBufferedBlockUpdate
{
	int x, z;
	unsigned char y;
	unsigned char blockId;
	unsigned char blockData;
	bool setData;
};
typedef std::vector<SBufferedBlockUpdate> BlockUpdateList;

typedef struct IntPair {
    int x, y;
} IntPair;

class ClientSideNetworkHandler : public NetEventCallback
{
public:
	ClientSideNetworkHandler(Minecraft* minecraft, IRakNetInstance* raknetInstance);
	virtual ~ClientSideNetworkHandler();

	virtual void levelGenerated(Level* level);

	virtual void onConnect(const RakNet::RakNetGUID& hostGuid);
	virtual void onUnableToConnect();
	virtual void onDisconnect(const RakNet::RakNetGUID& guid);

	virtual void handle(const RakNet::RakNetGUID& source, LoginStatusPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, StartGamePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, MessagePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SetTimePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, AddItemEntityPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, AddPaintingPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, TakeItemEntityPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, AddEntityPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, AddMobPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, AddPlayerPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, RemoveEntityPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, RemovePlayerPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, MovePlayerPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, MoveEntityPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, UpdateBlockPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ExplodePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, LevelEventPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, TileEventPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, EntityEventPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ChunkDataPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerEquipmentPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerArmorEquipmentPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, InteractPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SetEntityDataPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SetEntityMotionPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SetHealthPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SetSpawnPositionPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, AnimatePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, UseItemPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, HurtArmorPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, RespawnPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ContainerOpenPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ContainerClosePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ContainerSetContentPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ContainerSetSlotPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ContainerSetDataPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ChatPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, AdventureSettingsPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SignUpdatePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerSkinPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ExtPacket* packet);
private:

	void requestNextChunk();
    void arrangeRequestChunkOrder();

	bool isChunkLoaded(int x, int z);
	bool areAllChunksLoaded();
	void clearChunksLoaded();
private:

	Minecraft* minecraft;
	Level* level;
	IRakNetInstance* raknetInstance;
	RakNet::RakPeerInterface* rakPeer;

	RakNet::RakNetGUID serverGuid;
	/// The host sent us away and said why; its closing the connection is not news.
	bool _kicked;

	BlockUpdateList	bufferedBlockUpdates;
	int	requestNextChunkPosition;

    static const int NumRequestChunks = CHUNK_CACHE_WIDTH * CHUNK_CACHE_WIDTH;

    // How many RequestChunkPacket/ChunkDataPacket round trips run at once.
    // Each reply requests exactly one more (see the ChunkDataPacket handler),
    // so seeding this many up front is what keeps that many in flight the
    // whole way through -- one at a time was a non-issue on a real LAN's
    // sub-millisecond round trips, but over a relay with real internet
    // latency, 576 chunks done one after another is most of what made
    // joining take minutes.
    static const int MaxInFlightChunkRequests = 16;

    int requestNextChunkIndex;
    IntPair requestNextChunkIndexList[NumRequestChunks];
	bool chunksLoaded[NumRequestChunks];
	int numChunksLoaded;
};

#endif
