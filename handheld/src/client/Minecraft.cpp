#include "Minecraft.h"

#if defined(__APPLE__)
    #include <TargetConditionals.h>
#endif

#if defined(APPLE_DEMO_PROMOTION)
    #define NO_NETWORK
#endif

#if defined(RPI)
	#define CREATORMODE
#endif

#include "../network/RakNetInstance.h"
#include "../network/WebRakNetInstance.h"
#include "../network/ClientSideNetworkHandler.h"
#include "../network/ServerSideNetworkHandler.h"
//#include "../network/Packet.h"
#include "../world/entity/player/Inventory.h"
#include "../world/level/chunk/ChunkCache.h"
#include "../world/level/tile/Tile.h"
#include "../world/level/tile/PortalTile.h"
#include "gui/screens/ProgressScreen.h"
#include "../world/level/dimension/Dimension.h"
#include "../world/level/dimension/AquariumDimension.h"
#include "../world/level/levelgen/AquariumLevelSource.h"
#include <algorithm>
#include "../world/level/LevelConstants.h"
#include "../world/level/storage/LevelStorageSource.h"
#include "../world/level/storage/LevelStorage.h"
#include "player/input/KeyboardInput.h"
#ifndef STANDALONE_SERVER
#include "player/input/touchscreen/TouchInputHolder.h"
#endif
#include "player/LocalPlayer.h"
#include "gamemode/CreativeMode.h"
#include "gamemode/SurvivalMode.h"
#include "player/LocalPlayer.h"
#ifndef STANDALONE_SERVER
#include "particle/ParticleEngine.h"
#include "gui/Screen.h"
#include "gui/Font.h"
#include "gui/screens/RenameMPLevelScreen.h"
#include "sound/SoundEngine.h"
#include "../platform/web/WebBridge.h"
#endif
#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif
#include "../platform/CThread.h"
#include "../platform/input/Mouse.h"
#include "../AppPlatform.h"
#include "../Performance.h"
#include "../LicenseCodes.h"
#include "../util/PerfTimer.h"
#include "../util/PerfRenderer.h"
#include "player/input/MouseBuildInput.h"

#include "../world/Facing.h"

#include "../network/packet/PlaceBlockPacket.h"

#include "player/input/IInputHolder.h"
#ifndef STANDALONE_SERVER
#include "player/input/touchscreen/TouchscreenInput.h"

#include "player/input/ControllerTurnInput.h"
#include "player/input/XperiaPlayInput.h"

#endif

#include "player/input/MouseTurnInput.h"
#include "../world/entity/MobFactory.h"
#include "../world/level/MobSpawner.h"
#include "../util/Mth.h"
#include "../network/packet/InteractPacket.h"
#ifndef STANDALONE_SERVER
#include "gui/screens/PrerenderTilesScreen.h"
#include "renderer/Textures.h"
#include "skin/PlayerSkins.h"
#include "../network/packet/PlayerSkinPacket.h"
#include "gui/screens/DeathScreen.h"
#endif

#include "../network/packet/RespawnPacket.h"
#include "IConfigListener.h"
#include "../world/entity/MobCategory.h"
#ifndef STANDALONE_SERVER
#include "gui/screens/FurnaceScreen.h"
#endif
#include "../world/Difficulty.h"
#include "../server/ServerLevel.h"
#ifdef CREATORMODE
#include "../server/CreatorLevel.h"
#endif
#include "../network/packet/AdventureSettingsPacket.h"
#include "../network/packet/SetSpawnPositionPacket.h"
#include "../network/command/CommandServer.h"
#include "gamemode/CreatorMode.h"
#ifndef STANDALONE_SERVER
#include "gui/screens/ArmorScreen.h"
#include "gui/screens/IngameBlockSelectionScreen.h"
#include "gui/screens/touch/TouchIngameBlockSelectionScreen.h"
#include "gui/screens/crafting/WorkbenchScreen.h"
#include "gui/screens/ChatScreen.h"
#include "../world/level/tile/FireTile.h"
#endif
#include "../world/level/levelgen/synth/ImprovedNoise.h"
#ifndef STANDALONE_SERVER
#include "renderer/tileentity/TileEntityRenderDispatcher.h"
#endif

#ifndef STANDALONE_SERVER
#include "renderer/ptexture/DynamicTexture.h"
#include "renderer/GameRenderer.h"
#include "renderer/ItemInHandRenderer.h"
#include "renderer/LevelRenderer.h"
#include "renderer/entity/EntityRenderDispatcher.h"
#include "gui/Screen.h"
#include "gui/Font.h"
#include "gui/screens/RenameMPLevelScreen.h"
#include "sound/SoundEngine.h"
#include "../platform/web/WebBridge.h"
#endif

static void checkGlError(const char* tag) {
#ifdef GLDEBUG
	while (1) {
        const int errCode = glGetError();
        if (errCode == GL_NO_ERROR) break;

		LOGE("################\nOpenGL-error @ %s : #%d\n", tag, errCode);
	}
#endif /*GLDEBUG*/
}

/*static*/
const char* Minecraft::progressMessages[] = {
	"Locating server",
	"Building terrain",
	"Preparing",
	"Saving chunks"
};

int Minecraft::customDebugId = Minecraft::CDI_NONE;

#if defined(_MSC_VER)
	#pragma warning( disable : 4355 ) // 'this' pointer in initialization list which is perfectly legal
#endif

bool Minecraft::useAmbientOcclusion = false;

Minecraft::Minecraft()
:	user(NULL),
	level(NULL),
	player(NULL),
	cameraTargetPlayer(NULL),
	levelRenderer(NULL),
	gameRenderer(NULL),
#ifndef STANDALONE_SERVER
	particleEngine(NULL),
	_perfRenderer(NULL),
#endif
	_commandServer(NULL),
#ifndef STANDALONE_SERVER
	textures(NULL),
#endif
	lastTickTime(-1),
	lastTime(0),
	ticksSinceLastUpdate(0),
	gameMode(NULL),
	mouseGrabbed(true),
	_musicOn(false),
	_musicVolume(-1.0f),
	missTime(0),
	pause(false),
	_running(false),
	timer(20),
#ifndef STANDALONE_SERVER
	gui(this),
#endif
	netCallback(NULL),
#ifndef STANDALONE_SERVER
	screen(NULL),
	font(NULL),
#endif
	screenMutex(false),
#ifndef STANDALONE_SERVER
	scheduledScreen(NULL),
	hasScheduledScreen(false),
	soundEngine(NULL),
#endif
	ticks(0),
	isGeneratingLevel(false),
	_hasSignaledGeneratingLevelFinished(true),
	_prepChunkIndex(0),
	_prepCount(0),
	_genBackground(0),
	_prepInProgress(false),
	_prepPhase(0),
	generateLevelThread(NULL),
	progressStagePercentage(0),
	progressStageStatusId(0),
	isLookingForMultiplayer(false),
	_licenseId(LicenseCodes::WAIT_PLATFORM_NOT_READY),
	inputHolder(0),
	_supportsNonTouchscreen(false),
#ifndef STANDALONE_SERVER
	screenChooser(this),
#endif
	width(1), height(1),
	//_respawnPlayerTicks(-1),
#ifdef __APPLE__
    _isSuperFast(false),
#endif
	_powerVr(false),
	commandPort(4711),
	reserved_d1(0),reserved_d2(0),
	reserved_f1(0),reserved_f2(0)
{
_portalCooldown = 0;
inputLocked = false;
_travelPlayer = NULL;
_pendingDim = 0;
_warmLevel = NULL;
_warmIndex = 0;
_pendingDimFrames = 0;
_prepNether = false;
_netherPrep = NULL;
_netherPrepIndex = 0;
//#ifdef ANDROID

#if defined(NO_NETWORK)
    raknetInstance = new IRakNetInstance();
#elif defined(MC_WASM)
	// Not RakNetInstance: in a browser its pingForHosts() takes RakPeer through
	// a Startup() that cannot succeed and then pings anyway, off the end of an
	// empty socketList. This one lists players from the lobby service instead of
	// broadcasting, and carries the game's traffic over a WebSocket relay rather
	// than UDP -- see WebRakNetInstance.h for the whole mechanism. With no relay
	// configured, which is what github.io serves, it lists and refuses to join.
	raknetInstance = new WebRakNetInstance();
#else
	raknetInstance = new RakNetInstance();
#endif
#ifndef STANDALONE_SERVER
	soundEngine = new SoundEngine(20.0f);
	soundEngine->init(this, &options);
#endif
	//setupPieces();
}

Minecraft::~Minecraft()
{
	delete netCallback;
	netCallback = NULL;
	destroyOtherLevels();
	delete raknetInstance;
#ifndef STANDALONE_SERVER
	delete levelRenderer;
	delete gameRenderer;
	delete particleEngine;

	delete soundEngine;
#endif
	delete gameMode;
#ifndef STANDALONE_SERVER
	delete font;
	delete textures;

	if (screen != NULL) {
		delete screen;
		screen = NULL;
	}
#endif
	if (level != NULL) {
		level->saveGame();
		if (level->getChunkSource())
			level->getChunkSource()->saveAll(true);
		delete level->getLevelStorage();
		delete level;
		level = NULL;
	}

	//delete player;
	delete user;
	delete inputHolder;

	delete storageSource;
	delete _perfRenderer;
	delete _commandServer;

	MobFactory::clearStaticTestMobs();
#ifndef STANDALONE_SERVER
	EntityRenderDispatcher::destroy();
#endif
}

