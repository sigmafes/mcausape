#include "PlayerRenderer.h"
#include "EntityRenderDispatcher.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/level/Level.h"
#include "../../../world/item/ArmorItem.h"
#include "../../Minecraft.h"
#include "../../model/PlayerSkinModel.h"
#include "../../skin/PlayerSkins.h"
#include "../Textures.h"

static const std::string armorFilenames[14] = {
	"armor/cloth_1.png",	"armor/cloth_2.png",
	"armor/chain_1.png",	"armor/chain_2.png",
	"armor/iron_1.png",		"armor/iron_2.png",
	"armor/diamond_1.png",	"armor/diamond_2.png",
	"armor/gold_1.png",		"armor/gold_2.png",
	"armor/coral_1.png",	"armor/coral_2.png",
	"armor/netherite_1.png",	"armor/netherite_2.png",
};

PlayerRenderer::PlayerRenderer( HumanoidModel* humanoidModel, float shadow )
:	super(humanoidModel, shadow),
	armorParts1(new HumanoidModel(1.0f)),
	armorParts2(new HumanoidModel(0.5f))
{
	for (int i = 0; i < 3; ++i)
		skinModels[i] = NULL;
}

PlayerRenderer::~PlayerRenderer() {
	delete armorParts1;
	delete armorParts2;
	for (int i = 0; i < 3; ++i)
		delete skinModels[i];
}

int PlayerRenderer::skinFor(Player* player) {
	// The local player's skin is the profile's, not a copy on the entity: a
	// new player object is made for every world, and the profile can change
	// while one is running.
	int id = player->skinId;
	Minecraft* mc = entityRenderDispatcher->minecraft;
	if (mc && player == (Player*) mc->player)
		id = PlayerSkins::localSkin;
	return PlayerSkins::get(id) ? id : 0;
}

HumanoidModel* PlayerRenderer::modelFor(int skinId) {
	const int model = PlayerSkins::get(skinId)->model;
	PlayerSkinModel*& slot = skinModels[model - 1];
	if (!slot)
		slot = new PlayerSkinModel(model);
	return slot;
}

void PlayerRenderer::render(Entity* mob, float x, float y, float z, float rot, float a) {
	const int skin = skinFor((Player*) mob);
	if (!skin) {
		super::render(mob, x, y, z, rot, a);
		return;
	}
	// Drawn with the skin's model for this one entity, then put back: the
	// default model is the one MobRenderer owns and deletes.
	HumanoidModel* original = getHumanoidModel();
	setHumanoidModel(modelFor(skin));
	super::render(mob, x, y, z, rot, a);
	setHumanoidModel(original);
}

void PlayerRenderer::bindMobTexture(Mob* mob) {
	const int skin = skinFor((Player*) mob);
	if (skin)
		PlayerSkins::bind(entityRenderDispatcher->textures, skin);
	else
		super::bindMobTexture(mob);
}

void PlayerRenderer::renderHandFor(Player* player) {
	const int skin = skinFor(player);
	if (!skin) {
		entityRenderDispatcher->textures->loadAndBindTexture("mob/char.png");
		renderHand();
		return;
	}
	HumanoidModel* original = getHumanoidModel();
	PlayerSkins::bind(entityRenderDispatcher->textures, skin);
	setHumanoidModel(modelFor(skin));
	glEnable2(GL_ALPHA_TEST);
	renderHand();
	setHumanoidModel(original);
}

void PlayerRenderer::setupPosition( Entity* mob, float x, float y, float z ) {
	Player* player = (Player*) mob;
	if(player->isAlive() && player->isSleeping()) {
		return super::setupPosition(mob, x + player->bedOffsetX, y + player->bedOffsetY, z + player->bedOffsetZ);
	}
	return super::setupPosition(mob, x, y, z);
}

