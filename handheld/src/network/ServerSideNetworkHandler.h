#ifndef _MINECRAFT_NETWORK_SERVERSIDENETWORKHANDLER_H_
#define _MINECRAFT_NETWORK_SERVERSIDENETWORKHANDLER_H_


#include "NetEventCallback.h"
#include "../raknet/RakNetTypes.h"
#include "../world/level/LevelListener.h"
#include <vector>
#include <string>
#include "../server/ServerAccess.h"

class Minecraft;
class Level;
class IRakNetInstance;
class Packet;
class Player;
class ExtPacket;

class ServerSideNetworkHandler : public NetEventCallback, public LevelListener
{
public:
	ServerSideNetworkHandler(Minecraft* minecraft, IRakNetInstance* raknetInstance);
	virtual ~ServerSideNetworkHandler();

	virtual void levelGenerated(Level* level);

	virtual void tileChanged(int x, int y, int z);
	virtual void tileBrightnessChanged(int x, int y, int z) { /* do nothing */ }
	virtual Packet* getAddPacketFromEntity(Entity* entity);
	virtual void entityAdded(Entity* e);
	virtual void entityRemoved(Entity* e);
	virtual void levelEvent(Player* source, int type, int x, int y, int z, int data);
	virtual void tileEvent(int x, int y, int z, int b0, int b1);

	virtual void onNewClient(const RakNet::RakNetGUID& clientGuid);
	virtual void onDisconnect(const RakNet::RakNetGUID& guid);

	void onReady_ClientGeneration(const RakNet::RakNetGUID& source);
	void onReady_RequestedChunks(const RakNet::RakNetGUID& source);

	virtual void handle(const RakNet::RakNetGUID& source, LoginPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ReadyPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, MovePlayerPacket* packet);
	//virtual void handle(const RakNet::RakNetGUID& source, PlaceBlockPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, RemoveBlockPacket* packet);
	//virtual void handle(const RakNet::RakNetGUID& source, ExplodePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, RequestChunkPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerEquipmentPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerArmorEquipmentPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SetHealthPacket* packet);
	//virtual void handle(const RakNet::RakNetGUID& source, TeleportEntityPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, InteractPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, AnimatePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, UseItemPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, EntityEventPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerActionPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, RespawnPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SendInventoryPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, DropItemPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ContainerSetSlotPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ContainerClosePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SignUpdatePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerSkinPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ChatPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SetEntityDataPacket* packet);

	virtual void tick();

	/** The most people a hosted world holds at once, the host included. With a
	    cap of ten worlds online (see the lobby) that makes a hundred players the
	    most this can ever carry. */
	static const int MAX_PLAYERS = 10;

	//
	// What the commands (ServerCommands) need. "Operator" is the host or a
	// name /op has listed.
	//
	ServerAccess& access() { return _access; }
	bool isOperator(Player* player);
	std::vector<Player*> allPlayers();
	Player* findPlayerByName(const std::string& name);
	void sendMessage(Player* to, const std::string& message);
	void sendExt(Player* to, ExtPacket& packet);
	void setPlayerGameMode(Player* player, bool creative);
	void kick(Player* player, const std::string& reason);
	/// After /whitelist: tell the lobby, and (when it was just turned on) send out whoever is not on it.
	void whitelistChanged();
	void enforceWhitelist();
	/// /auth enable asks everyone already in to log in; disable lets everyone act.
	void requireAuthFromEveryone();
	void releaseAuthFromEveryone();
	bool isAuthed(Player* player);
	/// Sends everyone the names of who is here (and keeps the host's own copy).
	void broadcastPlayerList();
	/// The lobby listing's extras: whether the whitelist is on, and how many are in.
	void announceLobbyInfo();

	bool allowsIncomingConnections() { return _allowIncoming; }
	void allowIncomingConnections(bool doAllow);

	Player* popPendingPlayer(const RakNet::RakNetGUID& source);
private:

	void redistributePacket(Packet* packet, const RakNet::RakNetGUID& fromPlayer);
	void displayGameMessage(const std::string& message);

	Player* getPlayer(const RakNet::RakNetGUID& source);
	void handleAuthCommand(Player* player, const std::string& cmd, const std::string& argument);
	void setAuthed(Player* player, bool authed);
	void applyGameMode(Player* player, bool creative);
	bool isUnauthenticated(const RakNet::RakNetGUID& source);
	void sendSkin(Player* player, const RakNet::RakNetGUID& target, bool broadcast);
private:

	Minecraft*					minecraft;
	Level*						level;
	IRakNetInstance*			raknetInstance;
	RakNet::RakPeerInterface*	rakPeer;

	std::vector<Player*> _pendingPlayers;
	bool _allowIncoming;

	ServerAccess _access;
	/// What the host knows about each visitor beyond its player.
	struct Session {
		RakNet::RakNetGUID guid;
		bool authed;
		int idleTicks;	///< ticks spent without logging in
		bool creative;	///< /gamemode: what this visitor is in now
		int failures;	///< wrong passwords so far
	};
	std::vector<Session> _sessions;
	Session* sessionOf(const RakNet::RakNetGUID& guid);
	void removeSession(const RakNet::RakNetGUID& guid);
	int _lastPlayerCount;
};

#endif