// Only called by server
void Minecraft::selectLevel( const std::string& levelId, const std::string& levelName, const LevelSettings& settings )
{
#if defined(CREATORMODE)
	level = new CreatorLevel(
#else
	level = new ServerLevel(
#endif
		storageSource->selectLevel(levelId, false),
		levelName,
		settings,
		SharedConstants::GeneratorVersion);

	// note: settings is useless beyond this point, since it's
	//       either copied to LevelData (or LevelData read from file)
	setLevel(level, "Generating level");
	setIsCreativeMode(level->getLevelData()->getGameType() == GameType::Creative);
	// Difficulty belongs to the world now, not to the options file.
	options.difficulty = level->getLevelData()->getDifficulty();
	_running = true;
}

void Minecraft::setLevel(Level* level, const std::string& message /* ="" */, LocalPlayer* forceInsertPlayer /* = NULL */) {
	cameraTargetPlayer = NULL;
	LOGI("Seed is %ld\n", level->getSeed());

	if (level != NULL) {
		level->raknetInstance = raknetInstance;
        gameMode->initLevel(level);
#if defined(MC_WASM)
		// hostMultiplayer() is what tells the lobby a world is running, but its
		// signature carries only the player's name. This is the first point the
		// world has one, and selectLevel() runs it before hostMultiplayer().
		if (level->getLevelData())
			mcpeLobbySetWorld(level->getLevelData()->getLevelName());
#endif

		if (!player && forceInsertPlayer)
		{
			player = forceInsertPlayer;
			player->resetPos(false);
			//level->addEntity(forceInsertPlayer);
		}
		else if (player != NULL) {
			player->resetPos(false);
			if (level != NULL) {
				level->addEntity(player);
			}
		}
		this->level = level;
		_hasSignaledGeneratingLevelFinished = false;
		// Threads in wasm mean pthreads, which mean SharedArrayBuffer, which
		// means COOP/COEP response headers -- and GitHub Pages cannot send
		// those. Generating on the main thread just blocks for a second or two.
#if defined(STANDALONE_SERVER) || defined(__EMSCRIPTEN__)
		const bool threadedLevelCreation = false;
#else
		const bool threadedLevelCreation = true;
#endif

		if (threadedLevelCreation) {
			// Threaded
			// "Lock"
			isGeneratingLevel = true;
			generateLevelThread = new CThread(Minecraft::prepareLevel_tspawn, this);
		} else {
#if defined(MC_WASM)
			// Same lock, driven from update() a slice at a time instead of by a
			// thread, so the frame in between paints ProgressScreen and the bar
			// moves. Running it here would block the page's only thread for the
			// whole generation and nothing would be drawn at all.
			if (level->isClientSide) {
				// A visitor makes nothing: its chunks come from the host
				// (ClientChunkSource), so the level is ready as it is.
				isGeneratingLevel = false;
			} else {
				isGeneratingLevel = true;
				_prepInProgress = true;
				prepareLevelBegin();
			}
#else
			// Non-threaded
			generateLevel("Currently not used", level);
#endif
		}
    } else {
        player = NULL;
    }

    this->lastTickTime = 0;
	this->_running = true;
}

void Minecraft::leaveGame(bool renameLevel /*=false*/)
{
    if (isGeneratingLevel || !_hasSignaledGeneratingLevelFinished)
        return;
    
	isGeneratingLevel = false;
	bool saveLevel = level && (!level->isClientSide || renameLevel);
	inputLocked = false;
	// A visitor's last inventory and position, to the host before we go.
	if (level && level->isClientSide && netCallback)
		static_cast<ClientSideNetworkHandler*>(netCallback)->sendPlayerData();

	raknetInstance->disconnect();
	if (saveLevel) {
		// If server or wanting to save level as client, save all unsaved chunks!
		// saveGame writes the entities and level.dat (which holds the player
		// and which dimension he is in); without it only the chunks were saved
		// and up to 30s of position and inventory went with the Level.
		level->saveGame();
		level->getChunkSource()->saveAll(true);
	}

	LOGI("Clearing levels\n");

	cameraTargetPlayer = NULL;
#ifndef STANDALONE_SERVER
	levelRenderer->setLevel(NULL);
	particleEngine->setLevel(NULL);
#endif
	destroyOtherLevels();
	LOGI("Erasing callback\n");
	delete netCallback;
	netCallback = NULL;

	LOGI("Erasing level\n");
	if (level != NULL) {
		LevelStorage* storage = level->getLevelStorage();
		delete level;
		delete storage;
		level = NULL;
	}
	//delete player;
	player = NULL;
	cameraTargetPlayer = NULL;

	_running = false;
#ifndef STANDALONE_SERVER
	if (renameLevel) {
		setScreen(new RenameMPLevelScreen(LevelStorageSource::TempLevelId));
	}
	else
		screenChooser.setScreen(SCREEN_STARTMENU);
#endif
}

void Minecraft::prepareLevel(const std::string& title) {
	// Straight through, exactly as this always ran. Only the web splits it up;
	// see prepareLevelChunkStep and the note in Minecraft.h.
	prepareLevelBegin();
	prepareLevelChunkStep(-1);
	prepareLevelSave();
	if (_prepNether) {
		prepareNetherBegin();
		prepareNetherStep(-1);
		prepareNetherFinish();
	}
	prepareLevelPrepare();
}

void Minecraft::prepareLevelBegin() {
	LOGI("status: 1\n");
	progressStageStatusId = 1;
	progressStagePercentage = 0;

	// Dont update lights if we load the level (ok, actually just with leveldata version=1.+(?))
	if (!level->isNew())
		level->setUpdateLights(false);

	// Both reset here rather than at construction: this runs for every world
	// load, and a second one starting at the phase the first finished on would
	// skip straight past generating anything.
	_prepChunkIndex = 0;
	_prepPhase = 0;

	// A new world used to build its whole Nether here as well, which made
	// creating one take twice as long. The Nether is made when somebody first
	// goes through a portal now (levelForDimension, or a trip on its own with
	// its loading screen), so it is not built in advance any more.
	_prepNether = false;
	_netherPrep = NULL;
	_netherPrepIndex = 0;

	// The chunk pass goes nearest the arrival first. A world being created, or
	// a trip to another dimension, stops after the chunks within a few of it
	// and lets the player in; the rest are made in the background while
	// playing. Loading a saved world still reads it all first.
	const int W = CHUNK_CACHE_WIDTH;
	int fx = W / 2, fz = W / 2;
	if (_travelPlayer) {
		fx = Mth::floor(_travelPlayer->x) >> 4;
		fz = Mth::floor(_travelPlayer->z) >> 4;
	}
	std::vector<std::pair<int, int> > byDistance;
	for (int i = 0; i < W * W; ++i) {
		const int dx = i / W - fx, dz = i % W - fz;
		byDistance.push_back(std::make_pair(dx * dx + dz * dz, i));
	}
	std::sort(byDistance.begin(), byDistance.end());
	_genOrder.clear();
	_prepCount = 0;
	const bool quick = !level->isClientSide && (level->isNew() || _travelPlayer != NULL);
	const int Radius = 5;
	for (int i = 0; i < W * W; ++i) {
		_genOrder.push_back(byDistance[i].second);
		if (!quick || byDistance[i].first <= Radius * Radius) ++_prepCount;
	}
	_genBackground = (unsigned int) _genOrder.size();	// none until in the world
}

void Minecraft::tickBackgroundGeneration(int maxMs)
{
	if (_genBackground >= _genOrder.size() || !level || level->isClientSide) return;
	const int W = CHUNK_CACHE_WIDTH;
	const int start = getTimeMs();
	while (_genBackground < _genOrder.size() && getTimeMs() - start < maxMs) {
		const int i = _genOrder[_genBackground++];
		level->getChunk(i / W, i % W);
	}
	if (_genBackground >= _genOrder.size())
		LOGI("Background generation done\n");
}

/** The expensive pass: one getTile per chunk column, which is what actually
    generates the world, plus the light settling behind it.

    It was two nested loops over x and z. They are folded into one index so the
    pass can stop between any two chunks and pick up next frame -- the whole
    point of the split. The arithmetic below reproduces the original bounds:
    both loops ran from 8 to CHUNK_CACHE_WIDTH * CHUNK_WIDTH stepping a chunk,
    which is CHUNK_CACHE_WIDTH steps each, so the index divides and remainders
    straight back into them. */
bool Minecraft::prepareLevelChunkStep(int maxIterations) {
	const int Max = _prepCount;
	int done = 0;

	while (_prepChunkIndex < Max) {
		if (maxIterations > 0 && done >= maxIterations)
			return false;

		const int index = _genOrder[_prepChunkIndex];
		const int x = 8 + (index / CHUNK_CACHE_WIDTH) * CHUNK_WIDTH;
		const int z = 8 + (index % CHUNK_CACHE_WIDTH) * CHUNK_WIDTH;

		progressStagePercentage = 100 * _prepChunkIndex / Max;
		++_prepChunkIndex;
		++done;

		level->getTile(x, 64, z);
		if (level->isNew())
			while (level->updateLights())
				;
	}

	return true;
}

void Minecraft::prepareLevelFinish() {
	prepareLevelSave();
	prepareLevelPrepare();
}

void Minecraft::prepareLevelSave() {
	level->setUpdateLights(true);

	for (int x = 0; x < CHUNK_CACHE_WIDTH; x++)
	{
		for (int z = 0; z < CHUNK_CACHE_WIDTH; z++)
		{
			// Only what the chunk pass made: asking for the rest would make it
			// all here, on one frame, and undo the quick start.
			if (!level->hasChunk(x, z)) continue;
			LevelChunk* chunk = level->getChunk(x, z);
			if (chunk && !chunk->createdFromSave)
			{
				chunk->unsaved = false;
				chunk->clearUpdateMap();

					// Terrain made while lights were off has no block light (lava, glowstone).
					// Redo it near where the player arrives; further out it is lit as the
					// chunks are visited with lights on.
					if (_travelPlayer != NULL) {
						int dx = x - (int) (_travelPlayer->x / 16);
						int dz = z - (int) (_travelPlayer->z / 16);
						if (dx >= -3 && dx <= 3 && dz >= -3 && dz <= 3) {
							level->updateLight(LightLayer::Block, x * 16, 0, z * 16, x * 16 + 15, 127, z * 16 + 15);
							if (!level->dimension->hasCeiling)
								level->updateLight(LightLayer::Sky, x * 16, 0, z * 16, x * 16 + 15, 127, z * 16 + 15);
						}
					}
			}
		}
	}

	LOGI("status: 3\n");
	progressStageStatusId = 3;
	if (level->isNew()) {
		level->setInitialSpawn(); // @note: should obviously be called from Level itself
		level->saveLevelData();
		level->getChunkSource()->saveAll(false);
		level->saveGame();
	} else {
		level->saveLevelData();
		level->loadEntities();
	}

}

// Builds the Nether for a world that was just created: every chunk, lit, saved
// to chunks_nether.dat. It runs on a throwaway Level that shares the world's
// storage (that is how the file ends up in the right place), and is thrown away
// again, so nothing of it stays in memory.
void Minecraft::prepareNetherBegin() {
	LevelStorage* storage = level->getLevelStorage();
	std::string levelName = level->getLevelData()->getLevelName();
#if defined(CREATORMODE)
	_netherPrep = new CreatorLevel(
#else
	_netherPrep = new ServerLevel(
#endif
		storage,
		levelName,
		LevelSettings::None(),
		SharedConstants::GeneratorVersion,
		Dimension::getNew(Dimension::NETHER));
	_netherPrepIndex = 0;
}

// One column per step, with the light settled after each, so a chunk is lit while
// its neighbours are still being made: the light of the one made later flows back
// across the border. maxMs < 0 runs to the end. Returns true when it is all done.
bool Minecraft::prepareNetherStep(int maxMs) {
	const int Max = CHUNK_CACHE_WIDTH * CHUNK_CACHE_WIDTH;
	const int start = getTimeMs();

	while (_netherPrepIndex < Max) {
		const int x = 8 + (_netherPrepIndex / CHUNK_CACHE_WIDTH) * CHUNK_WIDTH;
		const int z = 8 + (_netherPrepIndex % CHUNK_CACHE_WIDTH) * CHUNK_WIDTH;

		progressStagePercentage = 100 * _netherPrepIndex / Max;
		++_netherPrepIndex;

		_netherPrep->getTile(x, 64, z);
		_netherPrep->updateLights();

		if (maxMs >= 0 && (getTimeMs() - start) >= maxMs)
			return false;
	}

	// Whatever light is still queued.
	while (_netherPrep->updateLights()) {
		if (maxMs >= 0 && (getTimeMs() - start) >= maxMs)
			return false;
	}
	return true;
}

void Minecraft::prepareNetherFinish() {
	LevelStorage* storage = _netherPrep->getLevelStorage();
	_netherPrep->getChunkSource()->saveAll(false);
	delete _netherPrep;
	_netherPrep = NULL;

	// The storage keeps a pointer to the last Level prepared on it (its tick and
	// its pending-chunk list use it), and that one has just been deleted.
	storage->prepareLevel(level);
}
void Minecraft::prepareLevelPrepare() {
	progressStagePercentage = -1;
	progressStageStatusId = 2;
	LOGI("status: 2\n");

	level->prepare();

	// The per-phase Stopwatch prints went with the split -- they straddled the
	// three functions and cannot span frames. generateLevel still times and
	// prints the whole thing, which is the number that was ever read anyway.
	progressStageStatusId = 0;
}

static const int PORTAL_COOLDOWN_TICKS = 40;

// The Overworld comes in two kinds (NORMAL, and NORMAL_DAYCYCLE for creative):
// they are the same place.
static int kindOf(int id) { return Dimension::isOverworld(id) ? Dimension::NORMAL : id; }

Level* Minecraft::levelForDimension(int dimension, bool create)
{
	const int want = kindOf(dimension);
	if (level && level->dimension && kindOf(level->dimension->id) == want) return level;
	for (unsigned int i = 0; i < otherLevels.size(); ++i)
		if (kindOf(otherLevels[i]->dimension->id) == want) return otherLevels[i];
	if (!create || !level || level->isClientSide || isGeneratingLevel || !netCallback) return NULL;

	// Its own storage: a storage serves one Level (see LevelStorage::openSibling).
	LevelStorage* storage = level->getLevelStorage() ? level->getLevelStorage()->openSibling() : NULL;
	if (!storage) return NULL;

	Dimension* newDimension = Dimension::isOverworld(want)
		? DimensionFactory::createDefaultDimension(level->getLevelData())
		: Dimension::getNew(want);
#if defined(CREATORMODE)
	Level* other = new CreatorLevel(
#else
	Level* other = new ServerLevel(
#endif
		storage,
		level->getLevelData()->getLevelName(),
		LevelSettings::None(),
		SharedConstants::GeneratorVersion,
		newDimension);
	other->isSecondary = true;
	// What the world is, not what was on disk the last time it was saved.
	*other->getLevelData() = *level->getLevelData();
	other->getLevelData()->setLoadedPlayerTag(NULL);
	other->difficulty = level->difficulty;
	other->adventureSettings = level->adventureSettings;
	other->raknetInstance = raknetInstance;
	other->setUpdateLights(true);
	other->loadEntities();

	otherLevels.push_back(other);
	netCallback->levelAdded(other);
	LOGI("Level running: dimension %d\n", want);
	return other;
}

void Minecraft::destroyOtherLevel(Level* other)
{
	std::vector<Level*>::iterator it = std::find(otherLevels.begin(), otherLevels.end(), other);
	if (it == otherLevels.end()) return;
	otherLevels.erase(it);
	LOGI("Level done: dimension %d\n", other->dimension->id);
	if (_warmLevel == other) _warmLevel = NULL;
	if (netCallback) netCallback->levelRemoved(other);
	other->saveGame();
	if (other->getChunkSource())
		other->getChunkSource()->saveAll(true);
	LevelStorage* storage = other->getLevelStorage();
	delete other;
	delete storage;
}

void Minecraft::destroyOtherLevels()
{
	while (!otherLevels.empty())
		destroyOtherLevel(otherLevels.back());
}

void Minecraft::unloadEmptyLevels()
{
	ServerSideNetworkHandler* net = static_cast<ServerSideNetworkHandler*>(netCallback);
	for (int i = (int) otherLevels.size() - 1; i >= 0; --i) {
		Level* other = otherLevels[i];
		if (other == _warmLevel) continue;
		if (!other->players.empty()) continue;
		if (net && net->levelInUse(other)) continue;
		destroyOtherLevel(other);
	}
}

void Minecraft::findArrival(Level* dst, int fromDim, float fromX, float fromZ, bool respawn, Player* who, int* outX, int* outY, int* outZ)
{
	// Same coordinates on both sides of a Nether portal: no 1:8 scale, the
	// world is finite and small. Clamped so the arrival room is inside it.
	int tx = (int) fromX;
	int tz = (int) fromZ;
	if (tx < 4) tx = 4;
	if (tx >= LEVEL_WIDTH - 4) tx = LEVEL_WIDTH - 5;
	if (tz < 4) tz = 4;
	if (tz >= LEVEL_DEPTH - 4) tz = LEVEL_DEPTH - 5;

	const int dstDim = dst->dimension->id;
	PortalTile* portal = (PortalTile*) Tile::portalTile;
	int ty = 0;
	int foundX = 0, foundY = 0, foundZ = 0;
	if (respawn) {
		// Back from a death: the bed if he slept in one, else the world spawn.
		Pos p = (who && who->hasRespawnPosition()) ? who->getRespawnPosition() : dst->getSharedSpawnPos();
		tx = p.x;
		tz = p.z;
		ty = dst->getTopTileY(tx, tz) + 1;
	} else if (dstDim == Dimension::AQUARIUM) {
		// Every way in comes out at the fixed portal, on its frame.
		AquariumLevelSource::buildHub(dst);
		AquariumLevelSource::hubArrival(dst->random.nextInt(8), &tx, &ty, &tz);
	} else if (fromDim == Dimension::AQUARIUM) {
		// Back to the Overworld portal he went in by, or the spawn if he never
		// did (a command, or a save from before he had one).
		if (who && who->hasAquariumReturn) {
			tx = who->aquariumReturnX;
			ty = who->aquariumReturnY;
			tz = who->aquariumReturnZ;
		} else {
			Pos p = dst->getSharedSpawnPos();
			tx = p.x;
			tz = p.z;
			ty = dst->getTopTileY(tx, tz) + 1;
		}
	} else if (portal->findNearbyPortal(dst, tx, tz, 16, &foundX, &foundY, &foundZ)) {
		// A portal is already there (the one he came through the first time).
		tx = foundX;
		ty = foundY;
		tz = foundZ;
	} else {
		ty = (dstDim == Dimension::NETHER) ? 64 : dst->getTopTileY(tx, tz) + 1;
		portal->buildArrivalPortal(dst, tx, ty, tz);
	}
	*outX = tx;
	*outY = ty;
	*outZ = tz;
}

bool Minecraft::travelKeepsLevels()
{
	return netCallback && (level->players.size() > 1 || !otherLevels.empty()
		|| static_cast<ServerSideNetworkHandler*>(netCallback)->hasPending());
}

bool Minecraft::warmTravelLevel(Level* dst, int maxMs)
{
	const int total = CHUNK_CACHE_WIDTH * CHUNK_CACHE_WIDTH;
	if (dst != _warmLevel) {
		// Nearest the arrival first: the Aquarium's fixed portal, or the same
		// spot on the other side of a Nether portal.
		int fx = Mth::floor(player->x) >> 4, fz = Mth::floor(player->z) >> 4;
		if (dst->dimension->id == Dimension::AQUARIUM) {
			fx = AquariumDimension::HubX >> 4;
			fz = AquariumDimension::HubZ >> 4;
		}
		std::vector<std::pair<int, int> > byDistance;
		for (int i = 0; i < total; ++i) {
			const int dx = i % CHUNK_CACHE_WIDTH - fx, dz = i / CHUNK_CACHE_WIDTH - fz;
			byDistance.push_back(std::make_pair(dx * dx + dz * dz, i));
		}
		std::sort(byDistance.begin(), byDistance.end());
		_warmOrder.clear();
		for (int i = 0; i < total; ++i) _warmOrder.push_back(byDistance[i].second);
		_warmLevel = dst;
		_warmIndex = 0;
	}
	const int start = getTimeMs();
	while (_warmIndex < _warmOrder.size()) {
		const int i = _warmOrder[_warmIndex++];
		progressStagePercentage = 100 * _warmIndex / total;
		dst->getChunk(i % CHUNK_CACHE_WIDTH, i / CHUNK_CACHE_WIDTH);
		if (getTimeMs() - start >= maxMs) return false;
	}
	while (dst->updateLights())
		if (getTimeMs() - start >= maxMs) return false;
	_warmLevel = NULL;
	return true;
}

/** The host goes through a portal while others are in the world: the Levels
    stay, and the host's moves from one to the other. The Level it arrives in is
    the one that writes level.dat from now on. */
void Minecraft::changeDimensionShared(int dstDim, bool respawn)
{
	ServerSideNetworkHandler* net = static_cast<ServerSideNetworkHandler*>(netCallback);
	Level* src = level;
	Level* dst = levelForDimension(dstDim, true);
	if (!dst || dst == src) {
		LOGI("changeDimensionShared refused: no level for dimension %d\n", dstDim);
		return;
	}

	int tx, ty, tz;
	findArrival(dst, src->dimension->id, player->x, player->z, respawn, player, &tx, &ty, &tz);

	net->hostLeaving(src);
#ifndef STANDALONE_SERVER
	levelRenderer->setLevel(NULL);
	particleEngine->setLevel(NULL);
#endif
	src->detachEntityForTransfer(player);

	// The world's own data (time, game type, difficulty) goes with the host.
	*dst->getLevelData() = *src->getLevelData();
	dst->getLevelData()->setLoadedPlayerTag(NULL);
	src->isSecondary = true;
	dst->isSecondary = false;

	player->setLevel(dst);
	player->dimension = dst->dimension->id;
	player->moveTo(tx + 0.5f, (float) ty, tz + 0.5f, player->yRot, player->xRot);

	level = dst;
	otherLevels.erase(std::find(otherLevels.begin(), otherLevels.end(), dst));
	otherLevels.push_back(src);
	dst->addEntity(player);
	gameMode->initLevel(dst);
#ifndef STANDALONE_SERVER
	levelRenderer->setLevel(dst);
	particleEngine->setLevel(dst);
#endif
	cameraTargetPlayer = player;
	// Written now so a reload starts where he is.
	dst->saveLevelData();
	net->hostEntered(dst);

	_portalCooldown = PORTAL_COOLDOWN_TICKS;
}

void Minecraft::changeDimension(int dstDim, bool respawn)
{
	// Alone in the world: the old level is written out and replaced by the
	// other side's. With visitors about, the old one has to stay (it is theirs
	// still), so the host moves between Levels instead.
	LOGI("changeDimension: to %d\n", dstDim);
	if (!level || !player || isGeneratingLevel || level->isClientSide) {
		LOGI("changeDimension refused: level=%p player=%p generating=%d\n", level, player, (int) isGeneratingLevel);
		return;
	}
	if (kindOf(dstDim) == kindOf(level->dimension->id)) return;
	if (travelKeepsLevels()) {
		changeDimensionShared(dstDim, respawn);
		return;
	}

	std::string levelName = level->getLevelData()->getLevelName();
	LevelStorage* storage = level->getLevelStorage();
	const int fromDim = level->dimension->id;

	// The other side's dimension object is made from the old level's data
	// before that level is gone (the Overworld's kind depends on game mode).
	Dimension* newDimension = Dimension::isOverworld(dstDim)
		? DimensionFactory::createDefaultDimension(level->getLevelData())
		: Dimension::getNew(dstDim);

	// Write the old world out: chunks, entities, and level.dat with the
	// dimension it is leaving.
	level->getChunkSource()->saveAll(true);
	level->saveGame();

	// Take the player out of the old level without deleting him, then delete
	// the level. The storage is not part of the level and outlives it.
#ifndef STANDALONE_SERVER
	levelRenderer->setLevel(NULL);
	particleEngine->setLevel(NULL);
#endif
	level->detachEntityForTransfer(player);
	if (netCallback) netCallback->levelRemoved(level);
	delete level;
	level = NULL;

#if defined(CREATORMODE)
	Level* newLevel = new CreatorLevel(
#else
	Level* newLevel = new ServerLevel(
#endif
		storage,
		levelName,
		LevelSettings::None(),
		SharedConstants::GeneratorVersion,
		newDimension);

	// Arrival: a portal and a safe room there, or the Aquarium's fixed portal.
	// He is added to the level by _levelGenerated, after the terrain exists.
	int tx, ty, tz;
	findArrival(newLevel, fromDim, player->x, player->z, respawn, player, &tx, &ty, &tz);

	player->setLevel(newLevel);
	player->dimension = newDimension->id;
	player->moveTo(tx + 0.5f, (float) ty, tz + 0.5f, player->yRot, player->xRot);

	// The new Level read level.dat when it was built, so it carries the position
	// the player had in the OTHER dimension, and loadPlayer would apply it: in the
	// Nether that is inside rock, and resetPos pushes him up and out through the
	// ceiling. Dropping the loaded tag makes loadPlayer leave him where he is.
	newLevel->getLevelData()->setLoadedPlayerTag(NULL);
	_travelPlayer = player;
	player = NULL;

	// The renderer still holds chunks built for the level that was just deleted,
	// and a jump this far marks them dirty, so the next frame would rebuild them
	// through a dangling Level*. Point it at the new level now instead of waiting
	// for _levelGenerated: allChanged drops every old chunk.
#ifndef STANDALONE_SERVER
	levelRenderer->setLevel(newLevel);
	particleEngine->setLevel(newLevel);
#endif

	// The player arrives standing in the portal, so the cooldown stops the
	// contact from sending him straight back.
	_portalCooldown = PORTAL_COOLDOWN_TICKS;
	setLevel(newLevel, "Generating level");
}
void Minecraft::update() {
	//LOGI("Enter Update\n");

	if (Options::debugGl)
		LOGI(">>>>>>>>>>\n");

	TIMER_PUSH("root");

	updateMusic();

	//if (level) {
	//	LOGI("numplayers: %d\n", level->players.size());
	//	for (int i = 0; i < level->players.size(); ++i) {
	//		Player* p = level->players[i];
	//		bool inEnt = std::find(level->entities.begin(), level->entities.end(), p) != level->entities.end();
	//		LOGI("  %p, %d, %d - in? %d\n", p, p->entityId, p->owner.ToUint32(p->owner), inEnt);
	//	}
	//}

	if (pause && level != NULL) {
		float lastA = timer.a;
		timer.advanceTime();
		timer.a = lastA;
	} else {
		timer.advanceTime();
	}

	if (raknetInstance) {
		raknetInstance->runEvents(netCallback);
	}

	TIMER_PUSH("tick");
	int toTick = timer.ticks;
	for (int i = 0; i < toTick; ++i, ++ticks)
		tick(i, toTick-1);

	TIMER_POP_PUSH("updatelights");
	if (level && !isGeneratingLevel) {
		level->updateLights();
		for (unsigned int i = 0; i < otherLevels.size(); ++i) otherLevels[i]->updateLights();
	}
	TIMER_POP();

	#ifndef STANDALONE_SERVER
		if (gameMode != NULL) gameMode->render(timer.a);
		TIMER_PUSH("sound");
		soundEngine->update(player, timer.a);
		TIMER_POP_PUSH("render");
		gameRenderer->render(timer.a);
		TIMER_POP();

		// The frame with the loading screen has been drawn; now do the trip.
		if (_pendingDim != 0 && --_pendingDimFrames <= 0) {
			const int dst = _pendingDim - 1;
			Level* warm = (level && player && travelKeepsLevels()) ? levelForDimension(dst, true) : NULL;
			if (warm && warm != level && !warmTravelLevel(warm, 12)) {
				_pendingDimFrames = 1;	// more of it next frame
			} else {
				_pendingDim = 0;
				changeDimension(dst);
			}
		}
	#else
	CThread::sleep(1);
	#endif
#ifndef STANDALONE_SERVER
	Multitouch::resetThisUpdate();
#endif
	TIMER_POP();
#ifndef STANDALONE_SERVER
	checkGlError("Update finished");

	if (options.renderDebug) {
		if (!PerfTimer::enabled) {
			PerfTimer::reset();
			PerfTimer::enabled = true;
		}

		//TIMER_PUSH("debugfps");
		_perfRenderer->renderFpsMeter(1);
		checkGlError("render debug");
		//TIMER_POP();
	} else {
		PerfTimer::enabled = false;
	}
#endif
	//LOGI("Exit Update\n");
}

void Minecraft::tick(int nTick, int maxTick) {
	if (missTime > 0) missTime--;
	if (netCallback) netCallback->tick();

	// A portal the player walked into asked for a trip. It is honoured here, at the
	// start of a tick, when no entity is mid-update, and not again until the
	// cooldown set by the last arrival has run out.
	if (level && player && !isGeneratingLevel) {
		int target = -1;
		Entity* requester = level->takePortalTravelRequest(&target);
		if (requester == player) {
			// Still standing in a portal: a trip only starts from a cold start, and
			// being inside keeps the cooldown full, so arriving in one cannot bounce
			// him back. It starts running down once he steps out.
			if (_portalCooldown == 0) {
				if (_pendingDim == 0) {
					// A Nether portal goes to the other side of it; the Aquarium's
					// say where they go.
					if (target < 0)
						target = (level->dimension->id == Dimension::NETHER) ? Dimension::NORMAL : Dimension::NETHER;
					_pendingDim = target + 1;
					_pendingDimFrames = 2;
					setScreen(new ProgressScreen());
				}
			} else {
				_portalCooldown = PORTAL_COOLDOWN_TICKS;
			}
		} else if (_portalCooldown > 0) {
			_portalCooldown--;
		}
	}
#ifndef STANDALONE_SERVER
	if (!screen && player) {
		if (player->health <= 0) {
			setScreen(new DeathScreen());
		}
	}
#endif
	TIMER_PUSH("gameMode");
	if (level && !pause) {
		gameMode->tick();
	}

	TIMER_POP_PUSH("commandServer");
	if (level && _commandServer) {
		_commandServer->tick();
	}

	TIMER_POP_PUSH("input");
	tickInput();
#ifndef STANDALONE_SERVER
	TIMER_POP_PUSH("gui");
	gui.tick();
#endif
	//
	// Ongoing level generation in a (perhaps) different thread. When it's
	// ready, _levelGenerated() is called once and any threads are deleted.
	//
	if (isGeneratingLevel) {
#if defined(MC_WASM)
		/* Deliberately here, below this frame's render up at gameRenderer->render:
		   the slice runs after ProgressScreen has been drawn with the percentage
		   the last slice left behind, so each frame shows the bar one step
		   further along.

		   One column at a time under a frame budget, rather than a fixed batch.
		   Columns are wildly uneven: getTile pulls a chunk into being and the
		   light settling behind it can run for a long time on a new world, so a
		   flat four-per-frame measured as a 1.2s freeze with the bar parked at
		   7%. Taking them singly bounds a stall to whatever one column costs,
		   and the budget still lets a fast machine chew through many per frame
		   rather than stretching a quick load over 256 needless frames. */
		/* Three phases with a drawn frame between them. The two tails cannot be
		   sliced -- saving the chunks and level->prepare() are each one call --
		   so what the frame in between buys is the stage *name* being on screen
		   before the work that earns it starts. Without it the last thing drawn
		   was the bar at 97% and it sat there through both, which is the same
		   complaint as the frozen menu, just shorter. */
		if (_prepInProgress) {
			if (_prepPhase == 0) {
				const int sliceStart = getTimeMs();
				bool passDone;
				do {
					passDone = prepareLevelChunkStep(1);
				} while (!passDone && (getTimeMs() - sliceStart) < 16);

				if (passDone) {
					// Painted next frame, before prepareLevelSave blocks.
					progressStageStatusId = 3;
					_prepPhase = 1;
				}
			} else if (_prepPhase == 1) {
				prepareLevelSave();
				// prepareLevelSave leaves the id on 3; move it on so the frame
				// after this one says "Preparing" rather than "Saving chunks".
				if (_prepNether) {
					// A new world also gets its Nether built now, so the first trip through a
					// portal reads it from disk (with its light) instead of generating it.
					prepareNetherBegin();
					progressStagePercentage = 0;
					progressStageStatusId = 1;
					_prepPhase = 3;
				} else {
					progressStagePercentage = -1;
					progressStageStatusId = 2;
					_prepPhase = 2;
				}
			} else if (_prepPhase == 3) {
				if (prepareNetherStep(16)) {
					prepareNetherFinish();
					progressStagePercentage = -1;
					progressStageStatusId = 2;
					_prepPhase = 2;
				}
			} else {
				prepareLevelPrepare();
				_prepInProgress = false;
				// What generateLevel() does at the end of the blocking path; the
				// next update() picks it up and calls _levelGenerated().
				isGeneratingLevel = false;
			}
		}
#endif
		return;
	} else if (!_hasSignaledGeneratingLevelFinished) {
		if (generateLevelThread) {
			delete generateLevelThread;
			generateLevelThread = NULL;
		}
		_levelGenerated();
	}

	//
	// Normal game loop, run before or efter level generation
	//
	if (level != NULL)
	{
		if (!pause) {
#ifndef STANDALONE_SERVER
			TIMER_POP_PUSH("gameRenderer");
			gameRenderer->tick(nTick, maxTick);

			TIMER_POP_PUSH("levelRenderer");
			levelRenderer->tick();
#endif
			level->difficulty = options.difficulty;
			if (level->isClientSide) level->difficulty = Difficulty::EASY;
			else if (level->getLevelData()) level->getLevelData()->setDifficulty(options.difficulty); // keeps the debug M toggle saved

			TIMER_POP_PUSH("level");
			level->tickEntities();
			level->tick();
			for (unsigned int i = 0; i < otherLevels.size(); ++i) {
				Level* other = otherLevels[i];
				other->difficulty = options.difficulty;
				other->tickEntities();
				other->tick();
				// Visitors travel by their own packets (see ServerSideNetworkHandler);
				// a request their portal left here is for nobody to answer.
				other->takePortalTravelRequest();
				// Its chunks are saved by its storage's tick, but its entities
				// only when it closes: a host whose tab died with visitors in
				// the Nether lost every mob, item and dropped thing there. Once
				// a minute, like a world's own save on pause.
				if (other->getTime() % 1200 == 0)
					other->saveGame();
			}
			// A dimension nobody is left in is saved and let go, every few
			// seconds rather than the tick it empties: a visitor stepping back
			// through a portal at once does not pay for loading it again.
			if (ticks % 200 == 0 && !otherLevels.empty())
				unloadEmptyLevels();
			tickBackgroundGeneration(4);
#ifndef STANDALONE_SERVER
			TIMER_POP_PUSH("animateTick");
			if (player) {
				level->animateTick(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));
				// Specks hanging in the Aquarium's water (Java's underwater
				// particle). Its water is air, so no tile's animateTick makes them.
				if (level->dimension->isAllWater()) {
					static Random specks;
					for (int i = 0; i < 8; ++i) {
						const float px = player->x + (specks.nextFloat() * 2 - 1) * 16;
						const float py = player->y + (specks.nextFloat() * 2 - 1) * 12;
						const float pz = player->z + (specks.nextFloat() * 2 - 1) * 16;
						if (level->getTile(Mth::floor(px), Mth::floor(py), Mth::floor(pz)) == 0)
							level->addParticle("suspended", px, py, pz, 0, 0, 0);
					}
				}
			}
#endif
		}
	}
#ifndef STANDALONE_SERVER
	textures->loadAndBindTexture("terrain.png");
	if (!pause && !(screen && !screen->renderGameBehind())) {
		#if !defined(RPI)
			#ifdef __APPLE__
			if (isSuperFast())
			#endif
			{
			if (nTick == maxTick) {
				TIMER_POP_PUSH("textures");
				textures->tick(true);
			}
			}
		#endif
	}
	TIMER_POP_PUSH("particles");
	particleEngine->tick();
	if (screen) {
		screenMutex = true;
		screen->tick();
		screenMutex = false;
	}

	// @note: fix to keep "isPressed" and "isReleased" as long as necessary.
	//      Most likely keyboard and mouse could/should be reset here as well.
	Multitouch::reset();
#endif
	TIMER_POP();
}

class InputRAII {
public:
	~InputRAII() {
#ifndef STANDALONE_SERVER
		Mouse::reset();
		Keyboard::reset();
#endif
	}
};

void Minecraft::tickInput() {
#ifndef STANDALONE_SERVER
	InputRAII raiiInput;

	if (screen && !screen->passEvents) {
		screenMutex = true;
		screen->updateEvents();
		//screen->updateSetScreen();
		screenMutex = false;
		if (hasScheduledScreen) {
			setScreen(scheduledScreen);
			scheduledScreen = NULL;
			hasScheduledScreen = false;
		}
		return;
	}

	if (!player) {
		return;
	}

// MC_SDL2: this is about having a real pointer to grab, as opposed to a
// touchscreen, which is what the #else branch below assumes.
#if defined(RPI) || defined(MC_SDL2)
	// While the mouse is grabbed the cursor is parked wherever it was last seen,
	// so routing clicks to the GUI would let a dig click also hit the hotbar.
	bool mouseDiggable = true;
	bool allowGuiClicks = !mouseGrabbed;
#else
	bool mouseDiggable = !gui.isInside(Mouse::getX(), Mouse::getY());
	bool allowGuiClicks = true;
#endif

	TIMER_PUSH("mouse");
	while (Mouse::next()) {
		//if (Mouse::getButtonState(MouseAction::ACTION_LEFT))
		//	LOGI("mouse-down-at: %d, %d\n", Mouse::getX(), Mouse::getY());
        	int passedTime = getTimeMs() - lastTickTime;
        	if (passedTime > 200) continue; // @note: As long Mouse::clear CLEARS the whole buffer, it's safe to break here
		// But since it might be rewritten anyway (and hopefully there aren't a lot of messages, we just continue.

		const MouseAction& e = Mouse::getEvent();

// On the web this click is doubly important: browsers only grant pointer lock
// from inside a user gesture, so the grab has to originate from a real click.
#if defined(RPI) || defined(MC_SDL2) // If clicked when not having focus, get focus @keyboard
		if (!mouseGrabbed) {
			if (!screen && e.data == MouseAction::DATA_DOWN) {
				grabMouse();
			}
		}
#endif

		if (allowGuiClicks && e.action == MouseAction::ACTION_LEFT && e.data == MouseAction::DATA_DOWN) {
			gui.handleClick(MouseAction::ACTION_LEFT, Mouse::getX(), Mouse::getY());
		}

		if (e.action == MouseAction::ACTION_WHEEL) {
			Inventory* v = player->inventory;
			int numSlots = gui.getNumSlots() - 1;
			int slot = (v->selected - e.dy + numSlots) % numSlots;
			v->selectSlot(slot);
		}
		/*
		if (mouseDiggable && options.useMouseForDigging) {
			if (Mouse::getEventButton() == MouseAction::ACTION_LEFT && Mouse::getEventButtonState()) {
				handleMouseClick(MouseAction::ACTION_LEFT);
				lastClickTick = ticks;
			}
			if (Mouse::getEventButton() == MouseAction::ACTION_RIGHT && Mouse::getEventButtonState()) {
				handleMouseClick(MouseAction::ACTION_RIGHT);
				lastClickTick = ticks;
			}
		}
		*/
	}

	TIMER_POP_PUSH("keyboard");
	while (Keyboard::next()) {
		int key = Keyboard::getEventKey();
		bool isPressed = (Keyboard::getEventKeyState() == KeyboardAction::KEYDOWN);
		player->setKey(key, isPressed);

		if (isPressed) {
			gui.handleKeyPressed(key);

			// Number keys pick a hotbar slot. This was gated to Win32 and the
			// Pi, so every SDL build -- macOS and web included -- had no way
			// to change slot but the scroll wheel.
			#if defined(WIN32) || defined(RPI) || defined(MC_SDL2)
				if (key >= '0' && key <= '9') {
					int digit = key - '0';
					int slot = digit - 1;

					if (slot >= 0 && slot < gui.getNumSlots()-1)
						player->inventory->selectSlot(slot);

					#if defined(WIN32) && !defined(MC_SDL2)
						if (isDebugWorld() && digit >= 1 && GetAsyncKeyState(VK_CONTROL) < 0) {
							// Set adventure settings here!
							AdventureSettingsPacket p(level->adventureSettings);
							p.toggle((AdventureSettingsPacket::Flags)(1 << slot));
							p.fillIn(level->adventureSettings);
							raknetInstance->send(p);
						}
						if (isDebugWorld() && digit == 0) {
							Pos pos((int)player->x, (int)player->y-1, (int)player->z);
							SetSpawnPositionPacket p(pos);
							raknetInstance->send(p);
						}
					#endif
				}
			#endif
			#if defined(RPI)
				if (key == Keyboard::KEY_E) {
					screenChooser.setScreen(SCREEN_BLOCKSELECTION);
				}
				if (!screen && key == Keyboard::KEY_O || key == 250) {
					releaseMouse();
				}
			#endif

			#if defined(MC_SDL2)
				// E opens the armour screen, which now doubles as the player's
				// inventory. The screen closes itself on a second E (see
				// ArmorScreen::keyPressed), so this only runs when no screen is
				// up. The old G (armour) and Q (craft) shortcuts are gone;
				// crafting is now a button on the HUD.
				if (key == Keyboard::KEY_E)
					setScreen(new ArmorScreen());
				// T opens the chat, in every mode (the commands need Allow Cheats).
				if (key == Keyboard::KEY_T)
					setScreen(new ChatScreen());
				// F5 cycles the camera: first person -> behind -> in front. It
				// sat among the debug-world keys and so did nothing in a real
				// world (the page keeps F5 from reloading the tab).
				if (key == Keyboard::KEY_F5)
					options.toggle(&Options::Option::THIRD_PERSON, 1);
			#endif
			// The developer keys, now only in a world made with Debug Mode on. They
			// used to be Windows-only; the SDL builds get them too.
			#if defined(WIN32) || defined(MC_SDL2)
			if (isDebugWorld()) {
				if (key == Keyboard::KEY_F) {
					options.isFlying = !options.isFlying;
					player->noPhysics = options.isFlying;
				}


				if (key == Keyboard::KEY_O) {
					useAmbientOcclusion = !useAmbientOcclusion;
					options.ambientOcclusion = useAmbientOcclusion;
					levelRenderer->allChanged();
				}

				if (key == Keyboard::KEY_L)
					options.viewDistance = (options.viewDistance + 1) % 4;

				if (key == Keyboard::KEY_U) {
					onGraphicsReset();
					player->heal(100);
				}

				if (key == Keyboard::KEY_B || key == 108) // Toggle the game mode
					setIsCreativeMode(!isCreativeMode());

				if (key == Keyboard::KEY_P) // Step forward in time
					level->setTime( level->getTime() + 1000);

				if (key == Keyboard::KEY_Y) {
					textures->reloadAll();
					player->hurtTo(2);
				}
				if (key == Keyboard::KEY_Z || key == 108) {
					for (int i = 0; i < 1; ++i) {
						Mob* mob = NULL;
						int forceId = 0;//MobTypes::Sheep;

						int types[] = {
							MobTypes::Sheep,
							MobTypes::Pig,
							MobTypes::Chicken,
							MobTypes::Cow,
						};

						int mobType = (forceId > 0)? forceId : types[Mth::random(sizeof(types) / sizeof(int))];
						mob = MobFactory::CreateMob(mobType, level);

						//((Animal*)mob)->setAge(-1000);
						float dx = 4 - 8 * Mth::random() + 4 * Mth::sin(Mth::DEGRAD * player->yRot);
						float dz = 4 - 8 * Mth::random() + 4 * Mth::cos(Mth::DEGRAD * player->yRot);
						if (mob && !MobSpawner::addMob(level, mob, player->x + dx, player->y, player->z + dz, Mth::random()*360, 0, true))
							delete mob;
					}
				}

				if (key == Keyboard::KEY_X) {
					const EntityList& entities = level->getAllEntities();
					for (int i = entities.size()-1; i >= 0; --i) {
						Entity* e = entities[i];
						if (!e->isPlayer())
							level->removeEntity(e);
					}
				}

				if (key == Keyboard::KEY_C /*|| key == 4*/) {
					player->inventory->clearInventoryWithDefault();
					// @todo: Add saving here for benchmarking
				}
				if (key == Keyboard::KEY_H) {
					setScreen( new PrerenderTilesScreen() );
				}

				if (key == Keyboard::KEY_O) {
					for (int i = Inventory::MAX_SELECTION_SIZE; i < player->inventory->getContainerSize(); ++i)
						if (player->inventory->getItem(i))
							player->inventory->dropSlot(i, false);
				}
				if (key == Keyboard::KEY_F3) {
					options.renderDebug = !options.renderDebug;
				}
				if (key == Keyboard::KEY_M) {
					options.difficulty = (options.difficulty == Difficulty::PEACEFUL)?
						Difficulty::NORMAL : Difficulty::PEACEFUL;
					//setIsCreativeMode( !isCreativeMode() );
				}

				if (options.renderDebug) {
					if (key >= '0' && key <= '9') {
						_perfRenderer->debugFpsMeterKeyPress(key - '0');
					}
				}
			}
			#endif

			#if defined(MC_SDL2)
				// Escape hands the cursor back first; pressing it again (with
				// the mouse already free) opens the pause menu.
				//
				// In a browser the first step is done for us: Escape is how the
				// user exits pointer lock, and the browser swallows that
				// keypress rather than delivering it. main_sdl.h watches for the
				// lock being dropped and releases the grab to match, so the
				// second press -- which does arrive -- lands here and pauses.
				if (key == Keyboard::KEY_ESCAPE) {
					if (mouseGrabbed)
						releaseMouse();
					else
						pauseGame(false);
				}
			#elif !defined(RPI)
				if (key == 82)
					pauseGame(false);
			#else
				if (key == Keyboard::KEY_ESCAPE)
					pauseGame(false);
			#endif

			#ifndef OPENGL_ES
				if (key == Keyboard::KEY_P && isDebugWorld()) {
					static bool isWireFrame = false;
					isWireFrame = !isWireFrame;
					glPolygonMode(GL_FRONT, isWireFrame? GL_LINE : GL_FILL);
					//glPolygonMode(GL_BACK, isWireFrame? GL_LINE : GL_FILL);
				}
			#endif
		}
		#if defined(WIN32) || defined(MC_SDL2)
			if (isPressed && key == Keyboard::KEY_M && isDebugWorld()) {
				for (int i = 0; i < 5 * SharedConstants::TicksPerSecond; ++i)
					level->tick();
			}
		#endif


		//if (!isPressed) LOGI("Key released: %d\n", key);

		if (!options.useMouseForDigging) {
			int passedTime = getTimeMs() - lastTickTime;
			if (passedTime > 200) continue;

			// Destroy and attack is on same button
			if (key == options.keyDestroy.key && isPressed) {
				BuildActionIntention bai(BuildActionIntention::BAI_REMOVE | BuildActionIntention::BAI_ATTACK);
				handleBuildAction(&bai);
			}
			else // Build and use/interact is on same button
			if (key == options.keyUse.key && isPressed) {
				BuildActionIntention bai(BuildActionIntention::BAI_BUILD | BuildActionIntention::BAI_INTERACT);
				handleBuildAction(&bai);
			}
		}
	}

	TIMER_POP_PUSH("tickbuild");
	BuildActionIntention bai;
	// @note: This might be a problem. This method is polling based here, but
	//        event based when using mouse (or keys..), and in java. Not quite
	//        sure just yet what way to go.
	bool buildHandled = inputHolder->getBuildInput()->tickBuild(player, &bai);
	if (buildHandled) {
		if (!bai.isRemoveContinue())
			handleBuildAction(&bai);
	}

	bool isTryingToDestroyBlock = (options.useMouseForDigging
			?	(Mouse::isButtonDown(MouseAction::ACTION_LEFT) && mouseDiggable)
			:	Keyboard::isKeyDown(options.keyDestroy.key))
		||	(buildHandled && bai.isRemove());

	TIMER_POP_PUSH("handlemouse");
#ifdef RPI
	handleMouseDown(MouseAction::ACTION_LEFT, isTryingToDestroyBlock);
	handleMouseClick(buildHandled && bai.isInteract()
		|| options.useMouseForDigging && Mouse::isButtonDown(MouseAction::ACTION_RIGHT));
#else
	// A pure "interact" (the touch pad's hold-to-use) counts as the button held
	// down; a mouse right-click is BUILD | INTERACT and must not -- it was fed
	// to handleMouseDown as a held LEFT button for the tick it was handled, and
	// whatever the crosshair still pointed at (in creative, the block just
	// placed against) was dug up.
	handleMouseDown(MouseAction::ACTION_LEFT, isTryingToDestroyBlock || (buildHandled && bai.isInteract() && !bai.isBuild()));
#endif

	lastTickTime = getTimeMs();

	// we have (hopefully) handled the keyboard & mouse queue and it
	// can now be emptied. If wanted, the reset could be changed to:
	// index -= numRead; // then this code doesn't have to be placed here
	// + it prepares for tick not handling all or any events.
	// update: RAII'ing instead, see above
	//Keyboard::reset();
	//Mouse::reset();

	TIMER_POP();
#endif
}

void Minecraft::handleMouseDown(int button, bool down) {
#ifndef STANDALONE_SERVER
#ifndef RPI
	if(player->isUsingItem()) {
		// keyUse is a keyboard key; with a mouse the right button is what holds
		// the bow drawn. Without this the bow was let go the tick after it was
		// drawn, as a shot too weak to leave the bow.
		if(!down && !Keyboard::isKeyDown(options.keyUse.key) && !Mouse::getButtonState(MouseAction::ACTION_RIGHT)) {
			gameMode->releaseUsingItem(player);
		}
		return;
	}
#endif
	if(player->isSleeping()) {
		return;
	}
    if (button == MouseAction::ACTION_LEFT && missTime > 0) return;
	if (down && hitResult.type == TILE && button == MouseAction::ACTION_LEFT && !hitResult.indirectHit) {
        int x = hitResult.x;
        int y = hitResult.y;
        int z = hitResult.z;
        gameMode->continueDestroyBlock(x, y, z, hitResult.f);
        particleEngine->crack(x, y, z, hitResult.f);
		player->swing();
    } else {
        gameMode->stopDestroyBlock();
    }
#endif
}

void Minecraft::handleMouseClick(int button) {
//	BuildActionIntention bai(
//		(button == MouseAction::ACTION_LEFT)?
//			BuildActionIntention::BAI_REMOVE
//		:	BuildActionIntention::BAI_BUILD);
//
//	handleBuildAction(&bai);
}

void Minecraft::handleBuildAction(BuildActionIntention* action) {
#ifndef STANDALONE_SERVER
	if (action->isRemove()) {
		if (missTime > 0) return;
		player->swing();
	}
	if(player->isUsingItem())
		return;
    bool mayUse = true;

	if (!hitResult.isHit()) {
		if (action->isRemove() && !gameMode->isCreativeType()) {
			missTime = 10;
		}
    } else if (hitResult.type == ENTITY) {
        if (action->isAttack()) {
			player->swing();
			//LOGI("attacking!\n");
			InteractPacket packet(InteractPacket::Attack, player->entityId, hitResult.entity->entityId);
			raknetInstance->send(packet);
            gameMode->attack(player, hitResult.entity);
		} else if (action->isInteract()) {
			if (hitResult.entity->interactPreventDefault())
				mayUse = false;
			//LOGI("interacting!\n");
			InteractPacket packet(InteractPacket::Interact, player->entityId, hitResult.entity->entityId);
			raknetInstance->send(packet);
            gameMode->interact(player, hitResult.entity);
        }
    } else if (hitResult.type == TILE) {
		int x = hitResult.x;
        int y = hitResult.y;
        int z = hitResult.z;
        int face = hitResult.f;

		int oldTileId = level->getTile(x, y, z);
        Tile* oldTile = Tile::tiles[oldTileId];

		//bool tryDestroyBlock = false;

		if (action->isRemove()) {
			if (!oldTile)
				return;

			//LOGI("tile: %s - %d, %d, %d. b: %f - %f\n", oldTile->getDescriptionId().c_str(), x, y, z, oldTile->getBrightness(level, x, y, z), oldTile->getBrightness(level, x, y+1, z));
            level->extinguishFire(x, y, z, hitResult.f);
			gameMode->startDestroyBlock(x, y, z, hitResult.f);
        }
		else {
			ItemInstance* item = player->inventory->getSelected();
 			if (gameMode->useItemOn(player, level, item, x, y, z, face, hitResult.pos)) {
			    mayUse = false;
			    player->swing();
			#ifdef RPI
				} else if (item && item->id == ((Item*)Item::sword_iron)->id) {
					player->swing();
			#endif
			}
			if (item && item->count <= 0) {
				player->inventory->clearSlot(player->inventory->selected);
			}
			//} else if (item && item->count != oldCount) {
			//	  gameRenderer->itemInHandRenderer->itemPlaced();
			//}
		}
	}
	if (mayUse && action->isInteract()) {
		ItemInstance* item = player->inventory->getSelected();
		if (item && !player->isUsingItem()) {
			if (gameMode->useItem(player, level, item)) {
				gameRenderer->itemInHandRenderer->itemUsed();
			}
			if (item && item->count <= 0) {
				player->inventory->clearSlot(player->inventory->selected);
			}
		}
	}
#endif
}

bool Minecraft::isOnlineClient()
{
    return (level != NULL && level->isClientSide);
}

bool Minecraft::isOnline()
{
	return netCallback != NULL;
}

void Minecraft::pauseGame(bool isBackPaused) {
#ifndef STANDALONE_SERVER
	if (screen != NULL) return;
	screenChooser.setScreen(isBackPaused? SCREEN_PAUSEPREV : SCREEN_PAUSE);
#endif
}
void Minecraft::gameLostFocus() {
#ifndef STANDALONE_SERVER
	if(screen != NULL) {
		screen->lostFocus();
	}
#endif
}


void Minecraft::setScreen( Screen* screen )
{
#ifndef	STANDALONE_SERVER
	// The inventory, armour and crafting screens wait for /login.
	if (inputLocked && screen && (dynamic_cast<ArmorScreen*>(screen) || dynamic_cast<IngameBlockSelectionScreen*>(screen)
			|| dynamic_cast<Touch::IngameBlockSelectionScreen*>(screen) || dynamic_cast<WorkbenchScreen*>(screen))) {
		delete screen;
		gui.addMessage("Primero inicia sesion: /login <contrasena> o /register <contrasena>.");
		return;
	}
	Mouse::reset();
	Multitouch::reset();
	Multitouch::resetThisUpdate();

	if (screenMutex) {
		hasScheduledScreen = true;
		scheduledScreen = screen;
		return;
	}

	if (screen != NULL && screen->isErrorScreen())
		return;
	if (screen == NULL && level == NULL)
		screen = screenChooser.createScreen(SCREEN_STARTMENU);

	if (this->screen != NULL) {
		this->screen->removed();
		delete this->screen;
	}

	this->screen = screen;
	if (screen != NULL) {
		releaseMouse();
		//ScreenSizeCalculator ssc = new ScreenSizeCalculator(options, width, height);
		int screenWidth = (int)(width * Gui::InvGuiScale); //ssc.getWidth();
		int screenHeight = (int)(height * Gui::InvGuiScale); //ssc.getHeight();
		screen->init(this, screenWidth, screenHeight);

		if (screen->isInGameScreen() && level) {
			level->saveLevelData();
            level->saveGame();
        }

		//noRender = false;
	} else {
		grabMouse();
	}
#endif
}

/** Menu music, which 0.6.1 never had. SoundEngine only ever plays one-shots --
    everything in data/sound is about a second long -- but Options has carried a
    Music slider since 2013 with nothing behind it, so that is what this drives.

    The track is streamed by the page rather than played through OpenAL: it is
    two orders of magnitude longer than any sound in the game, and that path
    decodes a whole file into memory before playing it. See the music block in
    shell.html.

    Music plays whenever no world is loaded, which is exactly the menus -- the
    title screen, the level list, the settings -- and stops on the way into a
    world. This runs every frame, so it only speaks when the answer changes. */
void Minecraft::updateMusic()
{
#if defined(__EMSCRIPTEN__) && !defined(STANDALONE_SERVER)
	const bool on = (level == NULL) && (options.music > 0.0f);
	const float volume = options.music;

	if (on == _musicOn && volume == _musicVolume)
		return;

	_musicOn = on;
	_musicVolume = volume;

	EM_ASM({
		if (window.mcpeMusic) window.mcpeMusic.set($0 != 0, $1);
	}, on ? 1 : 0, (double)volume);
#endif
}

void Minecraft::grabMouse()
{
#ifndef STANDALONE_SERVER
	if (mouseGrabbed) return;

	// There is no cursor to capture on a touchscreen: looking around is the
	// drag itself, and the input holder reads the raw pointers. This is the one
	// gate that matters, because setScreen(NULL) grabs on the way out of every
	// menu -- and on the web a grab means asking the browser for pointer lock,
	// which a phone would either refuse or, worse, honour.
	if (useTouchscreen()) return;

	mouseGrabbed = true;
	mouseHandler.grab();
	//setScreen(NULL);
#endif
}

void Minecraft::releaseMouse()
{
#ifndef STANDALONE_SERVER
	if (!mouseGrabbed) {
		return;
	}
	if (player) {
		player->releaseAllKeys();
	}
	mouseGrabbed = false;
	mouseHandler.release();
#endif
}

bool Minecraft::useTouchscreen() {
#ifdef RPI
	return false;
#endif
	return options.useTouchScreen || !_supportsNonTouchscreen;
}
bool Minecraft::supportNonTouchScreen() {
	return _supportsNonTouchscreen;
}
void Minecraft::init()
{
	options.minecraft = this;
#ifndef STANDALONE_SERVER
	// Has to be known *before* initDefaultValues(), which picks the touch vs
	// keyboard control defaults from supportNonTouchScreen(). It used to be set
	// afterwards, so the default read back as the constructor's `false` and
	// every platform silently forced useTouchScreen on.
	_supportsNonTouchscreen = !platform()->supportsTouchscreen();
#endif
	options.initDefaultValues();
#ifndef STANDALONE_SERVER
	checkGlError("Init enter");

	LOGI("IS TOUCHSCREEN? %d\n", options.useTouchScreen);

	textures = new Textures(&options, platform());
	textures->addDynamicTexture(new WaterTexture());
	textures->addDynamicTexture(new WaterSideTexture());
	textures->addDynamicTexture(new PortalTexture());
	textures->addDynamicTexture(new PortalTexture(Tile::aquariumPortal->tex, true));
	textures->addDynamicTexture(new FireTexture(Tile::fire->tex, 0));
	textures->addDynamicTexture(new FireTexture(Tile::fire->tex + 16, 11));
	gui.texturesLoaded(textures);

	levelRenderer = new LevelRenderer(this);
	gameRenderer = new GameRenderer(this);
	particleEngine = new ParticleEngine(level, textures);

	// Platform specific initialization here
	font = new Font(&options, "font/default8.png", textures);

	_perfRenderer = new PerfRenderer(this, font);

	checkGlError("Init complete");
#endif

	user = new User("TestUser", "");
	setIsCreativeMode(false); // false means it's Survival Mode
	reloadOptions();

}

void Minecraft::setSize(int w, int h) {
#ifndef STANDALONE_SERVER
    transformResolution(&w, &h);

	width  = w;
	height = h;

	if (width >= 1000) {
        #ifdef __APPLE__
            Gui::GuiScale = (width > 2000)? 8.0f : 4.0f;
        #else
            Gui::GuiScale = 4.0f;
        #endif
    }
	else if (width >= 800) {
#ifdef __APPLE__
        Gui::GuiScale = 4.0f;
#else
		Gui::GuiScale = 3.0f;
#endif
    }
	else if (width >= 400)
		Gui::GuiScale = 2.0f;
	else
		Gui::GuiScale = 1.0f;

	// The ladder above reads width alone, which stood in fine for screen size
	// when every device this shipped on was 16:9 or squarer. A phone held
	// sideways today is 2.2:1 or wider, so an 863x360 viewport picks the scale
	// meant for a desktop window and the pause menu's last button lands below
	// the bottom of the screen. Step back down until the tallest screen fits:
	// PauseScreen puts its third button at y=112 in GUI units and it is about
	// 24 tall, so nothing may be laid out in less than 140 of them.
	const float MinGuiHeight = 140.0f;
	while (Gui::GuiScale > 1.0f && (float)height < MinGuiHeight * Gui::GuiScale)
		Gui::GuiScale -= 1.0f;

	Gui::InvGuiScale = 1.0f / Gui::GuiScale;
	int screenWidth  = (int)(width  * Gui::InvGuiScale);
	int screenHeight = (int)(height * Gui::InvGuiScale);

	if (platform()) {
		float pixelsPerMillimeter = options.getProgressValue(&Options::Option::PIXELS_PER_MILLIMETER);
		pixelCalc.setPixelsPerMillimeter(pixelsPerMillimeter);
		pixelCalcUi.setPixelsPerMillimeter(pixelsPerMillimeter * Gui::InvGuiScale);
	}

	Config config = createConfig(this);
	gui.onConfigChanged(config);
    
	if (screen)
		screen->setSize(screenWidth, screenHeight);

	if (inputHolder)
		inputHolder->onConfigChanged(config);
	//LOGI("Setting size: %d, %d: %f\n", width, height, Gui::InvGuiScale);

#ifdef WIN32
	char resbuf[128];
	sprintf(resbuf, "            %d x %d @ scale %.2f", width, height, Gui::GuiScale);
	//gui.addMessage(resbuf);
#endif
#endif /* STANDALONE_SERVER */
}

void Minecraft::reloadOptions() {
	// options.txt is otherwise resolved relative to the working directory --
	// wherever the binary happened to be launched from on the desktop, and a
	// throwaway MEMFS root in the browser. Put it next to the saves instead.
	options.setStorageDirectory(externalStoragePath);

	options.update();
	options.save();
	bool wasTouchscreen = options.useTouchScreen;
	options.useTouchScreen = useTouchscreen();
	options.save();

	if ((wasTouchscreen != options.useTouchScreen) || (inputHolder == 0))
		_reloadInput();

	user->name = options.username;
	// Options were just written; see WebBridge::flushSaves.
	WebBridge::flushSaves();

	LOGI("Reloading-options\n");

    // @todo @fix Android and iOS behaves a bit differently when leaving
    //            an options screen (Android recreates OpenGL surface)
    setSize(width, height);
}

void Minecraft::setLocalSkin(int w, int h, const unsigned char* rgba) {
	const int id = rgba ? PlayerSkins::create(w, h, rgba) : 0;
	if (rgba && !id)
		return;	// not a skin; keep the one we have
	PlayerSkins::release(PlayerSkins::localSkin);
	PlayerSkins::localSkin = id;

	if (netCallback && raknetInstance && level && player) {
		const PlayerSkins::Skin* skin = PlayerSkins::get(id);
		PlayerSkinPacket packet(player->entityId, skin ? skin->w : 0, skin ? skin->h : 0,
		                        skin ? &skin->rgba[0] : 0);
		raknetInstance->send(packet);
	}
}

void Minecraft::_reloadInput() {
#ifndef STANDALONE_SERVER
	delete inputHolder;

	if (useTouchscreen()) {
		inputHolder = new TouchInputHolder(this, &options);
	} else {
		#if defined(ANDROID) || (defined(__APPLE__) && TARGET_OS_IPHONE)
			inputHolder = new CustomInputHolder(
				new XperiaPlayInput(&options),
				new ControllerTurnInput(2, ControllerTurnInput::MODE_DELTA),
				new IBuildInput());
		#else
			inputHolder = new CustomInputHolder(
				new KeyboardInput(&options),
				new MouseTurnInput(MouseTurnInput::MODE_DELTA, width/2, height/2),
				new MouseBuildInput());
		#endif
	}

	mouseHandler.setTurnInput(inputHolder->getTurnInput());
	if (level && player) {
		player->input = inputHolder->getMoveInput();
	}
#endif
}


//
// Multiplayer
//
void Minecraft::locateMultiplayer() {
	isLookingForMultiplayer = true;

	raknetInstance->pingForHosts(19132);
	netCallback = new ClientSideNetworkHandler(this, raknetInstance);
}

void Minecraft::cancelLocateMultiplayer() {
	isLookingForMultiplayer = false;

	raknetInstance->stopPingForHosts();

	delete netCallback;
	netCallback = NULL;
}

bool Minecraft::joinMultiplayer( const PingedCompatibleServer& server )
{
	if (isLookingForMultiplayer && netCallback) {
		isLookingForMultiplayer = false;
		return raknetInstance->connect(server.address.ToString(false), server.address.GetPort());
	}
	return false;
}

void Minecraft::hostMultiplayer(int port) {
    // Tear down last instance
    raknetInstance->disconnect();
    delete netCallback;
    netCallback = NULL;

#if !defined(NO_NETWORK)
	netCallback = new ServerSideNetworkHandler(this, raknetInstance);
    #ifdef STANDALONE_SERVER
        raknetInstance->host(user->name, port, 16);
    #else
        raknetInstance->host(user->name, port);
    #endif
#endif
}

//
// Level generation
//
/*static*/

	void* Minecraft::prepareLevel_tspawn(void *p_param)
{
	Minecraft* mc = (Minecraft*) p_param;
	mc->generateLevel("Currently not used", mc->level);
	return 0;
}

void Minecraft::generateLevel( const std::string& message, Level* level )
{
	Stopwatch s;
	s.start();
	prepareLevel(message);
	s.stop();
	s.print("Level generated: ");

	// "Unlock"
	isGeneratingLevel = false;
}

void Minecraft::_levelGenerated()
{
	// After a portal the player is the one who left the other level, not a new one.
	bool arrivedByPortal = false;
	if (player == NULL && _travelPlayer != NULL) {
		player = _travelPlayer;
		_travelPlayer = NULL;
		player->setLevel(level);
		arrivedByPortal = true;
		// Generation takes far longer than the cooldown, so arm it again now.
		_portalCooldown = PORTAL_COOLDOWN_TICKS;
	}
#ifndef STANDALONE_SERVER
	if (player == NULL) {
		player = (LocalPlayer*) gameMode->createPlayer(level);
		gameMode->initPlayer(player);
	}

	if (player) {
		player->input = inputHolder->getMoveInput();
	}

	if (levelRenderer != NULL) levelRenderer->setLevel(level);
	if (particleEngine != NULL) particleEngine->setLevel(level);

	gameMode->adjustPlayer(player);
	gui.onLevelGenerated();
#endif

	// A visitor's chunks are blank until the host's arrive, and validateSpawn
	// walks around looking for solid ground until it finds some: forever, on
	// an empty level. The host gave the position; there is nothing to check.
	if (!level->isClientSide)
		level->validateSpawn();
	level->loadPlayer(player, true);
	// level.dat was written during generation with nobody in the level, so write
	// it again now that the player is in, or a reload would start him at spawn.
	if (arrivedByPortal && player) level->saveLevelData();
	// if we are client side, we trust the server to have given us a correct position
	if (player && !level->isClientSide) {
		player->resetPos(false);
	}

	this->cameraTargetPlayer = player;

	if (raknetInstance->isServer())
		raknetInstance->announceServer(user->name);

	if (netCallback) {
		netCallback->levelGenerated(level);
	}

#if (defined(WIN32) || defined(RPI)) && !defined(MC_SDL2)
	if (_commandServer) {
		delete _commandServer;
	}
	_commandServer = new CommandServer(this);
	_commandServer->init(commandPort);
#endif

	// Hack to (hopefully) get the players to show (note: in LevelListener
	// instead, since adding yourself always generates a entityAdded)
	//EntityRenderDispatcher::getInstance()->onGraphicsReset();
	_hasSignaledGeneratingLevelFinished = true;
	// The chunks the quick start left out come now, a little a tick.
	_genBackground = (unsigned int) _prepCount;
}

Player* Minecraft::respawnPlayer(int playerId) {
	for (unsigned int i = 0; i < level->players.size(); ++i) {
		if (level->players[i]->entityId == playerId) {
			resetPlayer(level->players[i]);
			return level->players[i];
		}
	}
	return NULL;
}

void Minecraft::resetPlayer(Player* player) {
	level->validateSpawn();
	player->reset();

	Pos p;
	if(player->hasRespawnPosition()) {
		p = player->getRespawnPosition();
	}
	else {
		p = level->getSharedSpawnPos();
	}
	player->setPos((float)p.x + 0.5f, (float)p.y + 1.0f, (float)p.z + 0.5f);
	player->resetPos(true);

	if (isCreativeMode())
		player->inventory->clearInventoryWithDefault();
}

void Minecraft::respawnPlayer() {
	// RESET THE FRACKING PLAYER HERE
	//bool slowCheck = false;
	//for (int i = 0; i < level->entities.size(); ++i)
	//	if (level->entities[i] == player) slowCheck = true;
	//
	//LOGI("Has entity? %d, %d\n", level->getEntity(player->entityId), slowCheck);

	// Dying in the Nether or the Aquarium sends the player home, as in the other
	// editions: he comes back in the Overworld, not at that dimension's spawn.
	if (level && player && !level->isClientSide && level->dimension
		&& !Dimension::isOverworld(level->dimension->id)) {
		player->reset();
		if (isCreativeMode())
			player->inventory->clearInventoryWithDefault();
		changeDimension(Dimension::NORMAL, true);
		return;
	}

	resetPlayer(player);

	// tell server (or other client) that we re-spawned
	RespawnPacket packet(player);
	raknetInstance->send(packet);
}

void Minecraft::onGraphicsReset()
{
#ifndef STANDALONE_SERVER
	textures->clear();
	PlayerSkins::onGraphicsReset();
	
	font->onGraphicsReset();
	gui.onGraphicsReset();

	if (levelRenderer) levelRenderer->onGraphicsReset();
	if (gameRenderer) gameRenderer->onGraphicsReset();

	EntityRenderDispatcher::getInstance()->onGraphicsReset();
	TileEntityRenderDispatcher::getInstance()->onGraphicsReset();
#endif
}

int Minecraft::getProgressStatusId() {
	return progressStageStatusId;
}

const char* Minecraft::getProgressMessage()
{
	return progressMessages[progressStageStatusId];
}

bool Minecraft::isLevelGenerated()
{
	// A trip that is queued counts as not generated, so the loading screen stays
	// up and the old level is not drawn behind it.
	//
	// isGeneratingLevel going false and _levelGenerated() actually running are
	// two different frames (see the generation-progress block in update()):
	// the flag flips at the end of the frame that finishes the last prep
	// phase, but _levelGenerated() -- which is what calls
	// levelRenderer->setLevel(level), sizing its chunk array to this level --
	// only runs on the *next* frame's pass through here. Between those two
	// frames this would say "generated" a frame early, and renderLevel()
	// would cull whatever levelRenderer was last set up for: on a world
	// transition there is always a previous level's array still sitting
	// there, stale but a valid size, so this read as a harmless one-frame
	// glitch no one noticed. The first level a fresh tab ever loads -- a
	// client joining straight into multiplayer with no single-player world
	// before it -- has no such leftover array to fall back on, and the same
	// one-frame gap is a null chunk array instead, which is the crash this
	// guards against.
	return level != NULL && !isGeneratingLevel && _hasSignaledGeneratingLevelFinished && _pendingDim == 0;
}

LevelStorageSource* Minecraft::getLevelSource()
{
	return storageSource;
}

int Minecraft::getLicenseId() {
	if (!LicenseCodes::isReady(_licenseId))
		_licenseId = platform()->checkLicense();
	return _licenseId;
}

void Minecraft::audioEngineOn() {
#ifndef STANDALONE_SERVER
    soundEngine->enable(true);
#endif
}
void Minecraft::audioEngineOff() {
#ifndef STANDALONE_SERVER
    soundEngine->enable(false);
#endif
}

bool Minecraft::isDebugWorld()
{
	return level && !level->isClientSide && level->getLevelData() && level->getLevelData()->getDebug();
}

bool Minecraft::areCheatsAllowed()
{
	return level && !level->isClientSide && level->getLevelData() && level->getLevelData()->getCheats();
}

void Minecraft::setIsCreativeMode(bool isCreative)
{
#ifdef CREATORMODE
	delete gameMode;
	gameMode = new CreatorMode(this);
	_isCreativeMode = true;
#else
	if (!gameMode || isCreative != _isCreativeMode)
	{
		delete gameMode;
		if (isCreative) gameMode = new CreativeMode(this);
		else			gameMode = new SurvivalMode(this);
		_isCreativeMode = isCreative;
	}
#endif
	if (player) {
		gameMode->initAbilities(player->abilities);
		// The inventory goes with the mode: all the items in creative, empty in survival.
		if (player->inventory && player->inventory->isCreative() != _isCreativeMode)
			player->inventory->setCreative(_isCreativeMode);
	}
}

bool Minecraft::isCreativeMode() {
	return _isCreativeMode;
}

bool Minecraft::isKindleFire(int kindleVersion) {
	if (kindleVersion != 1)
		return false;

	std::string model = platform()->getPlatformStringVar(PlatformStringVars::DEVICE_BUILD_MODEL);
	std::string modelLower(model);
	std::transform(modelLower.begin(), modelLower.end(), modelLower.begin(), tolower);

	return (modelLower.find("kindle") != std::string::npos) && (modelLower.find("fire") != std::string::npos);
}

bool Minecraft::transformResolution(int* w, int* h)
{
	bool changed = false;

	// Kindle Fire 1: reporting wrong height in
	// certain cases (e.g. after screen lock)
	if (isKindleFire(1) && *h >= 560 && *h <= 620) {
		*h = 580;
		changed = true;
	}

	return changed;
}

ICreator* Minecraft::getCreator()
{
#ifdef CREATORMODE
	return ((CreatorMode*)gameMode)->getCreator();
#else
	return NULL;
#endif
}

void Minecraft::optionUpdated( const Options::Option* option, bool value ) {
	if(netCallback != NULL && option == &Options::Option::SERVER_VISIBLE) {
		ServerSideNetworkHandler* ss = (ServerSideNetworkHandler*) netCallback;
		ss->allowIncomingConnections(value);
	}
}

void Minecraft::optionUpdated( const Options::Option* option, float value ) {
#ifndef STANDALONE_SERVER
	if(option == &Options::Option::PIXELS_PER_MILLIMETER) {
		pixelCalcUi.setPixelsPerMillimeter(value * Gui::InvGuiScale);
		pixelCalc.setPixelsPerMillimeter(value);
	}
#endif
}

void Minecraft::optionUpdated( const Options::Option* option, int value ) {

}
