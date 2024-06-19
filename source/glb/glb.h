#pragma once

#include "source/core/types.h"
#include "source/platform/platform.h"

struct glb;
struct chunk;

struct gltf;
struct extension;
struct extras;
struct asset;

#define GLB_MAGIC 0x46546C67
#define GLB_CHUNK_TYPE_JSON 0x4E4F534A
#define GLB_CHUNK_TYPE_BIN 0x004E4942

struct glb
{
	u32 Magic;
	u32 Version;
	u32 Length;
	chunk Chunk0;
	chunk Chunk1;
};

struct chunk
{
	u32 ChunkLength;
	u32 ChunkType;
	u8 *ChunkData;
};


struct gltf
{
	asset Asset;
};

struct extension
{

};

struct extras
{

};

struct asset
{
	char *Copyright;
	char *Generator;
	char *Version;
	char *MinVersion;
	extension Extension;
	extras Extras;
};

struct memory_block;
struct work_queue;
glb LoadGLB(platform_file_handle *File, memory_block *Memory, work_queue *Queue);
