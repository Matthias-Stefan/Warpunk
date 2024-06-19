#pragma once

#include "source/core/memory.h"
#include "source/core/types.h"

#include "source/platform/mouse.h"
#include "source/platform/keyboard.h"
#include "source/platform/thread_pool.h"

typedef struct platform_file_handle
{
    unsigned char* Content;
    size_t Size;
} platform_file_handle;

template <typename T>
__forceinline T ExtractData(u8 *&Raw)
{
    T Data = *((T *)Raw);
    Raw += sizeof(T);
    return Data;
}

enum platform_error_type
{
    PlatformError_Fatal,
    PlatformError_Nonfatal,
};

/// Error services

#define PLATFORM_ERROR_MESSAGE(name) void name(platform_error_type Type, char *Message)

/// IO services 

#define PLATFORM_READ_DATA_FROM_FILE(name) void name(char *Filename, platform_file_handle *Handle)
typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

/// Multi-threading services

#define PLATFORM_CREATE_WORK_QUEUE(name) void name(work_queue *Queue, u32 ThreadCount, work_thread *WorkThreads)
typedef PLATFORM_CREATE_WORK_QUEUE(platform_create_work_queue);

#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(work_queue *Queue, void *Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

#define PLATFORM_ADD_WORK_QUEUE_ENTRY(name) void name(work_queue *Queue, work_queue_callback *Callback, void *Data)
typedef PLATFORM_ADD_WORK_QUEUE_ENTRY(platform_add_work_queue_entry);

#define PLATFORM_DO_NEXT_WORK_QUEUE_ENTRY(name) bool name(work_queue *Queue)
typedef PLATFORM_DO_NEXT_WORK_QUEUE_ENTRY(platform_do_next_work_queue_entry);

#define PLATFORM_COMPLETE_ALL_WORK(name) void name(work_queue *Queue)
typedef PLATFORM_COMPLETE_ALL_WORK(platform_complete_all_work);

/// Memory allocation services

#define PLATFORM_ALLOCATE_MEMORY(name) memory_block* name(u64 Size, char *Name)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(memory_block *Memory);
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

/// Audio services

#define PLATFORM_INITIALIZE_AUDIO(name)
typedef PLATFORM_INITIALIZE_AUDIO(platform_initialize_audio);

#define PLATFORM_PREPARE_AUDIO(name)
typedef PLATFORM_PREPARE_AUDIO(platform_prepare_audio);

#define PLATFORM_PLAY_AUDIO(name)
typedef PLATFORM_PLAY_AUDIO(platform_play_audio);

#define PLATFORM_STOP_AUDIO(name)
typedef PLATFORM_STOP_AUDIO(platform_stop_audio);

#define PLATFORM_GET_AUDIO_SAMPLE(name)
typedef PLATFORM_GET_AUDIO_SAMPLE(platform_get_audio_sample);

/// glb/glTF services

struct glb;
#define PLATFORM_LOAD_GLB(name) glb name(char *Filename)
typedef PLATFORM_LOAD_GLB(platform_load_gltf);

#define PLATFORM_UNLOAD_GLB(name) void name(glb *GLB)
typedef PLATFORM_UNLOAD_GLB(platform_unload_gltf);

typedef struct platform_api
{
    platform_add_work_queue_entry *AddWorkQueueEntry;
    platform_complete_all_work *CompleteAllWork;

    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;

    platform_load_gltf *LoadglTF;
    platform_unload_gltf *UnloadglTF;
} platform_api;
extern platform_api Platform;

typedef struct game_input
{
    float dtForFrame;

    keyboard Keyboard;
    mouse Mouse;

    int RenderWidth;
    int RenderHeight;

    bool QuitRequested;
} game_input;

struct camera;
typedef struct game_debug_info
{
    keyboard* Keyboard;
    mouse* Mouse;
    camera* Camera;
} game_debug_info;

typedef struct game_offscreen_buffer
{
    void* Memory;
    int Width;
    int Height;
    int Pitch;
} game_offscreen_buffer;

typedef struct game_context
{
    struct game_state *GameState;

    platform_api *PlatformAPI;
    memory_block *Memory;

    work_queue *HighPriorityQueue;
    work_queue *LowPriorityQueue;
} game_context;

/// Game services

#define GAME_INITIALIZE(name) void name(game_context *GameContext, game_input *Input, game_debug_info *DebugInfo)
typedef GAME_INITIALIZE(game_initialize);

#define GAME_UPDATE_AND_RENDER(name) void name(game_context *GameContext, game_input *Input, game_debug_info *DebugInfo)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);