void PlayerRenderer::setupRotations( Entity* mob, float bob, float bodyRot, float a ) {
	Player* player = (Player*) mob;
	if(player->isAlive() && player->isSleeping()) {
		glRotatef(player->getSleepRotation(), 0, 1, 0);
		glRotatef(getFlipDegrees(player), 0, 0, 1);
		glRotatef(270, 0, 1, 0);
		return;
	}
	super::setupRotations(mob, bob, bodyRot, a);

	// Swimming: tipped forward onto the front, following the look in water
	// (diving tips it head first), as Java's PlayerRenderer.setupRotations.
	// The model's origin is the feet, so once lying flat it is also moved so
	// the turn is about the chest and the body lies inside its 0.6 box.
	const float swim = player->getSwimAmount(a);
	if (swim > 0) {
		const float pitch = player->xRotO + (player->xRot - player->xRotO) * a;
		const float target = player->isInWater() ? -90.0f - pitch : -90.0f;
		glRotatef(swim * target, 1, 0, 0);
		if (player->bbHeight < 1.0f)
			glTranslatef(0, -1.0f, 0.3f);
	}
}

void PlayerRenderer::renderName( Mob* mob, float x, float y, float z ){
	// Not gated on adventureSettings.showNameTags any more: it starts false and
	// nothing in the game ever turns it on, so names were never drawn. The only
	// other players are in multiplayer, which is exactly where they belong.
	// Over everyone in view, through blocks, except a player who is sneaking.
	// Your own is there too once the camera is behind (or in front of) you:
	// that is the only view in which it is not inside your head.
	const bool self = (mob == entityRenderDispatcher->cameraEntity);
	if (self && !entityRenderDispatcher->options->thirdPersonView)
		return;
	if (mob->isSneaking())
		return;
	renderNameTag(mob, ((Player*)mob)->name, x, y, z, 32);
}

int PlayerRenderer::prepareArmor(Mob* mob, int layer, float a) {
	Player* player = (Player*) mob;

	ItemInstance* itemInstance = player->getArmor(layer);
	if (!ItemInstance::isArmorItem(itemInstance))
		return -1;

	ArmorItem* armorItem = (ArmorItem*) itemInstance->getItem();
	int fnIndex = (armorItem->modelIndex + armorItem->modelIndex) + (layer == 2 ? 1 : 0);
	bindTexture(armorFilenames[fnIndex]);

	HumanoidModel* armor = layer == 2 ? armorParts2 : armorParts1;

	armor->head.visible = layer == 0;
	//armor.hair.visible = layer == 0;
	armor->body.visible = layer == 1 || layer == 2;
	armor->arm0.visible = layer == 1;
	armor->arm1.visible = layer == 1;
	armor->leg0.visible = layer == 2 || layer == 3;
	armor->leg1.visible = layer == 2 || layer == 3;
	// The same pose as the body under it: the swimming stroke and the
	// sneak were the player model's only, so the armour stayed upright.
	armor->swimAmount = player->getSwimAmount(a);
	armor->sneaking = player->isSneaking();

	setArmor(armor);

	/*if (itemInstance.isEnchanted())
		return 15; */

	return 1;
}

// Iron, gold and netherite reflect as metal (tinted by their colour);
// diamond and coral as polished stone (a clear reflection). Leather and
// chain stay matt.
int PlayerRenderer::armorTag(Mob* mob, int layer) {
	ItemInstance* it = ((Player*) mob)->getArmor(layer);
	if (!ItemInstance::isArmorItem(it)) return -1;
	ArmorItem* armor = (ArmorItem*) it->getItem();
	if (&armor->armorType == &ArmorItem::IRON && armor->modelIndex != 5) return 200;	// coral is on IRON's material
	if (&armor->armorType == &ArmorItem::GOLD || &armor->armorType == &ArmorItem::NETHERITE) return 200;
	if (&armor->armorType == &ArmorItem::DIAMOND || armor->modelIndex == 5) return 215;
	return -1;
}

void PlayerRenderer::onGraphicsReset() {
	super::onGraphicsReset();

	if (armorParts1) armorParts1->onGraphicsReset();
	if (armorParts2) armorParts2->onGraphicsReset();
	for (int i = 0; i < 3; ++i)
		if (skinModels[i]) skinModels[i]->onGraphicsReset();
}
