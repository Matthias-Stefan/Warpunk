#pragma once

#include "source/core/types.h"
#include "source/core/util.h"
#include "source/platform/asset.h"
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

struct memory_block;

#define PLATFORM_ALLOCATE_MEMORY(name) void name(memory_block* Memory, u64 Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(memory_block *Memory)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

#define PLATFORM_ENLARGE_MEMORY(name) void name(memory_block *Memory, size_t Size)
typedef PLATFORM_ENLARGE_MEMORY(platform_enlarge_memory);

/// Audio services

// TODO(matthias) implemention
#define PLATFORM_INITIALIZE_AUDIO(name) void name()
typedef PLATFORM_INITIALIZE_AUDIO(platform_initialize_audio);

// TODO(matthias) implemention
#define PLATFORM_PREPARE_AUDIO(name) void name()
typedef PLATFORM_PREPARE_AUDIO(platform_prepare_audio);

// TODO(matthias) implemention
#define PLATFORM_PLAY_AUDIO(name) void name()
typedef PLATFORM_PLAY_AUDIO(platform_play_audio);

// TODO(matthias) implemention
#define PLATFORM_STOP_AUDIO(name) void name()
typedef PLATFORM_STOP_AUDIO(platform_stop_audio);

// TODO(matthias) implemention
#define PLATFORM_GET_AUDIO_SAMPLE(name) void name()
typedef PLATFORM_GET_AUDIO_SAMPLE(platform_get_audio_sample);

/// Asset loading services

#define PLATFORM_LOAD_ASSET(name) asset* name(char *Filename, memory_block *Memory)
typedef PLATFORM_LOAD_ASSET(platform_load_asset);

#define PLATFORM_UNLOAD_ASSET(name) void name(asset *Asset)
typedef PLATFORM_UNLOAD_ASSET(platform_unload_asset);

typedef struct platform_api
{
    platform_add_work_queue_entry *AddWorkQueueEntry;
    platform_complete_all_work *CompleteAllWork;

    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;
    platform_enlarge_memory *EnlargeMemory;

    platform_load_asset *LoadAsset;
    platform_unload_asset *UnloadAsset;

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

/// Memory

struct memory_block
{
    u64 Allocated = 0;
    u64 Used = 0;
    void *Data = nullptr;
    char *Name = nullptr;

    template<typename T>
    T *PushSize(u64 Size)
    {
        Assert((Used + Size) <= Allocated);

        u8 *Result = (u8 *)Data + Used;
        Used += Size;

        return (T *)Result;
    }

    void PopSize(u64 Size)
    {
        Used -= Size;
    }

    template<typename T>
    T *PushStruct()
    {
        return PushSize<T>(sizeof(T));
    }

    template<typename T>
    void PopStruct()
    {
        PopSize(sizeof(T));
    }

    template<typename T>
    T *PushArray(u64 Size)
    {
        return PushSize<T>(sizeof(T) * Size);
    }

    template<typename T>
    void PopArray(u64 Size)
    {
        PopSize(sizeof(T) * Size);
    }
};

template<typename T>
struct temporary_memory_block
{
    memory_block Memory;
    T *Data = (T *)Memory.Data;
};

template<typename T>
internal void
StartTemporaryMemory(temporary_memory_block<T> *TemporaryMemory,
                     size_t Size,
                     platform_allocate_memory *AllocationCallback)
{
    AllocationCallback(&TemporaryMemory->Memory, Size * sizeof(T));
    TemporaryMemory->Data = (T *)TemporaryMemory->Memory.Data;
}

template<typename T>
internal void
EndTemporaryMemory(temporary_memory_block<T> *TemporaryMemory,
                   platform_deallocate_memory *DeallocationCallback)
{
    DeallocationCallback(&TemporaryMemory->Memory);
    TemporaryMemory->Data = nullptr;
}

template<typename T>
internal void
EnlargeTemporaryMemory(temporary_memory_block<T> *TemporaryMemory,
                       size_t Size,
                       platform_enlarge_memory *EnlargeCallback)
{
    EnlargeCallback(&TemporaryMemory->Memory, Size * sizeof(T));
    TemporaryMemory->Data = (T *)TemporaryMemory->Memory.Data;
}
