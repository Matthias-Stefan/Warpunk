#pragma once

#include <cassert>
#include <immintrin.h>

#define Assert(Expression) if(!(Expression)) {*(volatile int *)0 = 0;}
#define ArraySize(Array) (sizeof(Array) / sizeof(Array[0]))

template <typename T>
inline T SwapEndian(T Data)
{
    T Result = 0;
    u8 *ResultBytes = (u8 *)(&Result);
    u8 *DataBytes = (u8 *)(&Data);
    
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        Result |= (T)(DataBytes[sizeof(T) - i - 1] << (8 * i));
    }
    
    return Result;
}

#define CopyStruct(Source, Destination, Size) CopyArray_(Source, Destination, Size)
#define CopyArray(Source, Destination, Size) CopyArray_(Source, Destination, Size)

internal void CopyArray_(void *Source, void* Destination, size_t Size)
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
