#ifndef NET_MINECRAFT_CLIENT__Minecraft_H__
#define NET_MINECRAFT_CLIENT__Minecraft_H__

#include "Options.h"
#ifndef STANDALONE_SERVER
#include "MouseHandler.h"
#endif
#include "Timer.h"
#include <string>
#include <vector>
#include "player/input/ITurnInput.h"
#ifndef STANDALONE_SERVER
#include "gui/Gui.h"
#include "gui/screens/ScreenChooser.h"
#endif
//#include "../network/RakNetInstance.h"
#include "../world/phys/HitResult.h"

class User;
class Level;
class LocalPlayer;
class IInputHolder;
class Mob;
class Player;
class LevelRenderer;
class GameRenderer;
class ParticleEngine;
class Entity;
class ICreator;
class GameMode;
class Textures;
class CThread;
class SoundEngine;
class Screen;
class Font;
class LevelStorageSource;
class BuildActionIntention;
class PerfRenderer;
class LevelSettings;
class IRakNetInstance;
class NetEventCallback;
class CommandServer;
struct PingedCompatibleServer;
//class ExternalFileLevelStorageSource;


#include "../App.h"
#include "PixelCalc.h"
class AppPlatform;
class AppPlatform_android;

class Minecraft: public App
{
protected:
	Minecraft();
public:
	virtual ~Minecraft();

	void init();
	void setSize(int width, int height);
	void reloadOptions();

	/// Makes an RGBA image the profile's skin (NULL: back to the default), and
	/// tells whoever we are playing with. Called from the web page, which owns
	/// the file picker and keeps the image between sessions.
	void setLocalSkin(int w, int h, const unsigned char* rgba);

	bool supportNonTouchScreen();
	bool useTouchscreen();
	void grabMouse();
	void releaseMouse();
	void updateMusic();

	void handleBuildAction(BuildActionIntention*);

	void toggleDimension(){}
	bool isCreativeMode();
	// The world was made with Debug Mode on: the developer keys work in it.
	bool isDebugWorld();
	// The world was made with Allow Cheats on: chat commands will check this.
	bool areCheatsAllowed();
	void setIsCreativeMode(bool isCreative);
	void setScreen(Screen*);

	/** A hosted world runs the host's Level (`level`) and, beside it, any
	    other dimension somebody is in (`otherLevels`: the Overworld, the
	    Nether, the Aquarium). One is made when the first player goes through
	    a portal to it, ticks beside the host's, and is saved and dropped when
	    nobody is left in it (unloadEmptyLevels). Hosts only. */
	std::vector<Level*> otherLevels;
	/** A visitor that has not logged in yet (/auth): it may chat, to type
	    /login, and nothing else -- no walking, no turning, no inventory. Set
	    by the host (ExtPacket::AuthLock). */
	bool inputLocked;
	/// The running Level of `dimension`; with `create`, the other one is made if it is not running.
	Level* levelForDimension(int dimension, bool create);
	void destroyOtherLevel(Level* other);
	void destroyOtherLevels();
	/// Saves and drops every other Level nobody is in or on the way to.
	void unloadEmptyLevels();
	/// Where somebody coming from `fromDim` to `dst` lands: the Aquarium's
	/// fixed portal, the Overworld portal they last took to it, or a Nether
	/// portal found or built there.
	void findArrival(Level* dst, int fromDim, float fromX, float fromZ, bool respawn, Player* who, int* outX, int* outY, int* outZ);

	virtual void selectLevel(const std::string& levelId, const std::string& levelName, const LevelSettings& settings);
	virtual void setLevel(Level* level, const std::string& message = "", LocalPlayer* forceInsertPlayer = NULL);

	void generateLevel( const std::string& message, Level* level );
	LevelStorageSource* getLevelSource();

	bool isLookingForMultiplayer;
	void locateMultiplayer();
	void cancelLocateMultiplayer();
	bool joinMultiplayer(const PingedCompatibleServer& server);
	void hostMultiplayer(int port=19132);
	Player* respawnPlayer(int playerId);
	void respawnPlayer();
	void resetPlayer(Player* player);
	void doActuallyRespawnPlayer();

