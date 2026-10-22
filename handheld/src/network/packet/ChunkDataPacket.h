#ifndef NET_MINECRAFT_NETWORK_PACKET__ChunkDataPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ChunkDataPacket_H__

#include "../Packet.h"
#include "../../world/level/chunk/LevelChunk.h"
#include <vector>
#include <zlib.h>

/** One whole chunk, from the host to a visitor: block ids, block data, sky
    light and block light, deflated.

    0.6.1 sent only the sections changed since the chunk was generated and left
    the visitor to generate the rest, and the light, itself; see
    ClientChunkSource for why that went. Whole, a chunk is 80 KB; deflated it is
    a few KB, mostly air and stone in runs. */
class ChunkDataPacket : public Packet
{
public:
	// ...and the waterlogged bits (LevelChunk::waterlogged), last.
	static const int RawSize = CHUNK_BLOCK_COUNT + 3 * (CHUNK_BLOCK_COUNT / 2) + LevelChunk::WaterloggedBytes;

	int x, z;
	std::vector<unsigned char> deflated;

	ChunkDataPacket()
	:	x(0), z(0)
	{
	}

	ChunkDataPacket(int x, int z, LevelChunk* chunk)
	:	x(x), z(z)
	{
		std::vector<unsigned char> raw(RawSize);
		const int half = CHUNK_BLOCK_COUNT / 2;
		memcpy(&raw[0], chunk->getBlockData(), CHUNK_BLOCK_COUNT);
		memcpy(&raw[CHUNK_BLOCK_COUNT], chunk->data.data, half);
		memcpy(&raw[CHUNK_BLOCK_COUNT + half], chunk->skyLight.data, half);
		memcpy(&raw[CHUNK_BLOCK_COUNT + 2 * half], chunk->blockLight.data, half);
		memcpy(&raw[CHUNK_BLOCK_COUNT + 3 * half], chunk->waterlogged, LevelChunk::WaterloggedBytes);

		uLongf size = compressBound(RawSize);
		deflated.resize(size);
		if (compress2(&deflated[0], &size, &raw[0], RawSize, 6) != Z_OK)
			size = 0;
		deflated.resize(size);
	}

	/// Unpacks into a chunk of the visitor's. False if it is not one we can read.
	bool apply(LevelChunk* chunk) const
	{
		if (deflated.empty()) return false;
		std::vector<unsigned char> raw(RawSize);
		uLongf size = RawSize;
		if (uncompress(&raw[0], &size, &deflated[0], (uLong) deflated.size()) != Z_OK || size != (uLongf) RawSize)
			return false;
		const int half = CHUNK_BLOCK_COUNT / 2;
		memcpy(chunk->getBlockData(), &raw[0], CHUNK_BLOCK_COUNT);
		memcpy(chunk->data.data, &raw[CHUNK_BLOCK_COUNT], half);
		memcpy(chunk->skyLight.data, &raw[CHUNK_BLOCK_COUNT + half], half);
		memcpy(chunk->blockLight.data, &raw[CHUNK_BLOCK_COUNT + 2 * half], half);
		memcpy(chunk->waterlogged, &raw[CHUNK_BLOCK_COUNT + 3 * half], LevelChunk::WaterloggedBytes);
		return true;
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CHUNKDATA));
		bitStream->Write(x);
		bitStream->Write(z);
		bitStream->Write((int) deflated.size());
		if (!deflated.empty())
			bitStream->Write((const char*) &deflated[0], (unsigned int) deflated.size());
	}

	void read(RakNet::BitStream* bitStream)
	{
		int size = 0;
		bitStream->Read(x);
		bitStream->Read(z);
		bitStream->Read(size);
		deflated.clear();
		// Never more than the chunk itself would be: anything else is not ours.
		if (size <= 0 || size > (int) compressBound(RawSize)) return;
		deflated.resize(size);
		if (!bitStream->Read((char*) &deflated[0], (unsigned int) size))
			deflated.clear();
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (ChunkDataPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ChunkDataPacket_H__*/
