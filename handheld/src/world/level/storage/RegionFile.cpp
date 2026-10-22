#include "RegionFile.h"
#include "../../../platform/log.h"

const int SECTOR_BYTES = 4096;
const int SECTOR_INTS = SECTOR_BYTES / 4;
const int SECTOR_COLS = 32;

static void logAssert(int actual, int expected) {
	if (actual != expected) {
		LOGI("ERROR: I/O operation failed (%d vs %d)\n", actual, expected);
	}
}

typedef std::map<std::string, RegionFile*> OpenRegionFiles;
static OpenRegionFiles& openRegionFiles() { static OpenRegionFiles m; return m; }

RegionFile* RegionFile::acquire(const std::string& basePath, const char* fileName)
{
	const std::string key = basePath + "/" + fileName;
	OpenRegionFiles& open = openRegionFiles();
	OpenRegionFiles::iterator it = open.find(key);
	if (it != open.end()) {
		++it->second->refs;
		return it->second;
	}
	RegionFile* f = new RegionFile(basePath, fileName);
	if (!f->open()) {
		delete f;
		return NULL;
	}
	f->refs = 1;
	open[key] = f;
	return f;
}

void RegionFile::release(RegionFile* f)
{
	if (!f || --f->refs > 0) return;
	openRegionFiles().erase(f->filename);
	delete f;
}

RegionFile::RegionFile(const std::string& basePath, const char* fileName)
:	file(NULL),
	refs(0)
{
	filename = basePath;
	filename += "/";
	filename += fileName;

	offsets = new int[SECTOR_INTS];

	emptyChunk = new int[SECTOR_INTS];
	memset(emptyChunk, 0, SECTOR_INTS * sizeof(int));
}

RegionFile::~RegionFile()
{
	close();
	delete [] offsets;
	delete [] emptyChunk;
}

