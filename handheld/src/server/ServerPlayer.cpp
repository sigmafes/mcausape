#include "../network/packet/ExtPacket.h"
#include "ServerPlayer.h"
#include <cmath>
#include "../network/packet/SetEntityMotionPacket.h"
#include "../world/level/Level.h"

#include "../network/RakNetInstance.h"
#include "../network/packet/ContainerOpenPacket.h"
#include "../world/level/tile/entity/BrewingStandTileEntity.h"
#include "../network/packet/ContainerClosePacket.h"
#include "../network/packet/ContainerSetDataPacket.h"
#include "../network/packet/ContainerSetSlotPacket.h"
#include "../network/packet/ContainerSetContentPacket.h"
#include "../network/packet/ChatPacket.h"
#include "../network/packet/EntityEventPacket.h"
#include "../network/packet/SetHealthPacket.h"
#include "../network/packet/TakeItemEntityPacket.h"
#include "../client/Minecraft.h"
#include "../world/level/tile/entity/FurnaceTileEntity.h"
#include "../world/inventory/FurnaceMenu.h"
#include "../world/inventory/FillingContainer.h"
#include "../world/inventory/ContainerMenu.h"
#include "../world/entity/EntityEvent.h"
#include "../network/packet/AnimatePacket.h"
#include "../world/level/tile/entity/ChestTileEntity.h"
#include "../network/packet/HurtArmorPacket.h"

ServerPlayer::ServerPlayer( Minecraft* minecraft, Level* level )
:   super(level, minecraft->isCreativeMode()),
	_mc(minecraft),
	_sentHealth(-999),
	_sentBurning(false),
	_containerCounter(0)
{
	hasFakeInventory = true;
	footSize = 0;
}

ServerPlayer::~ServerPlayer() {
	setContainerMenu(NULL);
}

void ServerPlayer::stopSleepInBed(bool forcefulWakeUp, bool updateLevelList, bool saveRespawnPoint) {
	if(isSleeping()) {
		AnimatePacket packet(AnimatePacket::WAKE_UP, this);
		_mc->raknetInstance->send(owner, packet);
	}
	super::stopSleepInBed(forcefulWakeUp, updateLevelList, saveRespawnPoint);
}


void ServerPlayer::aiStep() {
    updateAttackAnim();
    super::aiStep();
}

void ServerPlayer::tick() {
	// Knockback. A visitor moves its own player, so a hit that pushed this
	// copy here (a mob's knockback, the hoglin's launch, an arrow) moved
	// nothing on the visitor's screen: 0.6.1 never sent it. Now the push goes
	// to the visitor as motion, the way Java's server sends it. Only real
	// pushes - a burn marks hurt too, with nothing to send.
	if (hurtMarked) {
		hurtMarked = false;
		if (std::fabs(xd) + std::fabs(yd) + std::fabs(zd) > 0.1f) {
			SetEntityMotionPacket motion(entityId, xd, yd, zd);
			_mc->raknetInstance->send(owner, motion);
		}
	}

	super::tick();

	// On fire or not, as the host sees it: the visitor's own level never
	// burns anyone, so this is how it knows to draw the flames.
	if ((onFire > 0) != _sentBurning) {
		_sentBurning = onFire > 0;
		ExtPacket burning(ExtPacket::Burning, _sentBurning ? 1 : 0);
		_mc->raknetInstance->send(owner, burning);
	}

	if(!useItem.isNull())
		useItemDuration--;

	//LOGI("Server:tick. Cmenu: %p\n", containerMenu);
	if (containerMenu)
		containerMenu->broadcastChanges();

	if (health != _sentHealth) {
		_sentHealth = health;
		SetHealthPacket packet(health);
		_mc->raknetInstance->send(owner, packet);
	}
}

void ServerPlayer::take( Entity* e, int orgCount ) {
	TakeItemEntityPacket packet(e->entityId, entityId);
	// To the players in this player's level: another dimension has other entities.
	level->raknetInstance->send(packet);

	super::take(e, orgCount);
}

