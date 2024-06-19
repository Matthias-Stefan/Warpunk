#if false
#pragma once

#include <math.h>
#include <immintrin.h>

#include "source/core/memory.h"
#include "source/core/util.h"

#include "source/platform/keyboard.h"
#include "source/platform/mouse.h"

//////////////////////////////////////////////////////////////////////

namespace wp
{
    //////////////////////////////////////////////////////////////////////
    
    //////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////


enum game_keyboard_buttons
{
    PlatformKeyboardButton_A,
    PlatformKeyboardButton_B,
    PlatformKeyboardButton_C,
    PlatformKeyboardButton_D,
    PlatformKeyboardButton_E,
    PlatformKeyboardButton_F,
    PlatformKeyboardButton_G,
    PlatformKeyboardButton_H,
    PlatformKeyboardButton_I,
    PlatformKeyboardButton_J,
    PlatformKeyboardButton_K,
    PlatformKeyboardButton_L,
    PlatformKeyboardButton_M,
    PlatformKeyboardButton_N, 
    PlatformKeyboardButton_O,
    PlatformKeyboardButton_P,
    PlatformKeyboardButton_Q,
    PlatformKeyboardButton_R,
    PlatformKeyboardButton_S,
    PlatformKeyboardButton_T,
    PlatformKeyboardButton_U,
    PlatformKeyboardButton_V,
    PlatformKeyboardButton_W,
    PlatformKeyboardButton_X,
    PlatformKeyboardButton_Y,
    PlatformKeyboardButton_Z,
    
    PlatformKeyboardButton_ArrowLeft,
    PlatformKeyboardButton_ArrowUp,
    PlatformKeyboardButton_ArrowRight,
    PlatformKeyboardButton_ArrowDown,
    
    PlatformKeyboardButton_Alt,
    
    PlatformKeyboardButton_Count,
};

typedef struct button_state
{
    bool IsDown;
    bool WasDown;
} button_state;

typedef struct game_keyboard_input
{
    button_state State[PlatformKeyboardButton_Count];
} game_keyboard_input;

enum game_mouse_buttons
{
    PlatformMouseButton_Left,
    PlatformMouseButton_Middle,
    PlatformMouseButton_Right,
    PlatformMouseButton_Wheel,
    
