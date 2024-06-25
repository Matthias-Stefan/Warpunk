#pragma once

#include <cstdint>
#include <intrin.h>

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using b32 = s32;
using index = size_t;

#define internal static
#define persist static
#define global static

#define KB(value) (1024LL * value)
#define MB(value) (1024LL * KB(value))
#define GB(value) (1024LL * MB(value))
#define TB(value) (1024LL * GB(value))

#define PI_2 1.57079632679489661923
#define PI   3.14159265358979323846
#define TAU  2*PI

#define MAX_U8 255
#define MAX_U16 65535
#define MAX_U32 ((u32)-1)
#define MAX_U64 ((u64)-1)

inline u32 GetThreadID(void)
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);

    return(ThreadID);
}
