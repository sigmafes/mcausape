#ifndef NET_MINECRAFT_WORLD_LEVEL__LevelSettings_H__
#define NET_MINECRAFT_WORLD_LEVEL__LevelSettings_H__

//package net.minecraft.world.level;

#include <string>
#include "../Difficulty.h"

namespace GameType {
	const int Undefined = -1;
	const int Survival = 0;
	const int Creative = 1;

	const int Default = Creative;
}

class LevelSettings
{
public:
    // Difficulty, cheats and debug are chosen on the Create New World screen and
    // travel with the world; the defaults are what a world made before they existed
    // behaves like.
    LevelSettings(long seed, int gameType, int difficulty = Difficulty::NORMAL, bool cheats = false, bool debug = false)
    :   seed(seed),
        gameType(gameType),
        difficulty(difficulty),
        cheats(cheats),
        debug(debug)
    {
    }
	static LevelSettings None() {
		return LevelSettings(-1,-1);
	}

    long getSeed() const {
        return seed;
    }

    int getGameType() const {
        return gameType;
    }

    int getDifficulty() const { return difficulty; }
    bool getCheats() const { return cheats; }
    bool getDebug() const { return debug; }

	//
	// Those two should actually not be here
	// @todo: Move out when we add LevelSettings.cpp :p
	//
	static int validateGameType(int gameType) {
        switch (gameType) {
		case GameType::Creative:
		case GameType::Survival:
            return gameType;
        }
        return GameType::Default;
    }

	static std::string gameTypeToString(int gameType) {
		if (gameType == GameType::Survival) return "Survival";
		if (gameType == GameType::Creative) return "Creative";
		return "Undefined";
	}

private:
    const long seed;
    const int gameType;
    const int difficulty;
    const bool cheats;
    const bool debug;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL__LevelSettings_H__*/
