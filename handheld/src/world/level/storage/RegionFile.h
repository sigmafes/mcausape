#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__

#include <map>
#include <string>
#include "../../../raknet/BitStream.h"

typedef std::map<int, bool> FreeSectorMap;

class RegionFile
{
public:
	// fileName is the chunk file inside basePath. Each dimension gets its own,
	// so the Nether's chunks never share a file with the Overworld's.
	RegionFile(const std::string& basePath, const char* fileName = "chunks.dat");
	virtual ~RegionFile();

	/** The one RegionFile for a file, opened, shared by every storage that asks
	    (NULL if it cannot be opened). A host runs two Levels of one world, each
	    with its own storage; two RegionFiles on one file each kept their own
	    table of used sectors, handed out the same ones and overwrote each
	    other's header -- a chunk then read back with a garbage length. Give it
	    back with release(), never delete. */
	static RegionFile* acquire(const std::string& basePath, const char* fileName);
	static void release(RegionFile* file);

	bool open();
	bool readChunk(int x, int z, RakNet::BitStream** destChunkData);
	bool writeChunk(int x, int z, RakNet::BitStream& chunkData);
private:
	bool write(int sector, RakNet::BitStream& chunkData);
	void close();
	void forget(int x, int z);

	FILE* file;
	std::string	filename;
	int* offsets;
	int* emptyChunk;
	FreeSectorMap sectorFree;
	int refs;
};


#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__*/
