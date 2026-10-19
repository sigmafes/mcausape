#ifndef _MINECRAFT_WORLD_LEVELCONSTANTS_H_
#define _MINECRAFT_WORLD_LEVELCONSTANTS_H_


const int LEVEL_HEIGHT = 128;
// World width in chunks (the world is CHUNK_CACHE_WIDTH x CHUNK_CACHE_WIDTH).
// Must be a power of two (ChunkCache masks with width-1) and at most 32: the
// chunk file (RegionFile) holds one 32x32 table of chunk offsets, and a chunk
// outside it would write past the end of that table.
const int CHUNK_CACHE_WIDTH = 32; // in chunks
const int CHUNK_WIDTH = 16; // in blocks
const int CHUNK_DEPTH = 16;
const int LEVEL_WIDTH = CHUNK_CACHE_WIDTH * CHUNK_WIDTH;
const int LEVEL_DEPTH = CHUNK_CACHE_WIDTH * CHUNK_DEPTH;
const int CHUNK_COLUMNS = CHUNK_WIDTH * CHUNK_DEPTH;
const int CHUNK_BLOCK_COUNT = CHUNK_COLUMNS * LEVEL_HEIGHT;

#endif
