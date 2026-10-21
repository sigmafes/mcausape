#ifndef NET_MINECRAFT_CLIENT_MULTIPLAYER__ClientChunkSource_H__
#define NET_MINECRAFT_CLIENT_MULTIPLAYER__ClientChunkSource_H__

#include "../../world/level/chunk/ChunkSource.h"
#include "../../world/level/chunk/LevelChunk.h"
#include "../../world/level/chunk/EmptyLevelChunk.h"
#include "../../world/level/LevelConstants.h"
#include <cstring>

/** A visitor's chunks: blank until the host sends them (ChunkDataPacket), never
    generated and never saved.

    0.6.1 had every visitor generate the whole world from the seed and sent only
    what had changed since. That only works if both tabs grow exactly the same
    world, which the Nether does not, and if the record of what changed is
    complete, which it was not -- and generating it took a visitor longer than
    receiving it. The host's chunks are the world now; this only holds them. */
class ClientChunkSource: public ChunkSource
{
public:
	ClientChunkSource(Level* level)
	:	level(level)
	{
		memset(chunks, 0, sizeof(chunks));
		empty = new EmptyLevelChunk(level, NULL, 0, 0);
	}

	~ClientChunkSource()
	{
		for (int i = 0; i < CHUNK_CACHE_WIDTH * CHUNK_CACHE_WIDTH; ++i) {
			if (chunks[i]) {
				chunks[i]->deleteBlockData();
				delete chunks[i];
			}
		}
		delete empty;
	}

	bool hasChunk(int x, int z) { return true; }

	LevelChunk* getChunk(int x, int z)
	{
		if (x < 0 || z < 0 || x >= CHUNK_CACHE_WIDTH || z >= CHUNK_CACHE_WIDTH) return empty;
		LevelChunk*& c = chunks[x + z * CHUNK_CACHE_WIDTH];
		if (!c) {
			unsigned char* blocks = new unsigned char[CHUNK_BLOCK_COUNT];
			memset(blocks, 0, CHUNK_BLOCK_COUNT);
			c = new LevelChunk(level, blocks, x, z);
			// Dark until it arrives: no sky light is better than a lit hole.
			c->terrainPopulated = true;
			c->dontSave = true;
		}
		return c;
	}

	LevelChunk* create(int x, int z) { return getChunk(x, z); }
	void postProcess(ChunkSource* parent, int x, int z) {}
	bool tick() { return false; }
	bool shouldSave() { return false; }
	Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) { return Biome::MobList(); }
	std::string gatherStats() { return "ClientChunkSource"; }

private:
	Level* level;
	LevelChunk* chunks[CHUNK_CACHE_WIDTH * CHUNK_CACHE_WIDTH];
	LevelChunk* empty;
};

#endif /*NET_MINECRAFT_CLIENT_MULTIPLAYER__ClientChunkSource_H__*/
