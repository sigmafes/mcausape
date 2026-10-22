
#include "ClientSideNetworkHandler.h"
#include "packet/PacketInclude.h"
#include "RakNetInstance.h"
#include "../world/level/chunk/ChunkSource.h"
#include "../world/level/Level.h"
#include "../world/level/storage/LevelStorageSource.h"
#include "../world/entity/player/Player.h"
#include "../world/entity/player/Inventory.h"
#include "../client/Minecraft.h"
#include "../client/gamemode/GameMode.h"
#ifndef STANDALONE_SERVER
#include "../client/gui/screens/DisconnectionScreen.h"
#endif
#include "../client/player/LocalPlayer.h"
#include "../client/multiplayer/MultiPlayerLevel.h"
#include "../client/multiplayer/ClientChunkSource.h"
#include "../client/player/input/KeyboardInput.h"
#include "../client/sound/SoundEngine.h"
#include "../world/entity/MobFactory.h"
#include "../raknet/RakPeerInterface.h"
#include "../world/level/Explosion.h"
#include "../world/level/tile/entity/FurnaceTileEntity.h"
#include "../world/inventory/BaseContainerMenu.h"
#ifndef STANDALONE_SERVER
#include "../client/particle/TakeAnimationParticle.h"
#endif
#include "../world/entity/EntityFactory.h"
#include "../world/entity/item/PrimedTnt.h"
#include "../world/entity/projectile/Arrow.h"
#include "../world/level/tile/entity/ChestTileEntity.h"
#include "../client/player/RemotePlayer.h"
#include "../world/level/tile/LevelEvent.h"
#include "../world/entity/item/FallingTile.h"
#include "../client/skin/PlayerSkins.h"
#include "../world/level/dimension/Dimension.h"
#ifndef STANDALONE_SERVER
#include "../client/gui/screens/ProgressScreen.h"
#include "../client/renderer/LevelRenderer.h"
#include "../client/particle/ParticleEngine.h"
#endif

static MultiPlayerLevel* mpcast(Level* l) { return (MultiPlayerLevel*) l; }

ClientSideNetworkHandler::ClientSideNetworkHandler(Minecraft* minecraft, IRakNetInstance* raknetInstance)
:	minecraft(minecraft),
	raknetInstance(raknetInstance),
	level(NULL),
	_kicked(false),
	_holding(false),
	_gotPlayerData(false),
	_syncTicks(0),
	_holdX(0), _holdY(0), _holdZ(0),
	numChunksLoaded(0)
{
	rakPeer = raknetInstance->getPeer();
}

ClientSideNetworkHandler::~ClientSideNetworkHandler()
{
	// Everyone else's skins came from this host and go with it.
	PlayerSkins::releaseAllBut(PlayerSkins::localSkin);
}

bool ClientSideNetworkHandler::areAllChunksLoaded()
{
	// Counted on arrival, not on request. With MaxInFlightChunkRequests out at
	// once, "the last one has been asked for" comes that many replies before
	// "the last one is here" -- and this is what decides when buffered block
	// updates are applied and READY_REQUESTEDCHUNKS is sent.
	return numChunksLoaded >= NumRequestChunks;
}

bool ClientSideNetworkHandler::isChunkLoaded(int x, int z)
{
	if (x < 0 || x >= CHUNK_CACHE_WIDTH || z < 0 || z >= CHUNK_CACHE_WIDTH) {
		LOGE("Error: Tried to request chunk (%d, %d)\n", x, z);
		return true;
	}
	return chunksLoaded[x * CHUNK_CACHE_WIDTH + z];
	//return areAllChunksLoaded();
}

void ClientSideNetworkHandler::onConnect(const RakNet::RakNetGUID& hostGuid)
{
	LOGI("onConnect, server guid: %s, local guid: %s\n", hostGuid.ToString(), rakPeer->GetMyGUID().ToString());
	serverGuid = hostGuid;

	clearChunksLoaded();
	LoginPacket packet(minecraft->user->name.c_str(), SharedConstants::NetworkProtocolVersion);
	raknetInstance->send(packet);
}

void ClientSideNetworkHandler::onUnableToConnect()
{
	LOGI("onUnableToConnect\n");
}