void ServerPlayer::hurtArmor(int dmg) {
    super::hurtArmor(dmg);
    HurtArmorPacket packet(dmg);
    _mc->raknetInstance->send(owner, packet);
}

void ServerPlayer::openContainer( ChestTileEntity* container) {
	LOGI("Client is opening a container\n");
	nextContainerCounter();
	ContainerOpenPacket packet(_containerCounter, ContainerType::CONTAINER, container->getName(), container->getContainerSize());
	_mc->raknetInstance->send(owner, packet);
	setContainerMenu(new ContainerMenu(container, container->runningId));
}

void ServerPlayer::openFurnace( FurnaceTileEntity* furnace ) {
	LOGI("Client is opening a furnace\n");
	nextContainerCounter();
	ContainerOpenPacket packet(_containerCounter, ContainerType::FURNACE, furnace->getName(), furnace->getContainerSize());
	_mc->raknetInstance->send(owner, packet);
	setContainerMenu(new FurnaceMenu(furnace));
}

void ServerPlayer::openBrewingStand( BrewingStandTileEntity* stand ) {
	// The furnace's menu: same slot numbers, fuel timer and progress.
	nextContainerCounter();
	ContainerOpenPacket packet(_containerCounter, ContainerType::BREWING_STAND, stand->getName(), stand->getContainerSize());
	_mc->raknetInstance->send(owner, packet);
	setContainerMenu(new FurnaceMenu(stand));
}

void ServerPlayer::onEffectAdded( int id, int duration ) {
	// A visitor drank on the host: its own client keeps its HUD and its air.
	ExtPacket packet(ExtPacket::MobEffect, id, duration);
	_mc->raknetInstance->send(owner, packet);
}

void ServerPlayer::closeContainer() {
	LOGI("Client is closing a container\n");
	ContainerClosePacket packet(containerMenu->containerId);
	_mc->raknetInstance->send(owner, packet);
	doCloseContainer();
}

void ServerPlayer::doCloseContainer() {
	if (!containerMenu) {
		LOGE("Container is missing @ doCloseContainer!\n");
	}
	setContainerMenu(NULL);
}

bool ServerPlayer::hasResource( int id ) {
	return true;
}

//
// IContainerListener
//
void ServerPlayer::setContainerData( BaseContainerMenu* menu, int id, int value ) {
	ContainerSetDataPacket p(menu->containerId, id, value);
	_mc->raknetInstance->send(owner, p);
	//LOGI("Setting container data for id %d: %d\n", id, value);
}

void ServerPlayer::slotChanged( BaseContainerMenu* menu, int slot, const ItemInstance& item, bool isResultSlot ) {
	if (isResultSlot) return;
	ContainerSetSlotPacket p(menu->containerId, slot, item);
	_mc->raknetInstance->send(owner, p);
	//LOGI("Slot %d changed\n", slot);
}

void ServerPlayer::refreshContainer( BaseContainerMenu* menu, const std::vector<ItemInstance>& items ) {
	ContainerSetContentPacket p(menu->containerId, menu->getItems());
	_mc->raknetInstance->send(owner, p);
	//LOGI("Refreshing container with %d items\n", items.size());
}

void ServerPlayer::nextContainerCounter() {
	if (++_containerCounter >= 100)
		_containerCounter = 0;
}

void ServerPlayer::setContainerMenu( BaseContainerMenu* menu ) {
	if (containerMenu == menu)
		return;

	if (containerMenu)
		delete containerMenu;

	containerMenu = menu;
	if (containerMenu) {
		containerMenu->containerId = _containerCounter;
		containerMenu->setListener(this);
	}
}

void ServerPlayer::completeUsingItem() {
	EntityEventPacket p(entityId, EntityEvent::USE_ITEM_COMPLETE);
	_mc->raknetInstance->send(owner, p);
	super::completeUsingItem();
}

void ServerPlayer::displayClientMessage( const std::string& messageId ) {
	ChatPacket package(messageId);
	_mc->raknetInstance->send(owner, package);
}