    PlatformMouseButton_Count,
};

typedef struct mouse_state
{
    bool IsDown;
    bool WasDown;
    bool IsDragging;
} mouse_state;

#define PRESSED(State) (State.IsDown || State.WasDown)
#define PRESSED_ONCE(State) (State.IsDown && !(State.WasDown))

typedef struct platform_client_pos
{
    int x;
    int y;
} platform_client_pos;

typedef struct game_mouse_input
{
    bool IsMoving;
    mouse_state State[PlatformMouseButton_Count];
    platform_client_pos P;
    platform_client_pos DownP;
    platform_client_pos dtDraggingInitial;
    platform_client_pos dtDraggingRecent;
} game_mouse_input;

struct camera;

typedef struct game_input
{
    float dtForFrame;
    
    wp::keyboard Keyboard;
    wp::mouse Mouse;
    
    int RenderWidth;
    int RenderHeight;
    
    bool QuitRequested;
} game_input;

typedef struct game_debug_info
{
    game_mouse_input *Mouse; 
    game_keyboard_input *Keyboard;
    camera *Camera;
} game_debug_info;

//
//
//

typedef struct platform_file_handle
{
    unsigned char *Content;
    size_t Size;
} platform_file_handle;

#define PLATFORM_READ_DATA_FROM_FILE(name) void name(char *Filename, platform_file_handle *Handle)
typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

//
// Memory
//

#if false
enum malloc_type
{
    MallocType_Persistent,
    MallocType_Temporary
};

typedef struct memory_block
{
    unsigned long int Size;
    void *Base;
    void *TemporaryBase;
    float Ratio;
    
    unsigned long int PersistentSize;
    unsigned long int PersistentUsed = 0;
    
    unsigned long int TemporarySize;
    unsigned long int TemporaryUsed = 0;
} memory_block;

#define PLATFORM_INITIALIZE_MEMORY(name) void name(memory_block *MemoryBlock, unsigned long int Size, float Ratio)

#define PushSizePersistent(Memory, Size, ...) PushSizePersistent_(Memory, Size, ## __VA_ARGS__)
#define PushStructPersistent(Memory, type, ...) PushSizePersistent_(Memory, sizeof(type), ## __VA_ARGS__)
#define PushArrayPersistent(Memory, type, Size, ...) (type *)(PushSizePersistent_(Memory, sizeof(type)*Size))

#define PopSizePersistent(Memory, Size, ...) PopSizePersistent_(Memory, Size, ## __VA_ARGS__)
#define PopStructPersistent(Memory, type, ...) PopSizePersistent_(Memory, sizeof(type), ## __VA_ARGS__)
#define PopArrayPersistent(Memory, Array, ...) PopSizePersistent_(Memory, sizeof(Array[0])*ArraySize(Array))

#define PushSizeTemporary(Memory, Size, ...) PushSizeTemporary_(Memory, Size, ## __VA_ARGS__)
#define PushStructTemporary(Memory, type, ...) PushSizeTemporary_(Memory, sizeof(type), ## __VA_ARGS__)
#define PushArrayTemporary(Memory, type, Size, ...) (type *)(PushSizeTemporary_(Memory, sizeof(type)*Size))

#define PopSizeTemporary(Memory, Size, ...) PopSizeTemporary_(Memory, Size, ## __VA_ARGS__)
#define PopStructTemporary(Memory, type, ...) PopSizeTemporary_(Memory, sizeof(type), ## __VA_ARGS__)
#define PopArrayTemporary(Memory, Array, ...) PopSizeTemporary_(Memory, sizeof(Array[0])*ArraySize(Array))

#define FlushTemporary(Memory) PopSizeTemporary_(Memory, Memory->TemporaryUsed)

internal void *
PushSizePersistent_(memory_block *Memory, unsigned int Size)
{
    void *Result = (unsigned char *)Memory->Base + Memory->PersistentUsed;
    Memory->PersistentUsed += Size;
    
    return Result;
}

internal void
PopSizePersistent_(memory_block *Memory, unsigned int  Size)
{
    Memory->PersistentUsed -= Size;
}

internal void *
PushSizeTemporary_(memory_block *Memory, unsigned int Size)
{
    void *Result = (unsigned char *)Memory->TemporaryBase + Memory->TemporaryUsed;
    Memory->TemporaryUsed += Size;
    
    return Result;
}

internal void
PopSizeTemporary_(memory_block *Memory, unsigned int  Size)
{
    Memory->TemporaryUsed -= Size;
}
#endif

#define CopyStruct(Source, Destination, Size) CopyArray_(Source, Destination, Size)
#define CopyArray(Source, Destination, Size) CopyArray_(Source, Destination, Size)

internal void
CopyArray_(void *Source, void* Destination, size_t Size)
{
    __m128i xmm0, xmm1, xmm2, xmm3;
    __m128i *Source128 = (__m128i *)Source;
    __m128i *Destination128 = (__m128i *)Destination;
    
    while (Size >= 64)
    {
        xmm0 = _mm_loadu_si128(Source128++);
        xmm1 = _mm_loadu_si128(Source128++);
        xmm2 = _mm_loadu_si128(Source128++);
        xmm3 = _mm_loadu_si128(Source128++);
        
        _mm_storeu_si128(Destination128++, xmm0);
        _mm_storeu_si128(Destination128++, xmm1);
        _mm_storeu_si128(Destination128++, xmm2);
        _mm_storeu_si128(Destination128++, xmm3);
        
        Size -= 64;
    }
    
    unsigned char *Destination8 = (unsigned char *)Destination128;
    unsigned char *Source8 = (unsigned char *)Source128;
    
    while (Size--)
    {
        *Destination8++ = *Source8++;
    }
}

//
//
//

typedef struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
} game_offscreen_buffer;

//
//
//

enum platform_error_type
{
    PlatformError_Fatal,
    PlatformError_Nonfatal,
};

#define PLATFORM_ERROR_MESSAGE(name) void name(platform_error_type Type, char *Message)
typedef PLATFORM_ERROR_MESSAGE(platform_error_message);

#define GAME_INITIALIZE(name) void name(memory_block *Memory, \
game_input *Input, \
game_debug_info *DebugInfo)
typedef GAME_INITIALIZE(game_initialize);

#define GAME_UPDATE_AND_RENDER(name) void name(memory_block *Memory, \
game_input *Input, \
game_debug_info *DebugInfo)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
#endif