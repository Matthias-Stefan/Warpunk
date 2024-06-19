#pragma once

#include "immintrin.h"

#include "source/core/types.h"

inline f32
ToRadian(f32 Degree)
{
    f32 Result = Degree * PI / 180;
    return Result;
}

inline f32
ToDegree(f32 Radian)
{
    f32 Result = Radian * 180 / PI;
    return Result;
}

inline f32
Square(f32 Float32)
{
    f32 Result = Float32 * Float32;
    return Result;
}

inline f32
Abs(f32 Float32)
{
    f32 Result = Float32 > 0.0f ? Float32 : (-Float32);
    
    return Result;
}

inline s32 
RoundF32ToI32(f32 Float32)
{
    return (s32)(Float32 + 0.5f);
}

template<typename T>
inline T
Clamp(T Value, T Min, T Max)
{
    if (Value < Min) 
    {
        return Min;
    } 
    else if (Value > Max) 
    {
        return Max;
    } 
    else 
    {
        return Value;
    }
}

inline f32
SquareRoot(f32 Float32)
{
    f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(Float32)));
    return Result;
}

inline f32
Min(f32 A, f32 B)
{
    f32 Result = _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(A), _mm_set_ss(B)));
    return Result;
}

inline f32
Max(f32 A, f32 B)
{
    f32 Result = _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(A), _mm_set_ss(B)));
    return Result;
}

inline f32
Sine(f32 Float32)
{
    f32 Result = _mm_cvtss_f32(_mm_sin_ps(_mm_set_ss(Float32)));
    return Result;
}

inline f32
Cosine(f32 Float32)
{
    f32 Result = _mm_cvtss_f32(_mm_cos_ps(_mm_set_ss(Float32)));
    return Result;
}

inline f32
ArcCosine(f32 Float32)
{
    f32 Result = _mm_cvtss_f32(_mm_acos_ps(_mm_set_ss(Float32)));
    return Result;
}

inline f32
ArcTangent2(f32 A, f32 B) 
{
    f32 Result = _mm_cvtss_f32(_mm_atan2_ps(_mm_set_ss(A), _mm_set_ss(B)));
    return Result;
}

inline f32
Pow(f32 Base, f32 Exponent)
{
    f32 Result = _mm_cvtss_f32(_mm_pow_ps(_mm_set_ss(Base), _mm_set_ss(Exponent)));
    return Result;
}
