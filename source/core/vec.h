#pragma once

#include <cmath>
#include <optional>

#include "source/core/types.h"

template<typename T, size_t S>
struct vec
{
    T E[S];
    
    inline T& operator[](size_t index) 
    {
        return E[index];
    }
};

template<typename T>
struct vec<T, 2>
{
    union
    {
        T E[2];
        struct 
        {
            T x;
            T y;
        };
        struct
        {
            T u;
            T v;
        };
        struct
        {
            T Width;
            T Height;
        };
    };
    
    inline operator vec<T, 3>()
    {
        vec<T, 3> Result = { E[0], E[1], T(0) };
        return Result;
    }
    
    inline operator vec<T, 4>()
    {
        vec<T, 4> Result = { E[0], E[1], T(0), T(0) };
        return Result;
    }
};

template<typename T>
struct vec<T, 3>
{
    union
    {
        T E[3];
        struct 
        {
            T x;
            T y;
            T z;
        };
        struct
        {
            T r;
            T g;
            T b;
        };
    };
    
    inline operator vec<T, 2>()
    {
        vec<T, 2> Result = { E[0], E[1] };
        return Result;
    }
    
    inline operator vec<T, 4>()
    {
        vec<T, 4> Result = { E[0], E[1], E[2], T(0) };
        return Result;
    }
};

template<typename T>
struct vec<T, 4>
{
    union
    {
        T E[4];
        struct 
        {
            T x;
            T y;
            T z;
            T w;
        };
        struct
        {
            T r;
            T g;
            T b;
            T a;
        };
    };
    
    inline operator vec<T, 2>()
    {
        vec<T, 2> Result = { E[0], E[1] };
        return Result;
    }
    
    inline operator vec<T, 3>()
    {
        vec<T, 3> Result = { E[0], E[1], E[2] };
        return Result;
    }
};

template<typename T>
using v2 = vec<T, 2>;

using v2s8 = vec<s8, 2>;
using v2s16 = vec<s16, 2>;
using v2s32 = vec<s32, 2>;
using v2s64 = vec<s64, 2>;

using v2u8 = vec<u8, 2>;
using v2u16 = vec<u16, 2>;
using v2u32 = vec<u32, 2>;
using v2u64 = vec<u64, 2>;

using v2f32 = vec<f32, 2>;
using v2f64 = vec<f64, 2>;

using v2s = vec<s32, 2>; 
using v2u = vec<u32, 2>; 
using v2f = vec<f32, 2>; 
using v2d = vec<f64, 2>; 

template<typename T>
using v3 = vec<T, 3>;

using v3s8 = vec<s8, 3>;
using v3s16 = vec<s16, 3>;
using v3s32 = vec<s32, 3>;
using v3s64 = vec<s64, 3>;

using v3u8 = vec<u8, 3>;
using v3u16 = vec<u16, 3>;
using v3u32 = vec<u32, 3>;
using v3u64 = vec<u64, 3>;

using v3f32 = vec<f32, 3>;
using v3f64 = vec<f64, 3>;

using v3s = vec<s32, 3>; 
using v3u = vec<u32, 3>; 
using v3f = vec<f32, 3>; 
using v3d = vec<f64, 3>; 

template<typename T>
using v4 = vec<T, 4>;

using v4s8 = vec<s8, 4>;
using v4s16 = vec<s16, 4>;
using v4s32 = vec<s32, 4>;
using v4s64 = vec<s64, 4>;

using v4u8 = vec<u8, 4>;
using v4u16 = vec<u16, 4>;
using v4u32 = vec<u32, 4>;
using v4u64 = vec<u64, 4>;

using v4f32 = vec<f32, 4>;
using v4f64 = vec<f64, 4>;

using v4s = vec<s32, 4>; 
using v4u = vec<u32, 4>; 
using v4f = vec<f32, 4>; 
using v4d = vec<f64, 4>; 

