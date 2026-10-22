#ifndef NET_MINECRAFT_SharedConstants_H__
#define NET_MINECRAFT_SharedConstants_H__

#include <string>

enum LevelGeneratorVersion
{
	LGV_ORIGINAL = 0,
};

namespace Common {
	std::string getGameVersionString(const std::string& versionSuffix = "");
}

#ifndef MC_DEPLOY_NUMBER
#define MC_DEPLOY_NUMBER 0
#endif

namespace SharedConstants
{
	// 0.5.0 uses NPv8
	// 0.6.0 uses NPv9
	// mcausape adds ExtPacket, login refusals and the dimension in StartGamePacket
	// on top of it, so it is NPv10: a build from before them is refused at login
	// ("outdated") instead of misreading packets.
	//
	// A deployed web build speaks only to the same deploy: every one may change
	// what goes over the wire, and the site always serves the newest, so an old
	// tab and a new one would misread each other. The deploy number (commits on
	// main, MC_DEPLOY_NUMBER) becomes the protocol, both ways; a local build
	// (0) keeps 14.
    const int NetworkProtocolVersion = MC_DEPLOY_NUMBER > 0 ? 100000 + MC_DEPLOY_NUMBER : 14;
	const int NetworkProtocolLowestSupportedVersion = NetworkProtocolVersion;
	const int GameProtocolVersion = 1;
	const int GameProtocolLowestSupportedVersion = 1;

	const int StorageVersion = 3;

	const int MaxChatLength = 100;
    
	const int TicksPerSecond = 20;

	const int GeneratorVersion = (int)LGV_ORIGINAL;
	//int FULLBRIGHT_LIGHTVALUE = 15 << 20 | 15 << 4;
}

#endif /*NET_MINECRAFT_SharedConstants_H__*/