void ClientSideNetworkHandler::onDisconnect(const RakNet::RakNetGUID& guid)
{
	LOGI("onDisconnect\n");
	// A client's only connection is to the host, so this is the host going
	// away: leaving its world, closing its tab, or timing out. 0.6.1 turned
	// the joined world into a local one here and let the player carry on
	// alone in a copy nobody else could reach; now the visit ends.
	//
	// The screen's button is what calls leaveGame(), not this: runEvents() is
	// still walking packets through this handler, and leaveGame() deletes it.
	// Staying client-side matters too -- leaveGame() saves a level that is not.
#ifndef STANDALONE_SERVER
	if (!_kicked)
		minecraft->setScreen(new DisconnectionScreen("The host has left the world"));
#endif
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, LoginStatusPacket* packet) {
	if (packet->status == LoginStatus::Success) {
		raknetInstance->setIsLoggedIn(true);
		return;
	}

	if (packet->status == LoginStatus::Failed_ClientOld) {
		LOGI("Disconnect! Client is outdated!\n");
#ifndef STANDALONE_SERVER
		minecraft->setScreen(new DisconnectionScreen("Could not connect: Outdated client!"));
#endif
	}
	if (packet->status == LoginStatus::Failed_ServerOld) {
		LOGI("Disconnect! Server is outdated!\n");
#ifndef STANDALONE_SERVER
		minecraft->setScreen(new DisconnectionScreen("Could not connect: Outdated server!"));
#endif
	}

	// The host's own rules. Its connection stays open until the player leaves
	// the screen, so the screen is not replaced by "the host has left".
	const char* refusal = NULL;
	if (packet->status == LoginStatus::Failed_BadName) refusal = "Could not connect: your name is not allowed (letters, digits, _ and -, up to 16)";
	if (packet->status == LoginStatus::Failed_NameTaken) refusal = "Could not connect: someone with your name is already in this world";
	if (packet->status == LoginStatus::Failed_NotWhitelisted) refusal = "Could not connect: this world has a whitelist and you are not on it";
	if (packet->status == LoginStatus::Failed_Full) refusal = "Could not connect: this world is full (10 players)";
	if (refusal) {
		_kicked = true;
#ifndef STANDALONE_SERVER
		minecraft->setScreen(new DisconnectionScreen(refusal));
#endif
	}
}


