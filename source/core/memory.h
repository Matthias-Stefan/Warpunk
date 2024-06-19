#pragma once

#include <stdlib.h>

#include "source/core/types.h"
#include "source/core/util.h"

struct memory_block
{
    template <typename T> T* PushSize(u64 Size)
    {
        Assert((Used + Size) < Allocated);
        
        u8* Result = (u8 *)Data + Used;
        Used += Size;
        
        return (T *)Result;
    }
    
    void PopSize(u64 Size)
    {
        Used -= Size;
    }
    
    template <typename T> T* PushStruct()
    {
        return PushSize<T>(sizeof(T));
    }
    
    template <typename T> void PopStruct()
    {
        PopSize(sizeof(T));
    }
    
    template <typename T> T* PushArray(u64 Size)
    {
        return PushSize<T>(sizeof(T) * Size);
    }
    
    template <typename T> void PopArray(u64 Size)
    {
        PopSize(sizeof(T) * Size);
    }
    
    u64 Allocated;
    u64 Used;
    void *Data = nullptr;
    char *Name;
};

template<typename T>
struct temporary_memory_block 
{
    temporary_memory_block<T>(u64 Size)
    {
        Allocated = Size * sizeof(T);
        Used = 0;
        Data = (T *)malloc(Allocated);
        Assert(Data != nullptr);
    }
    
    ~temporary_memory_block()
    {
        if (Data)
        {
            free(Data);
            Allocated = 0;
            Used = 0;
            Data = nullptr;
        }
    }
    
    u64 Allocated;
    u64 Used;
    T *Data = nullptr;
};

