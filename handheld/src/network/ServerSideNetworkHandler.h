#ifndef _MINECRAFT_NETWORK_SERVERSIDENETWORKHANDLER_H_
#define _MINECRAFT_NETWORK_SERVERSIDENETWORKHANDLER_H_


#include "NetEventCallback.h"
#include "../raknet/RakNetTypes.h"
#include <vector>
#include <string>
#include "../server/ServerAccess.h"

class Minecraft;
class Level;
class IRakNetInstance;
class Packet;
class Player;
class ExtPacket;

class LevelView;
class LevelNet;

/** The host's side of the network. A hosted world can have two Levels running
    (the host's, and the other dimension's when a visitor is there), so nothing
    here may assume "the" level: a player's level is `player->level`, and
    levelOf() finds the one a connection's player is in. Each Level gets a
    LevelView, which is what listens to it and says which one a change was in. */
class ServerSideNetworkHandler : public NetEventCallback
{
public:
	ServerSideNetworkHandler(Minecraft* minecraft, IRakNetInstance* raknetInstance);
	virtual ~ServerSideNetworkHandler();

	virtual void levelGenerated(Level* level);
	virtual void levelAdded(Level* level);
	virtual void levelRemoved(Level* level);

	// What a Level reports through its LevelView.
	void tileChangedIn(Level* level, int x, int y, int z);
	void entityAddedIn(Level* level, Entity* e);
	void entityRemovedIn(Level* level, Entity* e);
	void levelEventIn(Level* level, Player* source, int type, int x, int y, int z, int data);
	void tileEventIn(Level* level, int x, int y, int z, int b0, int b1);

	virtual Packet* getAddPacketFromEntity(Entity* entity);

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
	/// Puts the pvp rule on every running level.
	void applyRules();
	/// True if /grief off stops this visitor from changing the world (and says so, now and then).
	bool griefBlocked(const RakNet::RakNetGUID& source);
	/// Sends everyone the names of who is here (and keeps the host's own copy).
	void broadcastPlayerList();
	/// The lobby listing's extras: whether the whitelist is on, and how many are in.
	void announceLobbyInfo();

	bool allowsIncomingConnections() { return _allowIncoming; }
	void allowIncomingConnections(bool doAllow);

	Player* popPendingPlayer(const RakNet::RakNetGUID& source);

	std::vector<Player*>* pendingList() { return &_pendingPlayers; }
	/// Somebody is still loading a level (a login or a trip through a portal).
	bool hasPending() { return !_pendingPlayers.empty(); }

	/// The Level this connection's player is in, or is about to be in; NULL if
	/// the connection has no player.
	Level* levelOf(const RakNet::RakNetGUID& source);

	/** Sends a visitor through a portal: out of its level, into the other
	    dimension (made if it is not running), and tells its client to load it
	    (a StartGamePacket, then the same steps as joining). `respawn`: it died
	    there, so it arrives at the spawn point instead of by a portal. */
	void travelVisitor(Player* player, int dimension, bool respawn);

	/// The host is leaving / entering a Level (Minecraft::changeDimension while
	/// visitors are about): the visitors there lose / gain its player.
	void hostLeaving(Level* from);
	void hostEntered(Level* to);
private:
	struct Hook {
		Level* level;
		LevelView* view;
		LevelNet* net;
	};
	std::vector<Hook> _hooks;

	void hookLevel(Level* level);
	Hook* hookOf(Level* level);
	std::vector<Level*> levels();
	void redistributeIn(Level* level, Packet* packet, const RakNet::RakNetGUID& exclude);
	/// The one place an already-written packet goes out raw: ordered world
	/// state, high priority, channel 0. Everything else is raknetInstance->send.
	void sendWorld(const RakNet::RakNetGUID& to, RakNet::BitStream& bits);
	/// One chunk (and its tile entities) to one visitor, on the chunk channel.
	void sendChunkTo(const RakNet::RakNetGUID& to, Level* level, int x, int z);
	/// Starts sending a visitor every chunk of its level, nearest first.
	void startStream(const RakNet::RakNetGUID& to, Level* level, Player* player);
	void pumpStreams();
	double sendBufferBytes(const RakNet::RakNetGUID& to);
	void sendHostTo(const RakNet::RakNetGUID& target, bool broadcastInLevel, Level* level);

	/// To everyone in the sender's level but the sender.
	void redistributePacket(Packet* packet, const RakNet::RakNetGUID& fromPlayer);
	void displayGameMessage(const std::string& message);

	Player* getPlayer(const RakNet::RakNetGUID& source);
	void handleAuthCommand(Player* player, const std::string& cmd, const std::string& argument);
	void setAuthed(Player* player, bool authed);
	void checkVisitorPortal(Player* player, float x, float y, float z);
	void applyGameMode(Player* player, bool creative);
	bool isUnauthenticated(const RakNet::RakNetGUID& source);
	void sendSkin(Player* player, const RakNet::RakNetGUID& target, bool broadcast);
private:

	Minecraft*					minecraft;
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
		bool transferring;	///< going through a portal: not joining
		int griefNotice;	///< ticks until it may be told again
		int portalCooldown;	///< ticks before its next portal trip counts
		/// The chunks of its level still to send it, nearest first.
		std::vector<int> stream;
		unsigned int streamPos;
		Level* streamLevel;
	};
	std::vector<Session> _sessions;
	Session* sessionOf(const RakNet::RakNetGUID& guid);
	void removeSession(const RakNet::RakNetGUID& guid);
	int _lastPlayerCount;
};

#endif
