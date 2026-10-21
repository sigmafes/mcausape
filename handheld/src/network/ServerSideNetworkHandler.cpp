
#include "ServerSideNetworkHandler.h"
#include "../world/level/Level.h"
#include "../world/entity/player/Player.h"
#include "../world/entity/player/Inventory.h"
#include "../world/Container.h"
#include "../world/inventory/BaseContainerMenu.h"
#include "packet/PacketInclude.h"

#include "RakNetInstance.h"
#include "../client/Minecraft.h"
#include "../client/player/LocalPlayer.h"
#include "../client/gamemode/GameMode.h"
#include "../raknet/RakPeerInterface.h"
#include "../raknet/PacketPriority.h"
#ifndef STANDALONE_SERVER
#include "../client/sound/SoundEngine.h"
#endif
#include "../server/ServerPlayer.h"
#include "../world/entity/item/FallingTile.h"
#include "../client/skin/PlayerSkins.h"
#include "../server/ServerCommands.h"
#include "LevelNet.h"
#include "../world/level/LevelListener.h"
#include "../world/level/dimension/Dimension.h"
#include "../world/level/tile/PortalTile.h"
#include "../util/Mth.h"
#if defined(MC_WASM)
#include "WebRakNetInstance.h"
#endif
#include "../world/level/storage/LevelStorage.h"
#include "../world/level/storage/LevelData.h"

#define TIMES(x) for(int itc ## __LINE__ = 0; itc ## __LINE__ < x; ++ itc ## __LINE__)

/** Listens to one Level for the handler and says which Level it was. */
class LevelView: public LevelListener
{
public:
	LevelView(ServerSideNetworkHandler* handler, Level* level) : handler(handler), level(level) {}

	void tileChanged(int x, int y, int z) { handler->tileChangedIn(level, x, y, z); }
	void tileBrightnessChanged(int x, int y, int z) { /* do nothing */ }
	void entityAdded(Entity* e) { handler->entityAddedIn(level, e); }
	void entityRemoved(Entity* e) { handler->entityRemovedIn(level, e); }
	void levelEvent(Player* source, int type, int x, int y, int z, int data) { handler->levelEventIn(level, source, type, x, y, z, data); }
	void tileEvent(int x, int y, int z, int b0, int b1) { handler->tileEventIn(level, x, y, z, b0, b1); }

private:
	ServerSideNetworkHandler* handler;
	Level* level;
};

ServerSideNetworkHandler::ServerSideNetworkHandler(Minecraft* minecraft, IRakNetInstance* raknetInstance)
:	minecraft(minecraft),
	raknetInstance(raknetInstance),
	_lastPlayerCount(0)
{
	allowIncomingConnections(false);
	rakPeer = raknetInstance->getPeer();
}

ServerSideNetworkHandler::~ServerSideNetworkHandler()
{
	// Every Level it served goes back to the plain network.
	while (!_hooks.empty()) {
		Hook h = _hooks.back();
		_hooks.pop_back();
		h.level->removeListener(h.view);
		h.level->raknetInstance = raknetInstance;
		delete h.view;
		delete h.net;
	}

	for (unsigned int i = 0; i < _pendingPlayers.size(); ++i)
		delete _pendingPlayers[i];
	// The visitors' skins only lived as long as this game.
	PlayerSkins::releaseAllBut(PlayerSkins::localSkin);
}

void ServerSideNetworkHandler::applyRules()
{
	std::vector<Level*> all = levels();
	for (unsigned int i = 0; i < all.size(); ++i)
		all[i]->adventureSettings.noPvP = !_access.pvpOn();
}

bool ServerSideNetworkHandler::griefBlocked(const RakNet::RakNetGUID& source)
{
	if (_access.griefOn()) return false;
	Player* p = getPlayer(source);
	if (!p || isOperator(p)) return false;
	if (Session* s = sessionOf(source)) {
		if (s->griefNotice <= 0) {
			s->griefNotice = 5 * 20;
			sendMessage(p, "El anfitrion desactivo la interaccion de visitantes con el mundo.");
		}
	}
	return true;
}

void ServerSideNetworkHandler::hookLevel(Level* level)
{
	if (!level || level->isClientSide || hookOf(level)) return;
	Hook h;
	h.level = level;
	h.view = new LevelView(this, level);
	h.net = new LevelNet(raknetInstance, level, &_pendingPlayers);
	_hooks.push_back(h);
	level->addListener(h.view);
	// Its broadcasts go to its own players only.
	level->raknetInstance = h.net;
	level->adventureSettings.noPvP = !_access.pvpOn();
}

ServerSideNetworkHandler::Hook* ServerSideNetworkHandler::hookOf(Level* level)
{
	for (unsigned int i = 0; i < _hooks.size(); ++i)
		if (_hooks[i].level == level) return &_hooks[i];
	return NULL;
}

void ServerSideNetworkHandler::levelAdded(Level* level)
{
	hookLevel(level);
}

void ServerSideNetworkHandler::levelRemoved(Level* level)
{
	for (unsigned int i = 0; i < _hooks.size(); ++i) {
		if (_hooks[i].level == level) {
			Hook h = _hooks[i];
			_hooks.erase(_hooks.begin() + i);
			level->removeListener(h.view);
			level->raknetInstance = raknetInstance;
			delete h.view;
			delete h.net;
			return;
		}
	}
}

std::vector<Level*> ServerSideNetworkHandler::levels()
{
	std::vector<Level*> all;
	if (minecraft->level) all.push_back(minecraft->level);
	if (minecraft->otherLevel) all.push_back(minecraft->otherLevel);
	return all;
}

Level* ServerSideNetworkHandler::levelOf(const RakNet::RakNetGUID& source)
{
	std::vector<Level*> all = levels();
	for (unsigned int l = 0; l < all.size(); ++l)
		for (unsigned int i = 0; i < all[l]->players.size(); ++i)
			if (all[l]->players[i]->owner == source) return all[l];
	for (unsigned int i = 0; i < _pendingPlayers.size(); ++i)
		if (_pendingPlayers[i]->owner == source) return _pendingPlayers[i]->level;
	return NULL;
}

void ServerSideNetworkHandler::tileChangedIn(Level* level, int x, int y, int z)
{
	//LOGI("tileChanged(%d, %d, %d)\n", x, y, z);

	// broadcast change event
	UpdateBlockPacket packet(x, y, z, level->getTile(x, y, z), level->getData(x, y, z));
	redistributeIn(level, &packet, RakNet::UNASSIGNED_RAKNET_GUID);
}