	void update();

	void tick(int nTick, int maxTick);
	void tickInput();

	bool isOnlineClient();
	bool isOnline();
	void pauseGame(bool isBackPaused);
	void gameLostFocus();

	void prepareLevel(const std::string& message);

	/** prepareLevel, in three parts, so the web can run it across frames.

	    Every other platform generates on a thread while the main loop keeps
	    drawing ProgressScreen, which is where the loading bar comes from. There
	    are no threads in a browser, so prepareLevel ran inline and blocked the
	    one thread the page has: nothing painted, and a world load looked like
	    the menu freezing.

	    Nothing here is a new state for the game. isGeneratingLevel is already a
	    supported condition with the level half-built and the main loop running
	    -- it is exactly what the threaded platforms sit in -- so the web now
	    drives that same state cooperatively instead of from a thread.

	    prepareLevel() itself still calls all three back to back, so every other
	    platform runs the identical sequence it always did. */
	void prepareLevelBegin();
	/// Runs at most maxIterations of the chunk pass, or all of them when
	/// maxIterations <= 0. Returns true once the pass is finished.
	bool prepareLevelChunkStep(int maxIterations);
	/// The two blocking tails, split so the web can paint the stage name that
	/// goes with each before it starts. progressMessages has had "Saving chunks"
	/// and "Preparing" in it since 2013; on the web neither was ever on screen,
	/// because both ran inside one frame after the last one the bar drew.
	void prepareLevelSave();
	void prepareLevelPrepare();
	void prepareLevelFinish();
	// The Nether of a new world, built while it is created (see prepareNetherBegin).
	void prepareNetherBegin();
	bool prepareNetherStep(int maxMs);
	void prepareNetherFinish();

	void leaveGame(bool renameLevel = false);

	// Swaps the Overworld and the Nether around the local player. Called from
	// tick(), never mid-entity-update. Single-player only: see the guard in it.
	// respawn: the player died in the Nether and comes back at the Overworld's
	// spawn (or his bed), with no arrival portal.
	void changeDimensionShared(int dstDim, bool respawn);
	void changeDimension(int dstDim, bool respawn = false);
	/// Would a trip now keep the old Level (others in it, or another running)?
	bool travelKeepsLevels();
	/// Loads `dst`'s chunks nearest the arrival first, for maxMs; true when done.
	bool warmTravelLevel(Level* dst, int maxMs);

	int getProgressStatusId();
	const char* getProgressMessage();

	ICreator* getCreator();

	// void onGraphicsLost() {}
	void onGraphicsReset();

	bool isLevelGenerated();
	int getLicenseId();

    void audioEngineOn();
    void audioEngineOff();
    
	bool isPowerVR() { return _powerVr; }
	bool isKindleFire(int kindleVersion);
	bool transformResolution(int* w, int* h);
	void optionUpdated(const Options::Option* option, bool value);
	void optionUpdated(const Options::Option* option, float value);
	void optionUpdated(const Options::Option* option, int value);
#ifdef __APPLE__
    bool _isSuperFast;
    bool isSuperFast() { return _isSuperFast; }
#endif

protected:
	void _levelGenerated();

private:
	static void* prepareLevel_tspawn(void *p_param);

	// Ticks left before another portal trip is honoured, so the player who
	// arrives standing in the arrival portal does not bounce straight back.
	int _portalCooldown;

	// The player who went through a portal, held here while the other level is
	// generated. A new level has no player until _levelGenerated, and putting him
	// in earlier lets him fall through chunks that do not exist yet.
	LocalPlayer* _travelPlayer;

	// A portal trip is started one step before it is carried out: the loading
	// screen goes up first, and changeDimension runs two frames later, so the
	// work that blocks the page happens behind a screen that is already drawn.
	// The target dimension + 1; 0 = none pending.
	int _pendingDim;
	int _pendingDimFrames;
	// A trip into a Level made just for it loads that Level's chunks over
	// several frames behind the loading screen first, nearest the arrival
	// first, rather than all on the frame the host steps through.
	Level* _warmLevel;
	std::vector<int> _warmOrder;
	unsigned int _warmIndex;