void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, StartGamePacket* packet)
{
	LOGI("StartGamePacket\n");

#ifdef RPI
	if (packet->gameType != GameType::Creative) {
		minecraft->setScreen(new DisconnectionScreen("Could not connect: Incompatible server!"));
		return;
	}
#endif

	// A second StartGamePacket is the host sending us through a portal: the
	// same steps as joining, for the other dimension, with the player we have.
	const bool transfer = (minecraft->level != NULL && minecraft->player != NULL);
	LocalPlayer* travelling = NULL;
	if (transfer) {
		// Nothing the old level says is for us any more, and its entities and
		// the others' skins stay behind with it.
		this->level = NULL;
		_holding = false;
		clearChunksLoaded();
		bufferedBlockUpdates.clear();
		PlayerSkins::releaseAllBut(PlayerSkins::localSkin);

		Level* old = minecraft->level;
		travelling = minecraft->player;
		old->detachEntityForTransfer(travelling);
#ifndef STANDALONE_SERVER
		minecraft->levelRenderer->setLevel(NULL);
		minecraft->particleEngine->setLevel(NULL);
#endif
		LevelStorage* oldStorage = old->getLevelStorage();
		delete old;
		delete oldStorage;
		minecraft->level = NULL;
		// Held aside, as Minecraft::setLevel does for a new player: it is added
		// to the level once the level exists.
		minecraft->player = NULL;
		minecraft->cameraTargetPlayer = NULL;
	}

	const std::string& levelId = LevelStorageSource::TempLevelId;
	LevelStorageSource* storageSource = minecraft->getLevelSource();
	storageSource->deleteLevel(levelId);
	//level = new Level(storageSource->selectLevel(levelId, true), "temp", packet->levelSeed, SharedConstants::StorageVersion);
	Dimension* fixedDimension = (packet->dimension == Dimension::NETHER || packet->dimension == Dimension::AQUARIUM) ? Dimension::getNew(packet->dimension) : NULL;
	MultiPlayerLevel* level = new MultiPlayerLevel(
		storageSource->selectLevel(levelId, true),
		"temp",
		LevelSettings(packet->levelSeed, LevelSettings::validateGameType(packet->gameType)),
		SharedConstants::StorageVersion,
		fixedDimension);
	level->isClientSide = true;
	// Holds the host's chunks; generates nothing (see ClientChunkSource).
	level->setChunkSource(new ClientChunkSource(level));

	bool isCreative = (packet->gameType == GameType::Creative);
	LocalPlayer* player = travelling;
	if (player) {
		player->setLevel(level);
		player->dimension = level->dimension->id;
	} else {
		player = new LocalPlayer(minecraft, level, minecraft->user, level->dimension->id, isCreative);
		player->owner = rakPeer->GetMyGUID();
	}
	player->entityId = packet->entityId;
	player->moveTo(packet->x, packet->y, packet->z, player->yRot, player->xRot);

	LOGI("new pos: %f, %f [%f - %f]\n", player->x, player->z, player->bb.y0, player->bb.y1);

#ifndef STANDALONE_SERVER
	if (transfer)
		minecraft->setScreen(new ProgressScreen());
#endif
	minecraft->setLevel(level, "ClientSideNetworkHandler -> setLevel", player);
	minecraft->setIsCreativeMode(isCreative);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, MessagePacket* packet)
{
	LOGI("MessagePacket\n");
#ifndef STANDALONE_SERVER
	minecraft->gui.addMessage(packet->message.C_String());
#endif
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, SetTimePacket* packet)
{
	if (!level)
		return;

	LOGI("SetTimePacket\n");
	level->setTime(packet->time);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, AddEntityPacket* packet)
{
	if (!level)
		return;

	Entity* e = EntityFactory::CreateEntity(packet->type, level);
	if (!e)
		return;

	e->entityId = packet->entityId;
	e->setPos(packet->x, packet->y, packet->z);

	// Entity Specific stuff here
	switch (packet->type)
	{
		case EntityTypes::IdFallingTile: {
			int data = -packet->data();
            FallingTile* ft = (FallingTile*) e;
            ft->tile = data & 0xff;
            ft->data = data >> 16;
		}
		case EntityTypes::IdArrow: {
			Entity* owner = level->getEntity(packet->data());
			if (owner && owner->isMob())
				((Arrow*)e)->ownerId = owner->entityId;
			break;
		}
		default:
			break;
	}

    if (packet->hasMovementData()) {
		/*
		e->xd = packet->xd;
		e->yd = packet->yd;
		e->zd = packet->zd;
		*/
		e->lerpMotion(packet->xd, packet->yd, packet->zd);
		//LOGI("Client: reading entity with %f, %f, %f\n", e->xd, e->yd, e->zd);
	}

	mpcast(level)->putEntity(packet->entityId, e);
}
void ClientSideNetworkHandler::handle( const RakNet::RakNetGUID& source, AddPaintingPacket* packet ) {
	if (!level)
		return;

	Painting* painting = new Painting(level, packet->xTile, packet->yTile, packet->zTile, packet->dir, packet->motive);
	mpcast(level)->putEntity(packet->entityId, painting);
}
void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, AddMobPacket* packet)
{
	LOGI("AddMobPacket (%p)\n", level);

	if (!level) {
		LOGW("Trying to add a mob with no level!\n");
		//we skip this since we will get this player anyway when we request level
		return;
	}
	if (!packet->type) {
		LOGE("Trying to add a mob without a type id\n");
		return;
	}

	Mob* mob = MobFactory::CreateMob(packet->type, level);
	if (!mob) {
		LOGE("Server tried to add an unknown mob type! :%d\n", packet->type);
		return;
	}

	mob->entityId = packet->entityId;
	mob->moveTo(packet->x, packet->y, packet->z, packet->yRot, packet->xRot);
	mob->getEntityData()->assignValues(&packet->unpack);
	level->addEntity(mob);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, AddPlayerPacket* packet)
{
	if (!level) {
		//we skip this since we will get this player anyway when we request level
		return;
	}
	LOGI("AddPlayerPacket\n");

	Player* player = new RemotePlayer(level, minecraft->isCreativeMode());
	minecraft->gameMode->initAbilities(player->abilities);
	player->entityId = packet->entityId;
	level->addEntity(player);

	player->moveTo(packet->x, packet->y, packet->z, packet->yRot, packet->xRot);
	player->name = packet->name.C_String();
	player->owner = packet->owner;
	player->getEntityData()->assignValues(&packet->unpack);
    int slot = Inventory::MAX_SELECTION_SIZE;
    if (packet->carriedItemId == 0) {
        player->inventory->clearSlot(slot);
    } else {
        ItemInstance newItem(packet->carriedItemId, 1, packet->carriedItemAuxValue);
        player->inventory->replaceSlot(slot, &newItem);
    }
    player->inventory->moveToSelectedSlot(slot, true);
	//player->resetPos();

	// Someone already in the world's list only came into this dimension.
	std::string message = packet->name.C_String();
	bool known = false;
	for (unsigned int i = 0; i < minecraft->playerNames.size(); ++i)
		if (minecraft->playerNames[i] == message) known = true;
	message += " joined the game";
#ifndef STANDALONE_SERVER
	if (!known)
		minecraft->gui.addMessage(message);
#endif
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, RemovePlayerPacket* packet) {
	if (!level || source == minecraft->player->owner) return;

	if (Player* player = findPlayer(level, packet->entityId, &packet->owner)) {
		PlayerSkins::release(player->skinId);
		player->skinId = 0;
		player->reallyRemoveIfPlayer = true;
		level->removeEntity(player);
	}

	//for (int i = (int)level->players.size()-1; i >= 0; --i) {
	//	if (level->players[i]->owner == source) {
	//		level->players[i]->reallyRemoveIfPlayer = true;
	//		level->players.erase(level->players.begin() + i);
	//		break;
	//	}
	//}

	//if (!player) return;

	//std::string message = packet->name.C_String();
	//message += " joined the game";
	//minecraft->gui.addMessage(message);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, RemoveEntityPacket* packet)
{
	if (!level) return;

	Entity* entity = level->getEntity(packet->entityId);
	LOGI("RemoveEntityPacket %p %p, %d\n", entity, minecraft->player, entity?(int)(entity->isPlayer()): -1);
	if (!entity) return;

	if (entity->isPlayer() && entity != minecraft->player) {
		PlayerSkins::release(((Player*) entity)->skinId);
		((Player*) entity)->skinId = 0;
	}
	level->removeEntity(entity);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, AddItemEntityPacket* packet)
{
	if (!level) return;

	ItemEntity* entity = new ItemEntity(level, packet->x, packet->y, packet->z, ItemInstance(packet->itemId, packet->itemCount, packet->auxValue));
	entity->xd = packet->xa();
	entity->yd = packet->ya();
	entity->zd = packet->za();
	//LOGI("item-entity @ client: %f, %f, %f\n", entity->xd, entity->yd, entity->zd);

	mpcast(level)->putEntity(packet->id, entity);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, TakeItemEntityPacket* packet) {
	if (!level) return;

	Entity* e = level->getEntity(packet->itemId);
	if (!e)
		return;
	
	ItemInstance item;
	if (e->isItemEntity()) {
		item = ((ItemEntity*) e)->item;
#ifndef STANDALONE_SERVER
		if (Entity* to = level->getEntity(packet->playerId))
			minecraft->particleEngine->add(new TakeAnimationParticle(level, (ItemEntity*)e, to, -0.5f));
#endif
	}
	else if (e->getEntityTypeId() == EntityTypes::IdArrow)
		item = ItemInstance(Item::arrow);

	if (item.isNull())
		return;

	// try take it and if we don't have space; re-throw it
	if (minecraft->player->entityId == packet->playerId
	&& !minecraft->player->inventory->add(&item)) {
		DropItemPacket dropPacket(packet->playerId, item);
		minecraft->raknetInstance->send(dropPacket);
	}
	level->playSound(e, "random.pop", 0.2f, 1.0f * 2.f);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, MovePlayerPacket* packet)
{
	if (!level)
		return;

	//printf("MovePlayerPacket\n");
	Entity* entity = level->getEntity(packet->entityId);
	if (entity)
	{
		entity->lerpTo(packet->x, packet->y, packet->z, packet->yRot, packet->xRot, 3);
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, MoveEntityPacket* packet)
{
	if (!level)
		return;

	//printf("MovePlayerPacket\n");
	Entity* entity = level->getEntity(packet->entityId);
	if (entity)
	{
		float xRot = packet->hasRot? packet->xRot : entity->xRot;
		float yRot = packet->hasRot? packet->yRot : entity->yRot;
		entity->lerpTo(packet->x, packet->y, packet->z, yRot, xRot, 3);
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, UpdateBlockPacket* packet)
{
	if (!level) return;

	//LOGI("UpdateBlockPacket @ %d, %d, %d\n", packet->x, packet->y, packet->z);

	int x = packet->x, z = packet->z;

	if (isChunkLoaded(x >> 4, z >> 4))
	{
		//LOGI("chunk is loaded - UPDATE @ %d, %d, %d -- %d, %d\n", x, packet->y, z, packet->blockId, packet->blockData);

		int y = packet->y;
		int tileId = Tile::transformToValidBlockId(packet->blockId, x, y, z);
		level->setTileAndData(x, y, z, tileId, packet->blockData & 15);
		level->setWaterlogged(x, y, z, (packet->blockData & UpdateBlockPacket::WaterloggedBit) != 0);
	} else {
		SBufferedBlockUpdate update;
		update.blockId = packet->blockId;
		update.blockData = packet->blockData;
		update.setData = true;
		update.x = packet->x;
		update.y = packet->y;
		update.z = packet->z;
		bufferedBlockUpdates.push_back(update);
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ExplodePacket* packet) {
	if (!level) return;
	Explosion explosion(level, NULL, packet->x, packet->y, packet->z, packet->r);
	explosion.toBlow.insert(packet->toBlow.begin(), packet->toBlow.end());
	explosion.finalizeExplosion();
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, LevelEventPacket* packet) {
	if (!level) return;
	if(packet->eventId == LevelEvent::ALL_PLAYERS_SLEEPING) {
		minecraft->player->setAllPlayersSleeping();
	}
	else {
		minecraft->level->levelEvent(NULL, packet->eventId, packet->x, packet->y, packet->z, packet->data);
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, TileEventPacket* packet) {
	if (!level) return;
	minecraft->level->tileEvent(packet->x, packet->y, packet->z, packet->b0, packet->b1);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, EntityEventPacket* packet) {
	if (!level) return;

	Entity* e = level->getEntity(packet->entityId);
	if (e) e->handleEntityEvent(packet->eventId);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ChunkDataPacket* packet)
{
	if (!level) return;
	if (packet->x < 0 || packet->z < 0 || packet->x >= CHUNK_CACHE_WIDTH || packet->z >= CHUNK_CACHE_WIDTH)
		return;

	// The host's chunk, whole, over the blank one (ClientChunkSource).
	LevelChunk* chunk = level->getChunk(packet->x, packet->z);
	if (!chunk || !packet->apply(chunk)) {
		LOGI("Could not read chunk (%d, %d)\n", packet->x, packet->z);
		return;
	}
	chunk->recalcHeightmapOnly();
	chunk->unsaved = false;
	const int rx = packet->x << 4, rz = packet->z << 4;
	level->setTilesDirty(rx, 0, rz, rx + 15, LEVEL_HEIGHT - 1, rz + 15);

	bool& loaded = chunksLoaded[packet->x * CHUNK_CACHE_WIDTH + packet->z];
	if (!loaded) {
		loaded = true;
		++numChunksLoaded;
	}

	// Changes that came for this chunk before it did go in now.
	for (unsigned int i = 0; i < bufferedBlockUpdates.size(); ) {
		const SBufferedBlockUpdate& update = bufferedBlockUpdates[i];
		if ((update.x >> 4) == packet->x && (update.z >> 4) == packet->z) {
			int tileId = Tile::transformToValidBlockId(update.blockId, update.x, update.y, update.z);
			if (update.setData) {
				level->setTileAndData(update.x, update.y, update.z, tileId, update.blockData & 15);
				level->setWaterlogged(update.x, update.y, update.z, (update.blockData & UpdateBlockPacket::WaterloggedBit) != 0);
			}
			else
				level->setTile(update.x, update.y, update.z, tileId);
			bufferedBlockUpdates.erase(bufferedBlockUpdates.begin() + i);
		} else {
			++i;
		}
	}

	if (numChunksLoaded == NumRequestChunks) {
		ReadyPacket ready(ReadyPacket::READY_REQUESTEDCHUNKS);
		raknetInstance->send(ready);
	}
}

/// Every chunk within RingRadius of the player has arrived: it can move.
bool ClientSideNetworkHandler::ringLoaded()
{
	Player* p = minecraft->player;
	if (!p) return false;
	const int cx = Mth::floor(p->x / 16.0f), cz = Mth::floor(p->z / 16.0f);
	for (int x = cx - RingRadius; x <= cx + RingRadius; ++x)
		for (int z = cz - RingRadius; z <= cz + RingRadius; ++z)
			if (x >= 0 && z >= 0 && x < CHUNK_CACHE_WIDTH && z < CHUNK_CACHE_WIDTH && !chunksLoaded[x * CHUNK_CACHE_WIDTH + z])
				return false;
	return true;
}

void ClientSideNetworkHandler::levelGenerated(Level* level)
{
	this->level = level;
	clearChunksLoaded();
	ReadyPacket packet(ReadyPacket::READY_CLIENTGENERATION);
	raknetInstance->send(packet);

	// Our skin, for the host to keep on our player and pass on. Sent here, the
	// first moment we know our entity id is the one the host gave us.
	if (const PlayerSkins::Skin* skin = PlayerSkins::get(PlayerSkins::localSkin)) {
		PlayerSkinPacket skinPacket(minecraft->player->entityId, skin->w, skin->h, &skin->rgba[0]);
		raknetInstance->send(skinPacket);
	}

	// The host sends the chunks now, nearest first; the player is held where
	// it was put until the ones around it are here, or it would fall.
	_holding = true;
	if (minecraft->player) {
		_holdX = minecraft->player->x;
		_holdY = minecraft->player->y;
		_holdZ = minecraft->player->z;
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerEquipmentPacket* packet)
{
	if (!level)
		return;

	Entity* entity = level->getEntity(packet->entityId);
	if (!entity || !entity->isPlayer())
		return;

	Player* player = (Player*)entity;
	// make sure it's not our local player
	if (player->owner == rakPeer->GetMyGUID())
	{
		printf("Attempted to modify local player's inventory\n");
		return;
	}
	// override the player's inventory
	//int slot = player->inventory->getSlot(packet->itemId, packet->itemAuxValue);
	//if (slot >= 0) {
	//	player->inventory->moveToSelectedSlot(slot, true);
		//item->id = packet->itemId;
		//item->setAuxValue(packet->itemAuxValue);
		//item->count = 63;
	int slot = Inventory::MAX_SELECTION_SIZE;
	if (slot >= 0) {
		if (packet->itemId == 0) {
			player->inventory->clearSlot(slot);
		} else {
			ItemInstance newItem(packet->itemId, 63, packet->itemAuxValue);
			player->inventory->replaceSlot(slot, &newItem);
		}
		player->inventory->moveToSelectedSlot(slot, true);
	} else {
		LOGW("Warning: Remote player doesn't have his thing, Odd!\n");
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerArmorEquipmentPacket* packet) {
	if (!level)
		return;

	Entity* entity = level->getEntity(packet->entityId);
	if (!entity || !entity->isPlayer())
		return;

	Player* player = (Player*)entity;
	// make sure it's not our local player, since that should be UpdateArmorPacket
	if (player->owner == rakPeer->GetMyGUID()) {
		printf("Attempted to modify local player's armor visually\n");
		return;
	}

	packet->fillIn(player);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, InteractPacket* packet)
{
	if (!level)
		return;

	Entity* src = level->getEntity(packet->sourceId);
	Entity* entity = level->getEntity(packet->targetId);
	if (src && entity && src->isPlayer())
	{
		Player* player = (Player*) src;
		if (InteractPacket::Attack == packet->action)
			minecraft->gameMode->attack(player, entity);
		if (InteractPacket::Interact == packet->action)
			minecraft->gameMode->interact(player, entity);
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, SetEntityDataPacket* packet)
{
	if (!level)
		return;

	LOGI("SetEntityDataPacket\n");

	Entity* e = level->getEntity(packet->id);
	if (e) {
		SynchedEntityData* data = e->getEntityData();
		if (data)
			data->assignValues(&packet->getUnpackedData());
	}
}

void ClientSideNetworkHandler::handle( const RakNet::RakNetGUID& source, SetEntityMotionPacket* packet )
{
	if (!level)
		return;

	if (Entity* e = level->getEntity(packet->id)) {
		/*
		e->xd = packet->xd;
		e->yd = packet->yd;
		e->zd = packet->zd;
		*/
		e->lerpMotion(packet->xd, packet->yd, packet->zd);
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, AnimatePacket* packet)
{
	if (!level)
		return;

    // Own player - Then don't play... :
    if (minecraft->player->entityId == packet->entityId) {
        if (packet->action == AnimatePacket::Swing) return;
    }

	Entity* entity = level->getEntity(packet->entityId);
	if (!entity || !entity->isPlayer())
        return;
    
	Player* player = (Player*) entity;

    switch (packet->action) {
	case AnimatePacket::Swing:
		player->swing();
		break;
	case AnimatePacket::WAKE_UP:
		player->stopSleepInBed(false, false, false);
		break;
	default:
		LOGW("Unknown Animate action: %d\n", packet->action);
		break;
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, UseItemPacket* packet)
{
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, SetHealthPacket* packet)
{
	if (!level || !minecraft->player)
		return;

	minecraft->player->hurtTo(packet->health);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, SetSpawnPositionPacket* packet) {
	if (!level || !minecraft || !minecraft->player) return;
	if (!level->inRange(packet->x, packet->y, packet->z)) return;

	minecraft->player->setRespawnPosition(Pos(packet->x, packet->y, packet->z));
	level->getLevelData()->setSpawn(packet->x, packet->y, packet->z);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, HurtArmorPacket* packet) {
    if (!level || !minecraft->player) {
        return;
    }

    minecraft->player->hurtArmor(packet->dmg);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, RespawnPacket* packet)
{
	if (level) {
		//LOGI("RespawnPacket! %d\n", findPlayer(level, packet->entityId, NULL));
		NetEventCallback::handle(level, source, packet );
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ContainerOpenPacket* packet)
{
	if (!level || !minecraft || !minecraft->player)
		return;

	if (packet->type == ContainerType::FURNACE) {
		FurnaceTileEntity* te = new FurnaceTileEntity();
		te->clientSideOnly = true;
		minecraft->player->openFurnace(te);
		if (minecraft->player->containerMenu)
			minecraft->player->containerMenu->containerId = packet->containerId;
	}
	if (packet->type == ContainerType::CONTAINER) {
		ChestTileEntity* te = new ChestTileEntity();
		te->clientSideOnly = true;
		minecraft->player->openContainer(te);
		if (minecraft->player->containerMenu)
			minecraft->player->containerMenu->containerId = packet->containerId;
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ContainerClosePacket* packet)
{
	if (minecraft && minecraft->player && minecraft->player->containerMenu)
		minecraft->player->closeContainer();
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ContainerSetContentPacket* packet)
{
	if (!minecraft || !minecraft->player)
		return;

    if (packet->containerId == 0) {
        for (unsigned int i = 0; i < packet->items.size(); ++i) {
            minecraft->player->inventory->setItem(Inventory::MAX_SELECTION_SIZE + i, &packet->items[i]);
        }
    } else if (minecraft->player->containerMenu && minecraft->player->containerMenu->containerId == packet->containerId) {
        for (unsigned int i = 0; i < packet->items.size(); ++i) {
            minecraft->player->containerMenu->setSlot(i, &packet->items[i]);
        }
    }
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ContainerSetSlotPacket* packet)
{
	//LOGI("ContainerSetSlot\n");

	if (!minecraft->player
	  || !minecraft->player->containerMenu
	  || minecraft->player->containerMenu->containerId != packet->containerId)
	  return;

	//minecraft->player->containerMenu->setSlot(packet->slot, packet->item.isNull()? NULL : &packet->item);
	minecraft->player->containerMenu->setSlot(packet->slot, &packet->item);
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ContainerSetDataPacket* packet)
{
	//LOGI("ContainerSetData\n");
	if (minecraft->player && minecraft->player->containerMenu && minecraft->player->containerMenu->containerId == packet->containerId) {
		//LOGI("client: SetData2 %d, %d\n", packet->id, packet->value);
		minecraft->player->containerMenu->setData(packet->id, packet->value);
	}
}

void ClientSideNetworkHandler::handle( const RakNet::RakNetGUID& source, ChatPacket* packet )
{
#ifndef STANDALONE_SERVER
	minecraft->gui.displayClientMessage(packet->message);
#endif
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, SignUpdatePacket* packet)
{
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

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, AdventureSettingsPacket* packet) {
    if (!level)
		return;
	//assert(level != NULL && "level is NULL @ handle(AdventureSettingsPacket*)");

    packet->fillIn(level->adventureSettings);
}

/** What the host keeps for our name in this world, sent once on joining:
    the inventory, armour and health we had. The position is already ours
    (StartGamePacket put us there), and so is the dimension. */
void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerDataPacket* packet)
{
	LocalPlayer* p = minecraft->player;
	if (!p || packet->entityId != p->entityId) return;
	_gotPlayerData = true;
	_syncTicks = 0;

	CompoundTag* tag = PlayerDataPacket::fromBytes(packet->bytes);
	if (!tag) return;	// a name new to this world
	const float x = p->x, y = p->y, z = p->z, yRot = p->yRot, xRot = p->xRot;
	const int dimension = p->dimension;
	p->load(tag);
	tag->deleteChildren();
	delete tag;
	p->xo = p->xOld = x; p->yo = p->yOld = y; p->zo = p->zOld = z;
	p->setPos(x, y, z);
	p->yRot = p->yRotO = yRot;
	p->xRot = p->xRotO = xRot;
	p->xd = p->yd = p->zd = 0;
	p->dimension = dimension;
	// In creative the inventory is the item palette, not what was carried.
	if (p->inventory->isCreative()) p->inventory->clearInventoryWithDefault();
	if (p->health <= 0) p->health = p->getMaxHealth();
}

void ClientSideNetworkHandler::sendPlayerData()
{
	LocalPlayer* p = minecraft->player;
	if (!_gotPlayerData || !level || !p || p->health <= 0) return;
	CompoundTag tag;
	p->saveWithoutId(&tag);
	// The palette is not ours to keep; the host holds on to the survival one.
	if (p->inventory->isCreative()) tag.remove("Inventory");
	PlayerDataPacket packet(p->entityId, PlayerDataPacket::toBytes(&tag));
	tag.deleteChildren();
	packet.priority = MEDIUM_PRIORITY;
	packet.reliability = RELIABLE_ORDERED;
	raknetInstance->send(packet);
}

void ClientSideNetworkHandler::tick()
{
	if (_gotPlayerData && level && minecraft->player && ++_syncTicks >= SyncTicks) {
		_syncTicks = 0;
		sendPlayerData();
	}
	if (!_holding || !level || !minecraft->player) return;
	if (ringLoaded()) {
		_holding = false;
		return;
	}
	LocalPlayer* p = minecraft->player;
	p->setPos(_holdX, _holdY, _holdZ);
	p->xd = p->yd = p->zd = 0;
	p->fallDistance = 0;
}

void ClientSideNetworkHandler::clearChunksLoaded()
{
	// Init the chunk positions. CHUNK_CACHE_WIDTH (the world's width in chunks),
	// not CHUNK_WIDTH (a chunk's width in blocks): they were both 16 in 0.6.1,
	// so the wrong one worked until the world grew to 32 chunks a side, after
	// which this asked for x 0..63, z 0..15 -- half the world never synced and
	// the other half was requested out of range.
	for (int i = 0; i < NumRequestChunks; ++i) {
		chunksLoaded[i] = false;
	}
	numChunksLoaded = 0;
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ExtPacket* packet)
{
	switch (packet->kind) {
	case ExtPacket::GameMode:
		// The host changed our game mode (/gamemode <s|c> <us>).
		minecraft->setIsCreativeMode(packet->a != 0);
		break;
	case ExtPacket::PlayerList: {
		minecraft->playerNames.clear();
		const std::string& text = packet->text;
		std::string::size_type pos = 0;
		while (pos <= text.size() && !text.empty()) {
			std::string::size_type end = text.find('\n', pos);
			if (end == std::string::npos) end = text.size();
			if (end > pos) minecraft->playerNames.push_back(text.substr(pos, end - pos));
			pos = end + 1;
		}
		break;
	}
	case ExtPacket::Give:
		// /give run by the host for us: our inventory is ours to fill.
		if (minecraft->player && packet->a > 0 && packet->a < Item::MAX_ITEMS && Item::items[packet->a])
			minecraft->player->inventory->add(new ItemInstance(packet->a, packet->b, packet->c));
		break;
	case ExtPacket::AuthLock:
		minecraft->inputLocked = (packet->a != 0);
		break;
	case ExtPacket::Kick:
		_kicked = true;
#ifndef STANDALONE_SERVER
		minecraft->setScreen(new DisconnectionScreen(packet->text));
#endif
		break;
	}
}

void ClientSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerSkinPacket* packet)
{
	if (!level) return;
	// The host sends each player's skin after that player's AddPlayerPacket, on
	// the same ordered channel, so the player is always here by now.
	Entity* entity = level->getEntity(packet->entityId);
	if (!entity || !entity->isPlayer() || entity == minecraft->player)
		return;
	Player* player = (Player*) entity;
	PlayerSkins::release(player->skinId);
	player->skinId = packet->hasSkin()
		? PlayerSkins::create(packet->w, packet->h, &packet->rgba[0])
		: 0;
}
