#ifndef NET_MINECRAFT_NETWORK_PACKET__ChunkDataPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ChunkDataPacket_H__

#include "../Packet.h"
#include "../../world/level/chunk/LevelChunk.h"
#include "../../world/level/Level.h"
#include "../../world/level/dimension/Dimension.h"

class ChunkDataPacket : public Packet
{
public:

	int x, z;
	RakNet::BitStream chunkData;
	LevelChunk* chunk;

	ChunkDataPacket()
	{
	}

	ChunkDataPacket(int x, int z, LevelChunk* chunk)
	:	x(x),
		z(z),
		chunk(chunk)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CHUNKDATA));

		bitStream->Write(x);
		bitStream->Write(z);

		unsigned char* blockIds = chunk->getBlockData();
		DataLayer& blockData = chunk->data;

		const int setSize = LEVEL_HEIGHT / 8;
		const int setShift = 4; // power of LEVEL_HEIGHT / 8

		chunkData.Reset();

		// Mode 1, the Nether: the whole chunk, run-length coded. A visitor works
		// the rest of a chunk out for itself and sends none of it back, which is
		// right where the world grows the same everywhere; the Nether's
		// population (fortresses, lava, fire) did not, and a visitor saw a
		// different one from the host's. Netherrack is mostly runs, so this is
		// a few KB a chunk.
		const bool full = chunk->level != NULL && chunk->level->dimension != NULL && chunk->level->dimension->hasCeiling;
		chunkData.Write((unsigned char) (full ? 1 : 0));
		if (full) {
			for (int i = 0; i < CHUNK_COLUMNS; i++) {
				const int colDataPosition = (i % CHUNK_WIDTH) << 11 | (i / CHUNK_WIDTH) << 7;
				int y = 0;
				while (y < LEVEL_HEIGHT) {
					const unsigned char id = blockIds[colDataPosition + y];
					int run = 1;
					while (y + run < LEVEL_HEIGHT && run < 255 && blockIds[colDataPosition + y + run] == id) ++run;
					chunkData.Write((unsigned char) run);
					chunkData.Write(id);
					y += run;
				}
				bool anyData = false;
				const unsigned char* nibbles = &blockData.data[colDataPosition >> 1];
				for (int b = 0; b < LEVEL_HEIGHT / 2; ++b) if (nibbles[b] != 0) { anyData = true; break; }
				chunkData.Write((unsigned char) (anyData ? 1 : 0));
				if (anyData)
					chunkData.Write((const char*) nibbles, LEVEL_HEIGHT / 2);
			}
			bitStream->Write(chunkData);
			return;
		}

		for (int i = 0; i < CHUNK_COLUMNS; i++)
		{
			unsigned char updateBits = chunk->updateMap[i];
			chunkData.Write(updateBits);

			if (updateBits > 0)
			{
				int colDataPosition = (i % CHUNK_WIDTH) << 11 | (i / CHUNK_WIDTH) << 7;

				for (int set = 0; set < 8; set++)
				{
					if ((updateBits & (1 << set)) != 0) 
					{
						chunkData.Write((const char*)(&blockIds[colDataPosition + (set << setShift)]), setSize);
						// block data is only 4 bits per block
						chunkData.Write((const char*)(&blockData.data[(colDataPosition + (set << setShift)) >> 1]), setSize >> 1);
					}
				}
			}
		}

		bitStream->Write(chunkData);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(x);
		bitStream->Read(z);
		bitStream->Read(chunkData);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (ChunkDataPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ChunkDataPacket_H__*/
