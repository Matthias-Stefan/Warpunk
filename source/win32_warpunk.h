#pragma once

#include "source/core/types.h"
#include "source/platform/platform.h"

struct glb;

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    s32 Width;
    s32 Height;
    s32 Pitch;
    s32 BytesPerPixel;
};

struct win32_window_dimension
{
    s32 Width;
    s32 Height;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_game_code
{
    b32 IsValid;
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;
    
    game_initialize *Initialize;
    game_update_and_render *UpdateAndRender;
};

struct win32_state
{
    char EXEFilename[WIN32_STATE_FILE_NAME_COUNT];
    char *OnePastLastEXEFilenameSlash;
    
    HWND DefaultWindowHandle;
};

PLATFORM_ERROR_MESSAGE(Win32ErrorMessage);

internal PLATFORM_CREATE_WORK_QUEUE(Win32CreateWorkQueue);
internal PLATFORM_WORK_QUEUE_CALLBACK(Win32WorkQueueCallback);
internal PLATFORM_ADD_WORK_QUEUE_ENTRY(Win32AddWorkQueueEntry);
internal PLATFORM_DO_NEXT_WORK_QUEUE_ENTRY(Win32DoNextWorkQueueEntry);
internal PLATFORM_COMPLETE_ALL_WORK(Win32CompleteAllWork);

internal PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory);
internal PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory);
internal PLATFORM_ENLARGE_MEMORY(Win32EnlargeMemory);

internal PLATFORM_LOAD_ASSET(Win32LoadAsset);
internal PLATFORM_UNLOAD_ASSET(Win32UnloadAsset);