bool RegionFile::open()
{
	close();

	memset(offsets, 0, SECTOR_INTS * sizeof(int));

	// attempt to open file
	file = fopen(filename.c_str(), "r+b");
	if (file)
	{
		// read offset table
		logAssert(fread(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);

		// An entry pointing outside the file is damage (two writers fought over
		// it once, see acquire): reading it gives garbage, and writing a chunk
		// back "in place" there grew the in-memory file to wherever the number
		// pointed -- hundreds of MB -- and the tab ran out of memory. Such
		// entries are dropped (the chunk is made again) and the header fixed.
		fseek(file, 0, SEEK_END);
		const long fileSectors = ftell(file) / SECTOR_BYTES;
		bool repaired = false;
		for (int i = 0; i < SECTOR_INTS; i++) {
			const int offset = offsets[i];
			if (!offset) continue;
			const long base = (unsigned int) offset >> 8;
			const long count = offset & 0xff;
			if (base < 1 || count < 1 || base + count > fileSectors) {
				offsets[i] = 0;
				repaired = true;
			}
		}
		if (repaired) {
			LOGI("RegionFile: dropped damaged entries in %s\n", filename.c_str());
			fseek(file, 0, SEEK_SET);
			logAssert(fwrite(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);
		}

		// mark initial sector as blocked
		sectorFree[0] = false;

		// setup blocked sectors
		for (int sector = 0; sector < SECTOR_INTS; sector++)
		{
			int offset = offsets[sector];
			if (offset)
			{
				int base = offset >> 8;
				int count = offset & 0xff;

				for (int i = 0; i < count; i++)
				{
					sectorFree[base + i] = false;
				}
			}
		}
	}
	else
	{
		// new world
		file = fopen(filename.c_str(), "w+b");
		if (!file)
		{
			LOGI("Failed to create chunk file %s\n", filename.c_str());
			return false;
		}

		// write sector header (all zeroes)
		logAssert(fwrite(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);

		// mark initial sector as blocked
		sectorFree[0] = false;
	}

	return file != NULL;
}

void RegionFile::close()
{
	if (file)
	{
		fclose(file);
		file = NULL;
	}
}

/// Drops a damaged chunk's entry, so its next save takes fresh sectors
/// instead of writing over whatever another chunk keeps there.
void RegionFile::forget(int x, int z)
{
	offsets[x + z * SECTOR_COLS] = 0;
	fseek(file, (x + z * SECTOR_COLS) * sizeof(int), SEEK_SET);
	fwrite(&offsets[x + z * SECTOR_COLS], sizeof(int), 1, file);
}

bool RegionFile::readChunk(int x, int z, RakNet::BitStream** destChunkData)
{
	if (x < 0 || z < 0 || x >= SECTOR_COLS || z >= SECTOR_COLS) return false;
	int offset = offsets[x + z * SECTOR_COLS];

	if (offset == 0)
	{
		// not written to file yet
		return false;
	}

	int sectorNum = offset >> 8;

	fseek(file, sectorNum * SECTOR_BYTES, SEEK_SET);
	int length = 0;
	// A chunk whose length cannot be what was written -- past the sectors it
	// owns, or nothing there at all (a header saved without its data, or a
	// file two writers fought over) -- is treated as never saved, so it is
	// made again, instead of being allocated at that size and aborting.
	if (fread(&length, sizeof(int), 1, file) != 1
			|| length <= (int) sizeof(int)
			|| length > (offset & 0xff) * SECTOR_BYTES) {
		LOGI("RegionFile: chunk %d,%d unreadable (length %d), regenerating\n", x, z, length);
		forget(x, z);
		return false;
	}
	length -= sizeof(int);

	unsigned char* data = new unsigned char[length];
	if ((int) fread(data, 1, length, file) != length) {
		LOGI("RegionFile: chunk %d,%d truncated, regenerating\n", x, z);
		delete [] data;
		forget(x, z);
		return false;
	}
	*destChunkData = new RakNet::BitStream(data, length, false);
	//delete [] data;

	return true;
}

bool RegionFile::writeChunk(int x, int z, RakNet::BitStream& chunkData)
{
	if (x < 0 || z < 0 || x >= SECTOR_COLS || z >= SECTOR_COLS) return false;
	int size = chunkData.GetNumberOfBytesUsed() + sizeof(int);

	int offset = offsets[x + z * SECTOR_COLS];
	int sectorNum = offset >> 8;
	int sectorCount = offset & 0xff;
	int sectorsNeeded = (size / SECTOR_BYTES) + 1;

	if (sectorsNeeded > 256)
	{
		LOGI("ERROR: Chunk is too big to be saved to file\n");
		return false;
	}

	if (sectorNum != 0 && sectorCount == sectorsNeeded) {
		// the sector can be written on top of its old data
		write(sectorNum, chunkData);
	}
	else
	{
		// we need a new location

		// mark old sectors as free
		for (int i = 0; i < sectorCount; i++)
		{
			sectorFree[sectorNum + i] = true;
		}

		// find an available slot with enough run length
		int slot = 0;
		int runLength = 0;
		bool extendFile = false;
		while (runLength < sectorsNeeded)
		{
			if (sectorFree.find(slot + runLength) == sectorFree.end())
			{
				extendFile = true;
				break;
			}
			if (sectorFree[slot + runLength] == true)
			{
				runLength++;
			}
			else
			{
				slot = slot + runLength + 1;
				runLength = 0;
			}
		}

		if (extendFile)
		{
			fseek(file, 0, SEEK_END);
			for (int i = 0; i < (sectorsNeeded - runLength); i++)
			{
				fwrite(emptyChunk, sizeof(int), SECTOR_INTS, file);
				sectorFree[slot + i] = true;
			}
		}
		offsets[x + z * SECTOR_COLS] = (slot << 8) | sectorsNeeded;
		// mark slots as taken
		for (int i = 0; i < sectorsNeeded; i++)
		{
			sectorFree[slot + i] = false;
		}

		// write!
		write(slot, chunkData);

		// write sector data
		fseek(file, (x + z * SECTOR_COLS) * sizeof(int), SEEK_SET);
		fwrite(&offsets[x + z * SECTOR_COLS], sizeof(int), 1, file);
	}


	return true;
}

bool RegionFile::write(int sector, RakNet::BitStream& chunkData)
{
	fseek(file, sector * SECTOR_BYTES, SEEK_SET);
	//LOGI("writing %d B to sector %d\n", chunkData.GetNumberOfBytesUsed(), sector);
	int size = chunkData.GetNumberOfBytesUsed() + sizeof(int);
	logAssert(fwrite(&size, sizeof(int), 1, file), 1);
	logAssert(fwrite(chunkData.GetData(), 1, chunkData.GetNumberOfBytesUsed(), file), chunkData.GetNumberOfBytesUsed());

	return true;
}