	void handleMouseClick(int button);
	void handleMouseDown(int button, bool down);

	void _reloadInput();
public:
	int width;
	int height;

	// Vars that the platform is allowed to use in the future
	int commandPort;
	int reserved_d1, reserved_d2;
	float reserved_f1, reserved_f2;

	Options options;

	static bool useAmbientOcclusion;
	//static bool threadInterrupt;

	volatile bool pause;

	LevelRenderer*  levelRenderer;
	GameRenderer*   gameRenderer;
	ParticleEngine* particleEngine;
	SoundEngine*    soundEngine;

	GameMode* gameMode;
#ifndef STANDALONE_SERVER
	Textures* textures;
	ScreenChooser screenChooser;
	Font* font;
#endif
	IRakNetInstance*  raknetInstance;
	NetEventCallback* netCallback;

	/// Who is in the world, host first. The host keeps it up to date itself; a
	/// visitor is sent it (ExtPacket::PlayerList). What the Tab list shows.
	std::vector<std::string> playerNames;

	int lastTime;
	int lastTickTime;
	float ticksSinceLastUpdate;

	User*  user;
	Level* level;

	LocalPlayer*	player;
	IInputHolder*	inputHolder;
	Mob*			cameraTargetPlayer;
#ifndef STANDALONE_SERVER
	Gui gui;
#endif
	CThread* generateLevelThread;
	Screen* screen;
	static int customDebugId;

	static const int CDI_NONE = 0;
	static const int CDI_GRAPHICS = 1;
#ifndef STANDALONE_SERVER
	MouseHandler mouseHandler;
#endif
	bool mouseGrabbed;

	// What updateMusic() last told the page, so it only speaks on a change.
	// The volume starts at an impossible value to force that first push.
	bool _musicOn;
	float _musicVolume;

    PixelCalc pixelCalc;
    PixelCalc pixelCalcUi;

	HitResult hitResult;
	volatile int progressStagePercentage;

	// This field is initialized in main()
	// It sets the base path to where worlds can be written (sdcard on android)
	std::string externalStoragePath;
	std::string externalCacheStoragePath;
protected:
	Timer timer;
    // @note @attn @warn: this is dangerous as fuck!
	volatile bool isGeneratingLevel;
	bool _hasSignaledGeneratingLevelFinished;

	/// How far the chunk pass has got, so it can be resumed next frame.
	int _prepChunkIndex;
	/// The chunk pass's order, nearest the arrival first, and how much of it
	/// is done before the player is let in (_prepCount); the rest is made in
	/// the background once playing (tickBackgroundGeneration).
	std::vector<int> _genOrder;
	int _prepCount;
	unsigned int _genBackground;
	void tickBackgroundGeneration(int maxMs);
	/// Set while the web is driving prepareLevel a slice at a time.
	bool _prepInProgress;
	/// 0 chunk pass, 1 save, 2 prepare -- one frame drawn between each.
	int _prepPhase;
	// Phase 3 (between save and prepare): the Nether, only for a world being created.
	bool _prepNether;
	Level* _netherPrep;
	int _netherPrepIndex;

	LevelStorageSource* storageSource;
	bool _running;
	bool _powerVr;

private:
	volatile int progressStageStatusId;
	static const char* progressMessages[];

	int missTime;
	int ticks;
	bool screenMutex;
	bool hasScheduledScreen;
	Screen* scheduledScreen;

	int _licenseId;
	bool _supportsNonTouchscreen;

	bool _isCreativeMode;
	//int _respawnPlayerTicks;
	Player* _pendingRemovePlayer; // @attn @todo @fix: remove this shait and fix the respawn behaviour

	PerfRenderer* _perfRenderer;
	CommandServer* _commandServer;
};

#endif /*NET_MINECRAFT_CLIENT__Minecraft_H__*/