Packet* ServerSideNetworkHandler::getAddPacketFromEntity( Entity* entity ) {
	if (entity->isMob() && !entity->isPlayer()) { //@fix: This code is duplicated. See if it can be unified.
		if (minecraft->player) {
			// I guess this should always be true, but it crashed somewhere in this
			// function once and I only see this one as a potential problem
			return new AddMobPacket((Mob*)entity);
		}
	}
	else if (entity->isPlayer()) {

	} else if (entity->isItemEntity()) {
		AddItemEntityPacket* packet = new AddItemEntityPacket((ItemEntity*)entity);
		entity->xd = packet->xa();
		entity->yd = packet->ya();
		entity->zd = packet->za();
		//LOGI("item-entity @ server: %f, %f, %f\n", e->xd, e->yd, e->zd);
		return packet;
	} else if(entity->isHangingEntity()) {
		return new AddPaintingPacket((Painting*) entity);
	} else {
        int type = entity->getEntityTypeId();
        int data = entity->getAuxData();

        if (EntityTypes::IdFallingTile == type) {
            FallingTile* ft = (FallingTile*) entity;
            data = -(ft->tile | (ft->data << 16));
        }

		//LOGI("Server: adding entity with %f, %f, %f\n", e->xd, e->yd, e->zd);
		AddEntityPacket* packet = new AddEntityPacket(entity, data);
		/*
		entity->xd = packet->xd;
		entity->yd = packet->yd;
		entity->zd = packet->zd;
		*/
		return packet;
	}
	return NULL;
}
void ServerSideNetworkHandler::entityAddedIn(Level* level, Entity* e) {
	Packet* packet = getAddPacketFromEntity(e);
	if (packet != NULL) {
		redistributeIn(level, packet, RakNet::UNASSIGNED_RAKNET_GUID);
		delete packet;
	}
}

void ServerSideNetworkHandler::entityRemovedIn(Level* level, Entity* e)
{
	if (!e->isPlayer()) { //@fix: This code MIGHT be duplicated. See if it can be unified.
		RemoveEntityPacket packet(e->entityId);
		redistributeIn(level, &packet, RakNet::UNASSIGNED_RAKNET_GUID);
	} else { // Is a player
		RemovePlayerPacket packet((Player*) e);
		redistributeIn(level, &packet, RakNet::UNASSIGNED_RAKNET_GUID);
	}
}

void ServerSideNetworkHandler::redistributeIn(Level* level, Packet* packet, const RakNet::RakNetGUID& exclude)
{
	if (!level) return;
	RakNet::BitStream bitStream;
	packet->write(&bitStream);
	const RakNet::RakNetGUID me = rakPeer->GetMyGUID();
	for (unsigned int i = 0; i < level->players.size(); ++i) {
		const RakNet::RakNetGUID& to = level->players[i]->owner;
		if (to == me || to == exclude) continue;
		rakPeer->Send(&bitStream, packet->priority, packet->reliability, 0, to, false);
	}
	// The ones the level is still being loaded for hear about it too.
	for (unsigned int i = 0; i < _pendingPlayers.size(); ++i) {
		const RakNet::RakNetGUID& to = _pendingPlayers[i]->owner;
		if (_pendingPlayers[i]->level != level || to == exclude) continue;
		rakPeer->Send(&bitStream, packet->priority, packet->reliability, 0, to, false);
	}
}

void ServerSideNetworkHandler::redistributePacket(Packet* packet, const RakNet::RakNetGUID& fromPlayer)
{
	// Everyone in the sender's level but the sender. A host's own change is
	// in the host's level.
	Level* level = (fromPlayer == rakPeer->GetMyGUID()) ? minecraft->level : levelOf(fromPlayer);
	redistributeIn(level, packet, fromPlayer);
}

void ServerSideNetworkHandler::displayGameMessage(const std::string& message)
{
#ifndef STANDALONE_SERVER
	minecraft->gui.addMessage(message);
#else
	LOGI("%s\n", message.c_str());
#endif
	MessagePacket packet(message.c_str());
	raknetInstance->send(packet);
}

void ServerSideNetworkHandler::onNewClient(const RakNet::RakNetGUID& clientGuid)
{
	LOGI("onNewClient, client guid: %s\n", clientGuid.ToString());
}