template<typename T, size_t S>
inline vec<T, S> 
operator+(vec<T, S> A, vec<T, S> B)  
{
    vec<T, S> Result = {};
    
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        Result.E[Dim] = A.E[Dim] + B.E[Dim];
    }
    
    return Result;
}

template<typename T, size_t S>
inline vec<T, S>& 
operator+=(vec<T, S> &A, T B) 
{
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        A.E[Dim] += B;
    }
    
    return A;
}

template<typename T, size_t S>
inline vec<T, S>&
operator+=(vec<T, S> &A, vec<T, S> B)
{
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        A.E[Dim] += B.E[Dim];
    }
    
    return A;
}

template<typename T, size_t S>
inline vec<T, S> 
operator-(vec<T, S> A, vec<T, S> B) 
{
    vec<T, S> Result = {};
    
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        Result.E[Dim] = A.E[Dim] - B.E[Dim];
    }
    
    return Result;
}

template<typename T, size_t S>
inline vec<T, S>& 
operator-=(vec<T, S> &A, T B) 
{
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        A.E[Dim] -= B;
    }
    
    return *this;
}

template<typename T, size_t S>
inline vec<T, S>&
operator-=(vec<T, S> &A, vec<T, S> B)
{
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        A.E[Dim] -= B.E[Dim];
    }
    
    return A;
}

template<typename T, size_t S>
inline vec<T, S>
operator*(vec<T, S> A, T B)
{
    vec<T, S> Result = {};
    
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        Result.E[Dim] = A.E[Dim] * B;
    }
    
    return Result;
}

template<typename T, size_t S>
inline vec<T, S>&
operator*=(vec<T, S> &A, T B)
{
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        A.E[Dim] *= B;
    }
    
    return A;
}

template<typename T, size_t S>
inline vec<T, S>
operator*(vec<T, S> A, vec<T, S> B)
{
    vec<T, S> Result = {};
    
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        Result.E[Dim] = A.E[Dim] * B.E[Dim];
    }
    
    return Result;
}

template<typename T, size_t S>
inline vec<T, S>
operator/(const vec<T, S> &A, const T &B)
{
    vec<T, S> Result = (1.0f / B) * A; 
    
    return Result;
}

template<typename T, size_t S>
inline vec<T, S>
operator/(const T& B, const vec<T, S> &A)
{
    vec<T, S> Result;
    
    for (size_t Dim = 0; Dim < S; ++Dim) 
    {
        Result.E[Dim] = B / A.E[Dim];
    }
    
    return Result;
}

template<typename T, size_t S>
inline f32 
Magnitude(vec<T, S> &A)
{
    f32 Result = 0.0f;
    
    for (size_t Index = 0; Index < S; ++Index)
    {
        Result += (A.E[Index] * A.E[Index]);
    }
    
    return std::sqrt(Result);
}

template<typename T, size_t S>
inline f32 
MagnitudeSq(vec<T, S> &A)
{
    f32 Result = 0.0f;
    
    for (size_t Index = 0; Index < S; ++Index)
    {
        Result += (A.E[Index] * A.E[Index]);
    }
    
    return Result;
}

template<typename T, size_t S>
inline vec<T, S> 
Normalize(vec<T, S> &A)
{
    vec<T, S> Result;
    
    f32 InverseMagnitude = 1.0f / Magnitude(A); 
    for (size_t Index = 0; Index < S; ++Index)
    {
        Result.E[Index] = A.E[Index] * InverseMagnitude;
    }
    
    return Result;
}

template<typename T>
inline vec<T, 3> 
Cross(vec<T, 3> &A, vec<T, 3> &B)
{
    vec<T, 3> Result;
    
    Result.E[0] = A.E[1] * B.E[2] - A.E[2] * B.E[1];
    Result.E[1] = A.E[2] * B.E[0] - A.E[0] * B.E[2];
    Result.E[2] = A.E[0] * B.E[1] - A.E[1] * B.E[0];
    
    return Result;
}