void ServerSideNetworkHandler::onDisconnect(const RakNet::RakNetGUID& guid)
{
	if (!minecraft->level) return;
	LOGI("onDisconnect\n");

	removeSession(guid);
	// Left before the world finished loading: no player in the level yet.
	for (unsigned int i = 0; i < _pendingPlayers.size(); ++i) {
		if (_pendingPlayers[i]->owner == guid) {
			delete _pendingPlayers[i];
			_pendingPlayers.erase(_pendingPlayers.begin() + i);
			break;
		}
	}

	const std::vector<Player*> players = allPlayers();
	for (unsigned int i = 0; i < players.size(); i++)
	{
		Player* player = players[i];

		if (player->owner == guid)
		{
			std::string message = player->name;
			message += " disconnected from the game";
			displayGameMessage(message);

			//RemoveEntityPacket packet(player->entityId);
			//raknetInstance->send(packet);
			PlayerSkins::release(player->skinId);
			player->skinId = 0;
			player->reallyRemoveIfPlayer = true;
			player->level->removeEntity(player);
			//level->removePlayer(player);
			
			LOGI("&e@disc: %p", player);

			break;
		}
	}
	broadcastPlayerList();
	announceLobbyInfo();
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, LoginPacket* packet)
{
	if (!minecraft->level) return;
	if (!_allowIncoming) return;

	LOGI("LoginPacket\n");

	int loginStatus = LoginStatus::Success;
	//
	// Bad/incompatible client version
	//
	bool oldClient = packet->clientNetworkVersion < SharedConstants::NetworkProtocolLowestSupportedVersion;
	bool oldServer = packet->clientNetworkLowestSupportedVersion > SharedConstants::NetworkProtocolVersion;
	if (oldClient || oldServer)
		loginStatus = oldClient? LoginStatus::Failed_ClientOld : LoginStatus::Failed_ServerOld;

	// The host's own rules, in the order a player would want to hear them.
	const std::string loginName = packet->clientName.C_String();
	if (LoginStatus::Success == loginStatus) {
		if (!ServerAccess::validName(loginName)) {
			loginStatus = LoginStatus::Failed_BadName;
		} else {
			bool taken = false;
			std::vector<Player*> everyone = allPlayers();
			for (unsigned int i = 0; i < _pendingPlayers.size(); ++i) everyone.push_back(_pendingPlayers[i]);
			for (unsigned int i = 0; i < everyone.size(); ++i)
				if (ServerAccess::key(everyone[i]->name) == ServerAccess::key(loginName)) taken = true;
			if (taken)
				loginStatus = LoginStatus::Failed_NameTaken;
			else if (_access.whitelistOn() && !_access.isWhitelisted(loginName) && !_access.isOp(loginName))
				loginStatus = LoginStatus::Failed_NotWhitelisted;
			else if ((int) everyone.size() >= MAX_PLAYERS)
				loginStatus = LoginStatus::Failed_Full;
		}
	}

	RakNet::BitStream bitStream;
	LoginStatusPacket(loginStatus).write(&bitStream);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);

	if (LoginStatus::Success != loginStatus)
		return;

	//
	// Valid client version
	//
	// A newcomer starts in the Overworld, at its spawn, wherever the host is.
	Level* level = minecraft->levelForDimension(Dimension::NORMAL, true);
	if (!level) level = minecraft->level;
	Player* newPlayer = new ServerPlayer(minecraft, level);

	minecraft->gameMode->initAbilities(newPlayer->abilities);
	newPlayer->owner = source;
	newPlayer->name = loginName;
	_pendingPlayers.push_back(newPlayer);

	Session session;
	session.guid = source;
	session.authed = !_access.authOn();
	session.idleTicks = 0;
	session.failures = 0;
	session.transferring = false;
	session.portalCooldown = 0;
	session.griefNotice = 0;
	session.creative = minecraft->isCreativeMode();
	_sessions.push_back(session);
	applyGameMode(newPlayer, session.creative);

	// Reset the player so he doesn't spawn inside blocks
	while (newPlayer->y > 0) {
		newPlayer->setPos(newPlayer->x, newPlayer->y, newPlayer->z);
		if (level->getCubes(newPlayer, newPlayer->bb).size() == 0) break;
		newPlayer->y += 1;
	}
	newPlayer->moveTo(newPlayer->x, newPlayer->y - newPlayer->heightOffset, newPlayer->z, newPlayer->yRot, newPlayer->xRot);

	// send world seed
	{
        RakNet::BitStream bitStream;

        // @todo: Read from LevelData?
		int gameType = minecraft->isCreativeMode()
			? GameType::Creative
			: GameType::Survival;

        StartGamePacket(
            level->getSeed(),
            level->getLevelData()->getGeneratorVersion(),
            gameType,
            newPlayer->entityId,
            newPlayer->x, newPlayer->y - newPlayer->heightOffset, newPlayer->z,
            level->dimension->id
        ).write(&bitStream);

        rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ReadyPacket* packet)
{
	Level* level = levelOf(source);
	if (!level) return;

	if (packet->type == ReadyPacket::READY_CLIENTGENERATION)
		onReady_ClientGeneration(source);

	if (packet->type == ReadyPacket::READY_REQUESTEDCHUNKS)
		onReady_RequestedChunks(source);
}

void ServerSideNetworkHandler::onReady_ClientGeneration(const RakNet::RakNetGUID& source)
{
	Player* newPlayer = popPendingPlayer(source);
	if (!newPlayer) {
		for (int i = 0; i < 3; ++i)
			LOGE("We don't have a user associated with this player!\n");
		return;
	}
	// The level the newcomer is in -- a player coming through a portal arrives
	// in the other dimension's.
	Level* level = newPlayer->level;
	// Create a bitstream that can be used by everyone (after ::reset() )
	RakNet::BitStream bitStream;

	// send level info
	SetTimePacket(level->getTime()).write(&bitStream);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);

	// send all pre-existing players to the new player
	const PlayerList& players = level->players;
	for (unsigned int i = 0; i < players.size(); i++) {
		Player* player = players[i];

        bitStream.Reset();
        AddPlayerPacket(player).write(&bitStream);
        rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);

        if (player->getArmorTypeHash()) {
            bitStream.Reset();
            PlayerArmorEquipmentPacket(player).write(&bitStream);
            rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
        }

		sendSkin(player, source, false);
	}

	level->addEntity(newPlayer);
	// Someone who only changed dimension has not joined anything.
	Session* session = sessionOf(source);
	const bool transferred = session && session->transferring;
	if (session) session->transferring = false;
	if (!transferred) {
#ifndef STANDALONE_SERVER
		minecraft->gui.addMessage(newPlayer->name + " joined the game");
#else
		LOGW("%s joined the game\n", newPlayer->name.c_str());
#endif
	}

	// Send all Entities to the new player
	for (unsigned int i = 0; i < level->entities.size(); ++i) {
		Entity* e = level->entities[i];
		Packet* packet = getAddPacketFromEntity(e);
		if(packet != NULL) {
			bitStream.Reset();
			packet->write(&bitStream);
			rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
			delete packet;
		}
	}

	// Additional packets
	// * set spawn
	/*
	bitStream.Reset();
	SetSpawnPositionPacket spawnPacket(level->getSharedSpawnPos());
	raknetInstance->send(source, spawnPacket);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
	*/

	// broadcast the new player to all other players
	AddPlayerPacket addNewPlayer(newPlayer);
	redistributeIn(level, &addNewPlayer, source);
	// A skin the newcomer sent before this point is on newPlayer already.
	if (PlayerSkins::get(newPlayer->skinId))
		sendSkin(newPlayer, source, true);

	broadcastPlayerList();
	announceLobbyInfo();
}

//
// Messages to be sent after client has finished applying changes
//
void ServerSideNetworkHandler::onReady_RequestedChunks(const RakNet::RakNetGUID& source)
{
	if (Player* joined = getPlayer(source)) {
		if (_access.authOn() && !isAuthed(joined))
			sendMessage(joined, _access.hasAccount(joined->name)
				? "Este mundo pide autenticacion: usa /login <contrasena>."
				: "Este mundo pide autenticacion: usa /register <contrasena>.");
	}

	Level* level = levelOf(source);
	if (!level) return;

	RakNet::BitStream bitStream;
	// Send all TileEntities to the new player
	for (unsigned int i = 0; i < level->tileEntities.size(); ++i) {
		TileEntity* e = level->tileEntities[i];
		Packet* packet = e->getUpdatePacket();
		if (packet != NULL) {
			bitStream.Reset();
			packet->write(&bitStream);
			rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
			delete packet;
		}
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, MovePlayerPacket* packet)
{
	Level* level = levelOf(source);
	if (!level) return;

	//LOGI("MovePlayerPacket\n");
	if (Entity* entity = level->getEntity(packet->entityId))
	{
		entity->xd = entity->yd = entity->zd = 0;
		entity->lerpTo(packet->x, packet->y, packet->z, packet->yRot, packet->xRot, 3);

		// broadcast this packet to other clients
		redistributePacket(packet, source);

		if (entity->isPlayer())
			checkVisitorPortal((Player*) entity, packet->x, packet->y, packet->z);
	}
}

/** A visitor standing in a portal goes through it, once: a trip starts only
    from a cold start (cooldown 0), and being inside keeps the cooldown full, so
    arriving in one cannot send it straight back -- the same rule as the host's
    (Minecraft::tick). The check is here because a visitor's player is moved by
    its client's packets, not by physics, so its portal tile never sees it. */
void ServerSideNetworkHandler::checkVisitorPortal(Player* player, float x, float y, float z)
{
	if (player == (Player*) minecraft->player) return;
	Session* s = sessionOf(player->owner);
	if (!s || s->transferring) return;
	if (isUnauthenticated(player->owner)) return;

	Level* level = player->level;
	const int tx = Mth::floor(x);
	const int tz = Mth::floor(z);
	// The packet's y is the eye; feet and body are below it.
	const int feet = Mth::floor(y - player->heightOffset + 0.1f);
	const int portal = Tile::portalTile->id;
	const bool inPortal = level->getTile(tx, feet, tz) == portal || level->getTile(tx, feet + 1, tz) == portal;

	if (inPortal) {
		if (s->portalCooldown == 0) {
			travelVisitor(player, level->dimension->id == Dimension::NETHER ? Dimension::NORMAL : Dimension::NETHER, false);
		} else {
			s->portalCooldown = 40;
		}
	} else if (s->portalCooldown > 0) {
		--s->portalCooldown;
	}
}

void ServerSideNetworkHandler::travelVisitor(Player* player, int dimension, bool respawn)
{
	if (!player || player == (Player*) minecraft->player) return;
	Level* src = player->level;
	Session* s = sessionOf(player->owner);
	if (!src || !s || s->transferring) return;

	Level* dst = minecraft->levelForDimension(dimension, true);
	if (!dst || dst == src) return;

	int tx, ty, tz;
	minecraft->findArrival(dst, dimension == Dimension::NETHER, player->x, player->z, respawn, player, &tx, &ty, &tz);

	// Out of the old level: everyone there loses it.
	RemovePlayerPacket gone(player);
	redistributeIn(src, &gone, player->owner);
	src->detachEntityForTransfer(player);

	player->setLevel(dst);
	player->dimension = dimension;
	player->moveTo(tx + 0.5f, (float) ty, tz + 0.5f, player->yRot, player->xRot);

	// Into the new one the way a newcomer goes: its client is told to build the
	// dimension, then asks for it, and onReady_ClientGeneration adds it.
	s->transferring = true;
	s->portalCooldown = 40;
	_pendingPlayers.push_back(player);

	const int gameType = s->creative ? GameType::Creative : GameType::Survival;
	StartGamePacket start(dst->getSeed(), dst->getLevelData()->getGeneratorVersion(), gameType, player->entityId,
		player->x, player->y - player->heightOffset, player->z, dimension);
	raknetInstance->send(player->owner, start);
}

void ServerSideNetworkHandler::hostLeaving(Level* from)
{
	if (!minecraft->player) return;
	RemovePlayerPacket gone((Player*) minecraft->player);
	redistributeIn(from, &gone, RakNet::UNASSIGNED_RAKNET_GUID);
}

void ServerSideNetworkHandler::hostEntered(Level* to)
{
	Player* host = (Player*) minecraft->player;
	if (!host) return;
	for (unsigned int i = 0; i < to->players.size(); ++i) {
		Player* visitor = to->players[i];
		if (visitor == host) continue;
		AddPlayerPacket add(host);
		raknetInstance->send(visitor->owner, add);
		if (host->getArmorTypeHash()) {
			PlayerArmorEquipmentPacket armor(host);
			raknetInstance->send(visitor->owner, armor);
		}
		sendSkin(host, visitor->owner, false);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, RemoveBlockPacket* packet){
	Level* level = levelOf(source);
	if (!level) return;
	if (isUnauthenticated(source)) return;
	if (griefBlocked(source)) return;

	Player* player = getPlayer(source);
	if (!player) return;

	player->swing();

	int x = packet->x, y = packet->y, z = packet->z;

	// code copied from GameMode.cpp
	int oldId = level->getTile(x, y, z);
	int data = level->getData(x, y, z);
	Tile* oldTile = Tile::tiles[oldId];
	bool changed = level->setTile(x, y, z, 0);
	if (oldTile != NULL && changed) {
		level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, oldTile->soundType->getBreakSound(), (oldTile->soundType->getVolume() + 1) / 2, oldTile->soundType->getPitch() * 0.8f);

		if (!player->abilities.instabuild && player->canDestroy(oldTile))
			//oldTile->spawnResources(level, x, y, z, data, 1); //@todo
			oldTile->playerDestroy(level, player, x, y, z, data);

		oldTile->destroy(level, x, y, z, data);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, RequestChunkPacket* packet)
{
	Level* level = levelOf(source);
	if (!level)
        return;

    LevelChunk* chunk = level->getChunk(packet->x, packet->z);

	if (!chunk)
        return;

	ChunkDataPacket cpacket(chunk->x, chunk->z, chunk);

	RakNet::BitStream bitStream;
	cpacket.write(&bitStream);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
    //LOGI("RequestChunkPacket @ (%d, %d). %d bytes\n", packet->x, packet->z, cpacket.chunkData.GetNumberOfBytesUsed());
	//LOGI("size: %d\n", bitStream.GetNumberOfBytesUsed());

    const LevelChunk::TEMap& teMap = chunk->getTileEntityMap();
    for (LevelChunk::TEMapCIterator cit = teMap.begin(); cit != teMap.end(); ++cit) {
        TileEntity* te = cit->second;
        if (Packet* p = te->getUpdatePacket()) {
            bitStream.Reset();
            p->write(&bitStream);
            raknetInstance->send(source, p);
        }
    }
}

void ServerSideNetworkHandler::levelGenerated( Level* level )
{
	if (minecraft->player) {
		minecraft->player->owner = rakPeer->GetMyGUID();
	}
	
	hookLevel(level);
	if (LevelStorage* storage = level->getLevelStorage())
		_access.load(storage->getFolder());
	applyRules();
#ifndef STANDALONE_SERVER
	allowIncomingConnections(minecraft->options.serverVisible);
#else
	allowIncomingConnections(true);
#endif
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerEquipmentPacket* packet)
{
	Level* level = levelOf(source);
	if (!level) return;

	Player* player = getPlayer(source);
	if (!player) return;
	if (rakPeer->GetMyGUID() == player->owner) return;

	// override the player's inventory
	//int slot = player->inventory->getSlot(packet->itemId, packet->itemAuxValue);
	int slot = Inventory::MAX_SELECTION_SIZE;
	if (slot >= 0) {
		if (packet->itemId == 0) {
			player->inventory->clearSlot(slot);
		} else {
			// @note: 128 is an ugly hack for depletable items.
			// @todo: fix
			ItemInstance newItem(packet->itemId, 128, packet->itemAuxValue);
			player->inventory->replaceSlot(slot, &newItem);
		}
		player->inventory->moveToSelectedSlot(slot, true);
		redistributePacket(packet, source);
	} else {
		LOGW("Warning: Remote player doesn't have his thing, Odd!\n");
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerArmorEquipmentPacket* packet) {
    Level* level = levelOf(source);
    if (!level) return;

    Player* player = getPlayer(source);
    if (!player) return;
    if (rakPeer->GetMyGUID() == player->owner) return;

    packet->fillIn(player);
    redistributePacket(packet, source);
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, InteractPacket* packet) {
	Level* level = levelOf(source);
	if (!level) return;
	if (isUnauthenticated(source)) return;
	if (griefBlocked(source)) return;

	Entity* src = level->getEntity(packet->sourceId);
	Entity* entity = level->getEntity(packet->targetId);
	if (src && entity && src->isPlayer())
	{
		Player* player = (Player*) src;
		if (InteractPacket::Attack == packet->action) {
			player->swing();
			minecraft->gameMode->attack(player, entity);
		}
		if (InteractPacket::Interact == packet->action) {
			player->swing();
			minecraft->gameMode->interact(player, entity);
		}

		redistributePacket(packet, source);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, AnimatePacket* packet)
{
	Level* level = levelOf(source);
	if (!level)
        return;

    // Own player -> invalid
    if (minecraft->player && minecraft->player->entityId == packet->entityId) {
        return;
    }

	Entity* entity = level->getEntity(packet->entityId);
	if (!entity || !entity->isPlayer())
        return;

    Player* player = (Player*) entity;

	switch (packet->action) {
	case AnimatePacket::Swing:
		player->swing();
		break;
	default:
		LOGW("Unknown Animate action: %d\n", packet->action);
		break;
	}
	redistributePacket(packet, source);
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, UseItemPacket* packet)
{
	Level* level = levelOf(source);
	if (!level) return;

	if (isUnauthenticated(source)) return;
	// Eating and drawing a bow are not the world.
	if (packet->face != 255 && griefBlocked(source)) return;
	LOGI("UseItemPacket\n");
	Entity* entity = level->getEntity(packet->entityId);
	if (entity && entity->isPlayer()) {
		Player* player = (Player*) entity;
		int x = packet->x, y = packet->y, z = packet->z;
		Tile* t = Tile::tiles[level->getTile(x, y, z)];

		if (t == Tile::invisible_bedrock) return;
		if (t && t->use(level, x, y, z, player)) return;
		if (packet->item.isNull()) return;

		ItemInstance* item = &packet->item;

		if(packet->face == 255) {
            // Special case: x,y,z means direction-of-action
            player->aimDirection.set(packet->x / 32768.0f, packet->y / 32768.0f, packet->z / 32768.0f);
			minecraft->gameMode->useItem(player, level, item);
		}
		else {
			minecraft->gameMode->useItemOn(player, level, item, packet->x, packet->y, packet->z, packet->face,
				Vec3(packet->clickX + packet->x, packet->clickY + packet->y, packet->clickZ + packet->z));
		}
		
		//LOGW("Use Item not working! Out of synch?\n");

		// Don't have to redistribute (ugg.. this will mess up), cause tileUpdated is sent
		//redistributePacket(packet, source);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, EntityEventPacket* packet) {
	Level* level = levelOf(source);
	if (!level) return;

	if (Entity* e = level->getEntity(packet->entityId))
		e->handleEntityEvent(packet->eventId);
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerActionPacket* packet)
{
	Level* level = levelOf(source);
	if (!level) return;
	LOGI("PlayerActionPacket\n");
	Entity* entity = level->getEntity(packet->entityId);
	if (entity && entity->isPlayer()) {
		Player* player = (Player*) entity;
		if(packet->action == PlayerActionPacket::RELEASE_USE_ITEM) {
			minecraft->gameMode->releaseUsingItem(player);
			return;
		}
		else if(packet->action == PlayerActionPacket::STOP_SLEEPING) {
			player->stopSleepInBed(true, true, true);
		}
	}
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, RespawnPacket* packet )
{
	Level* level = levelOf(source);
	if (!level) return;

	// Dying in the Nether sends a visitor home, as it does the host: it is
	// taken to the Overworld's spawn, not set down where the Nether says.
	if (level->dimension->id == Dimension::NETHER) {
		if (Player* p = getPlayer(source)) {
			p->reset();
			travelVisitor(p, Dimension::NORMAL, true);
			return;
		}
	}

	NetEventCallback::handle(level, source, packet );
	redistributePacket(packet, source);
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, SendInventoryPacket* packet )
{
	Level* level = levelOf(source);
	if (!level) return;

	Entity* entity = level->getEntity(packet->entityId);
	if (entity && entity->isPlayer()) {
		Player* p = (Player*)entity;
		p->inventory->replace(packet->items, packet->numItems);
		if ((packet->extra & SendInventoryPacket::ExtraDrop) != 0) {
			p->inventory->dropAll(false);
            //@todo @armor : Drop armor
		}
	}
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, DropItemPacket* packet )
{
	Level* level = levelOf(source);
	if (!level) return;
	if (isUnauthenticated(source)) return;

	Entity* entity = level->getEntity(packet->entityId);
	if (entity && entity->isPlayer()) {
		Player* p = (Player*)entity;
		p->drop(new ItemInstance(packet->item), packet->dropType != 0);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ContainerClosePacket* packet) {
	Level* level = levelOf(source);
	if (!level) return;

	Player* p = findPlayer(level, &source);
	if (!p) return;

	if (p != minecraft->player)
		static_cast<ServerPlayer*>(p)->doCloseContainer();
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ContainerSetSlotPacket* packet) {
	Level* level = levelOf(source);
	if (!level) return;
	if (isUnauthenticated(source)) return;
	if (griefBlocked(source)) return;

	Player* p = findPlayer(level, &source);
	if (!p) return;

	if (p->containerMenu == NULL) {
		LOGW("User has no container!\n");
		return;
	}
	if (p->containerMenu->containerId != packet->containerId)
	{
		LOGW("Wrong container id: %d vs %d\n", p->containerMenu->containerId, packet->containerId);
		return;
	}
	
	if (ContainerType::FURNACE == p->containerMenu->containerType) {
		//LOGI("Server:Setting slot %d: %s\n", packet->slot, packet->item.toString().c_str());
		p->containerMenu->setSlot(packet->slot, &packet->item);
		//p->containerMenu->setSlot(packet->slot, packet->item.isNull()? NULL : &packet->item);
	}
	if (ContainerType::CONTAINER == p->containerMenu->containerType) {
		//LOGI("Server:Setting slot %d: %s\n", packet->slot, packet->item.toString().c_str());
		p->containerMenu->setSlot(packet->slot, &packet->item);
		//p->containerMenu->setSlot(packet->slot, packet->item.isNull()? NULL : &packet->item);
	}
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, SetHealthPacket* packet )
{
	std::vector<Player*> all = allPlayers();
	for (unsigned int i = 0; i < all.size(); ++i) {
		Player* p = all[i];
		if (p->owner == source) {
			if (packet->health <= -32) {
				int diff = packet->health - SetHealthPacket::HEALTH_MODIFY_OFFSET;
				if (diff > 0) p->hurt(NULL, diff);
				else if (diff < 0) p->heal(-diff);
			}
			break;
		}
	}
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, SignUpdatePacket* packet ) {
	if (isUnauthenticated(source)) return;
	if (griefBlocked(source)) return;
	redistributePacket(packet, source);
	Level* level = levelOf(source);
	if (!level)
		return;

	TileEntity* te = level->getTileEntity(packet->x, packet->y, packet->z);
	if (TileEntity::isType(te, TileEntityType::Sign)) {
		SignTileEntity* ste = (SignTileEntity*) te;
		if (ste->isEditable()) {
			for (int i = 0; i < SignTileEntity::NUM_LINES; i++) {
				ste->messages[i] = packet->lines[i];
			}
			//ste->setChanged();
		}
	}
}

void ServerSideNetworkHandler::allowIncomingConnections( bool doAllow )
{
	if (doAllow) {
		raknetInstance->announceServer(minecraft->options.username);
	} else {
		raknetInstance->announceServer("");
		// Invisible means closed, not just unlisted: whoever is already in is
		// sent out. Their guids are collected first -- onDisconnect removes the
		// player from the very list being walked.
		std::vector<RakNet::RakNetGUID> visitors;
		{
			std::vector<Player*> all = allPlayers();
			for (unsigned int i = 0; i < all.size(); ++i)
				if (all[i]->owner != rakPeer->GetMyGUID())
					visitors.push_back(all[i]->owner);
		}
		for (unsigned int i = 0; i < _pendingPlayers.size(); ++i)
			visitors.push_back(_pendingPlayers[i]->owner);
		for (unsigned int i = 0; i < visitors.size(); ++i) {
			// Tells them (they get the disconnection screen); a local close
			// raises no event here, so the player is removed by hand.
			rakPeer->CloseConnection(visitors[i], true);
			onDisconnect(visitors[i]);
		}
	}
	_allowIncoming = doAllow;
}

Player* ServerSideNetworkHandler::popPendingPlayer( const RakNet::RakNetGUID& source )
{
	if (!minecraft->level) {
		LOGE("Could not add player since Level is NULL!\n");
		return NULL;
	}

	for (unsigned int i = 0; i < _pendingPlayers.size(); ++i) {
		Player* p = _pendingPlayers[i];
		if (p->owner == source) {
			_pendingPlayers.erase(_pendingPlayers.begin() + i);
			return p;
		}
	}
	return NULL;
}

void ServerSideNetworkHandler::levelEventIn( Level* level, Player* source, int type, int x, int y, int z, int data )
{
	LevelEventPacket packet(type, x, y, z, data);
	redistributeIn(level, &packet, source? source->owner : RakNet::UNASSIGNED_RAKNET_GUID);
}

void ServerSideNetworkHandler::tileEventIn( Level* level, int x, int y, int z, int b0, int b1 )
{
	TileEventPacket packet(x, y, z, b0, b1);
	redistributeIn(level, &packet, RakNet::UNASSIGNED_RAKNET_GUID);
}

Player* ServerSideNetworkHandler::getPlayer( const RakNet::RakNetGUID& source ) {
	std::vector<Level*> all = levels();
	for (unsigned int l = 0; l < all.size(); ++l)
		for (unsigned int i = 0; i < all[l]->players.size(); ++i)
			if (source == all[l]->players[i]->owner) return all[l]->players[i];
    return NULL;
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerSkinPacket* packet)
{
	Level* level = levelOf(source);
	if (!level) return;
	// The sender is whoever it came from, not the id inside it: a client only
	// ever sends its own skin, and must not be able to dress anyone else.
	Player* player = getPlayer(source);
	if (!player) {
		for (unsigned int i = 0; i < _pendingPlayers.size(); ++i)
			if (_pendingPlayers[i]->owner == source) player = _pendingPlayers[i];
	}
	if (!player) return;

	PlayerSkins::release(player->skinId);
	player->skinId = packet->hasSkin()
		? PlayerSkins::create(packet->w, packet->h, &packet->rgba[0])
		: 0;

	// Still pending: onReady_ClientGeneration passes it on with the player.
	if (getPlayer(source) == player)
		sendSkin(player, source, true);
}

/// Sends one player's skin -- or, with none, "default skin" -- to target, or to
/// everyone but target when broadcast is set. The host's own player carries
/// no copy: its skin is the profile's.
void ServerSideNetworkHandler::sendSkin(Player* player, const RakNet::RakNetGUID& target, bool broadcast)
{
	int id = (player == (Player*) minecraft->player) ? PlayerSkins::localSkin : player->skinId;
	const PlayerSkins::Skin* skin = PlayerSkins::get(id);
	// Nobody has seen a skin for a player that never had one; no need to say so.
	if (!skin && !broadcast) return;
	PlayerSkinPacket packet(player->entityId, skin ? skin->w : 0, skin ? skin->h : 0,
	                        skin ? &skin->rgba[0] : 0);
	packet.priority = MEDIUM_PRIORITY;
	packet.reliability = RELIABLE_ORDERED;
	if (broadcast) {
		// To the others in the player's own level.
		redistributeIn(player->level, &packet, target);
		return;
	}
	RakNet::BitStream bs;
	packet.write(&bs);
	rakPeer->Send(&bs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, target, false);
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ChatPacket* packet)
{
	Level* level = levelOf(source);
	if (!level) return;
	Player* player = getPlayer(source);
	if (!player) return;

	// Only printable text, and only as much as the chat box itself takes.
	std::string line;
	for (unsigned int i = 0; i < packet->message.size() && line.size() < (unsigned int) ChatPacket::MAX_CHAT_LENGTH; ++i) {
		const char c = packet->message[i];
		if (c >= 32 && c < 127) line += c;
	}
	if (line.empty()) return;

	// A line starting with '/' is a command, run here for whoever sent it.
	if (line[0] == '/') {
		const std::string commandLine = line.substr(1);
		std::string word, rest;
		const std::string::size_type sp = commandLine.find(' ');
		word = commandLine.substr(0, sp);
		if (sp != std::string::npos) rest = commandLine.substr(sp + 1);
		for (unsigned int i = 0; i < word.size(); ++i) if (word[i] >= 'A' && word[i] <= 'Z') word[i] = (char) (word[i] + 32);

		if (word == "register" || word == "login") {
			handleAuthCommand(player, word, rest);
		} else if (_access.authOn() && !isAuthed(player)) {
			sendMessage(player, "Primero inicia sesion: /login <contrasena> o /register <contrasena>.");
		} else {
			ServerCommands::run(minecraft, this, player, commandLine);
		}
		return;
	}
	if (_access.authOn() && !isAuthed(player)) {
		sendMessage(player, "Primero inicia sesion: /login <contrasena> o /register <contrasena>.");
		return;
	}

	const std::string msg = "<" + player->name + "> " + line;
#ifndef STANDALONE_SERVER
	minecraft->gui.addMessage(msg);
#endif
	ChatPacket out(msg, false);
	raknetInstance->send(out);
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, SetEntityDataPacket* packet)
{
	Level* level = levelOf(source);
	if (!level) return;
	// A visitor reports on its own player -- crouching, mostly -- and no one
	// else's. Level::tick then passes whatever changed on to everyone.
	Player* player = getPlayer(source);
	if (!player || player->entityId != packet->id) return;
	player->getEntityData()->assignValues(&packet->getUnpackedData());
}

//
// Who is here, who may do what.
//

ServerSideNetworkHandler::Session* ServerSideNetworkHandler::sessionOf(const RakNet::RakNetGUID& guid)
{
	for (unsigned int i = 0; i < _sessions.size(); ++i)
		if (_sessions[i].guid == guid) return &_sessions[i];
	return NULL;
}

void ServerSideNetworkHandler::removeSession(const RakNet::RakNetGUID& guid)
{
	for (unsigned int i = 0; i < _sessions.size(); ++i)
		if (_sessions[i].guid == guid) { _sessions.erase(_sessions.begin() + i); return; }
}

bool ServerSideNetworkHandler::isOperator(Player* player)
{
	return player && (player == (Player*) minecraft->player || _access.isOp(player->name));
}

std::vector<Player*> ServerSideNetworkHandler::allPlayers()
{
	// The host's level first, so the host is first.
	std::vector<Player*> all;
	std::vector<Level*> inWorld = levels();
	for (unsigned int l = 0; l < inWorld.size(); ++l)
		all.insert(all.end(), inWorld[l]->players.begin(), inWorld[l]->players.end());
	return all;
}

Player* ServerSideNetworkHandler::findPlayerByName(const std::string& name)
{
	const std::string k = ServerAccess::key(name);
	std::vector<Player*> all = allPlayers();
	for (unsigned int i = 0; i < all.size(); ++i)
		if (ServerAccess::key(all[i]->name) == k) return all[i];
	return NULL;
}

void ServerSideNetworkHandler::sendMessage(Player* to, const std::string& message)
{
	if (!to) return;
	if (to == (Player*) minecraft->player) {
#ifndef STANDALONE_SERVER
		minecraft->gui.addMessage(message);
#endif
		return;
	}
	MessagePacket packet(RakNet::RakString::NonVariadic(message.c_str()));
	raknetInstance->send(to->owner, packet);
}

void ServerSideNetworkHandler::sendExt(Player* to, ExtPacket& packet)
{
	if (!to || to == (Player*) minecraft->player) return;
	raknetInstance->send(to->owner, packet);
}

bool ServerSideNetworkHandler::isAuthed(Player* player)
{
	if (!player || player == (Player*) minecraft->player) return true;
	Session* s = sessionOf(player->owner);
	return !s || s->authed;
}

bool ServerSideNetworkHandler::isUnauthenticated(const RakNet::RakNetGUID& source)
{
	if (!_access.authOn()) return false;
	Session* s = sessionOf(source);
	return s && !s->authed;
}

void ServerSideNetworkHandler::applyGameMode(Player* player, bool creative)
{
	Session* s = sessionOf(player->owner);
	const bool locked = _access.authOn() && s && !s->authed;
	player->abilities.mayfly = creative;
	player->abilities.instabuild = creative;
	// Whoever has not logged in yet cannot be hurt either: they cannot defend themselves.
	player->abilities.invulnerable = creative || locked;
	if (!creative) player->abilities.flying = false;
}

void ServerSideNetworkHandler::setPlayerGameMode(Player* player, bool creative)
{
	if (!player || player == (Player*) minecraft->player) return;
	if (Session* s = sessionOf(player->owner)) s->creative = creative;
	applyGameMode(player, creative);
	ExtPacket packet(ExtPacket::GameMode, creative ? 1 : 0);
	sendExt(player, packet);
}

void ServerSideNetworkHandler::setAuthed(Player* player, bool authed)
{
	Session* s = sessionOf(player->owner);
	if (!s) return;
	s->authed = authed;
	s->idleTicks = 0;
	applyGameMode(player, s->creative);
}

void ServerSideNetworkHandler::handleAuthCommand(Player* player, const std::string& cmd, const std::string& argument)
{
	if (player == (Player*) minecraft->player) {
		sendMessage(player, "El anfitrion no necesita autenticarse.");
		return;
	}
	if (!_access.authOn()) {
		sendMessage(player, "La autenticacion esta desactivada en este mundo.");
		return;
	}
	Session* s = sessionOf(player->owner);
	if (!s) return;
	if (s->authed) {
		sendMessage(player, "Ya iniciaste sesion.");
		return;
	}

	// The password is everything after the command, spaces and all.
	std::string password = argument;
	while (!password.empty() && password[0] == ' ') password.erase(0, 1);

	if (cmd == "register") {
		if (password.size() < 4) {
			sendMessage(player, "Uso: /register <contrasena> (minimo 4 caracteres)");
		} else if (_access.hasAccount(player->name)) {
			sendMessage(player, "Ese nombre ya esta registrado: usa /login <contrasena>.");
		} else {
			_access.registerAccount(player->name, password);
			setAuthed(player, true);
			sendMessage(player, "Registrado. Sesion iniciada.");
		}
		return;
	}

	// login
	if (!_access.hasAccount(player->name)) {
		sendMessage(player, "Ese nombre no esta registrado: usa /register <contrasena>.");
	} else if (_access.checkPassword(player->name, password)) {
		setAuthed(player, true);
		sendMessage(player, "Sesion iniciada.");
	} else if (++s->failures >= 5) {
		kick(player, "Demasiados intentos de contrasena.");
	} else {
		sendMessage(player, "Contrasena incorrecta.");
	}
}

void ServerSideNetworkHandler::requireAuthFromEveryone()
{
	std::vector<Player*> all = allPlayers();
	for (unsigned int i = 0; i < all.size(); ++i) {
		if (all[i] == (Player*) minecraft->player) continue;
		setAuthed(all[i], false);
		sendMessage(all[i], _access.hasAccount(all[i]->name)
			? "El anfitrion activo la autenticacion: usa /login <contrasena>."
			: "El anfitrion activo la autenticacion: usa /register <contrasena>.");
	}
}

void ServerSideNetworkHandler::releaseAuthFromEveryone()
{
	std::vector<Player*> all = allPlayers();
	for (unsigned int i = 0; i < all.size(); ++i)
		if (all[i] != (Player*) minecraft->player) setAuthed(all[i], true);
}

void ServerSideNetworkHandler::kick(Player* player, const std::string& reason)
{
	if (!player || player == (Player*) minecraft->player) return;
	const RakNet::RakNetGUID guid = player->owner;
	ExtPacket packet(ExtPacket::Kick, 0, 0, 0, reason);
	sendExt(player, packet);
	// Tells them (the Kick above is what they show); a local close raises no
	// event here, so the player is removed by hand.
	rakPeer->CloseConnection(guid, true);
	onDisconnect(guid);
}

void ServerSideNetworkHandler::enforceWhitelist()
{
	if (!_access.whitelistOn()) return;
	std::vector<Player*> out;
	std::vector<Player*> all = allPlayers();
	for (unsigned int i = 0; i < all.size(); ++i) {
		Player* p = all[i];
		if (p == (Player*) minecraft->player) continue;
		if (!_access.isWhitelisted(p->name) && !_access.isOp(p->name)) out.push_back(p);
	}
	for (unsigned int i = 0; i < out.size(); ++i)
		kick(out[i], "Ya no estas en la whitelist de este mundo.");
}

void ServerSideNetworkHandler::whitelistChanged()
{
	enforceWhitelist();
	announceLobbyInfo();
}

void ServerSideNetworkHandler::broadcastPlayerList()
{
	std::vector<Player*> all = allPlayers();
	std::string text;
	std::vector<std::string> names;
	// The host first, as in Java's list.
	if (minecraft->player) { names.push_back(minecraft->player->name); text += minecraft->player->name; }
	for (unsigned int i = 0; i < all.size(); ++i) {
		if (all[i] == (Player*) minecraft->player) continue;
		names.push_back(all[i]->name);
		if (!text.empty()) text += "\n";
		text += all[i]->name;
	}
	minecraft->playerNames = names;
	ExtPacket packet(ExtPacket::PlayerList, 0, 0, 0, text);
	for (unsigned int i = 0; i < all.size(); ++i)
		if (all[i] != (Player*) minecraft->player)
			raknetInstance->send(all[i]->owner, packet);
}

void ServerSideNetworkHandler::announceLobbyInfo()
{
#if defined(MC_WASM)
	mcpeLobbySetInfo(_access.whitelistOn() ? 1 : 0, (int) allPlayers().size(), MAX_PLAYERS);
#endif
}

void ServerSideNetworkHandler::tick()
{
	for (unsigned int i = 0; i < _sessions.size(); ++i)
		if (_sessions[i].griefNotice > 0) --_sessions[i].griefNotice;

	// The other dimension stops running when nobody is in it and nobody is on
	// the way there.
	if (minecraft->otherLevel && minecraft->otherLevel->players.empty()) {
		bool arriving = false;
		for (unsigned int i = 0; i < _pendingPlayers.size(); ++i)
			if (_pendingPlayers[i]->level == minecraft->otherLevel) arriving = true;
		if (!arriving)
			minecraft->destroyOtherLevel();
	}

	// Whoever has not logged in within a minute is sent out; reminded every 15 seconds.
	std::vector<Player*> late;
	std::vector<Player*> all = allPlayers();
	for (unsigned int i = 0; i < all.size(); ++i) {
		Player* p = all[i];
		if (p == (Player*) minecraft->player) continue;
		Session* s = sessionOf(p->owner);
		if (!s || s->authed || !_access.authOn()) continue;
		++s->idleTicks;
		if (s->idleTicks >= 60 * 20) late.push_back(p);
		else if (s->idleTicks % (15 * 20) == 0)
			sendMessage(p, _access.hasAccount(p->name) ? "Usa /login <contrasena>." : "Usa /register <contrasena>.");
	}
	for (unsigned int i = 0; i < late.size(); ++i)
		kick(late[i], "Tiempo agotado para iniciar sesion.");

	// A player count that moved without anyone joining or leaving through here
	// (a visitor removed by the allow-incoming switch, say) still reaches the lobby.
	const int count = (int) all.size();
	if (count != _lastPlayerCount) {
		_lastPlayerCount = count;
		announceLobbyInfo();
	}
}
