#pragma once

#include "source/core/util.h"

template<typename T, size_t C, size_t R>
struct mat
{
    T E[C][R];
};

template<typename T>
using mat4x4 = mat<T, 4, 4>;

template<typename T, size_t C, size_t R>
inline mat<T, C, R> 
operator+(mat<T, C, R> A, mat<T, C, R> B)
{
    Assert(R == R && C == C);
    
    mat<T, C, R> Result;
    for (size_t column = 0; column < C; ++column)
    {
        for (size_t row = 0; row < R; ++row)
        {
            Result.E[column][row] = A.E[column][row] + B.E[column][row];
        }
    }
    
    return Result;
}

template<typename T>
inline v4<T>
operator*(mat4x4<T> A, v4<T> B)
{
    v4<T> Result = Transform(A, B);
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
inline mat4x4<T>
operator*(mat4x4<T> A, mat4x4<T> B)
{
    mat4x4<T> Result = {};
    
    Result.E[0][0] = A.E[0][0] * B.E[0][0] + A.E[0][1] * B.E[1][0] + A.E[0][2] * B.E[2][0] + A.E[0][3] * B.E[3][0];
    Result.E[0][1] = A.E[0][0] * B.E[0][1] + A.E[0][1] * B.E[1][1] + A.E[0][2] * B.E[2][1] + A.E[0][3] * B.E[3][1];
    Result.E[0][2] = A.E[0][0] * B.E[0][2] + A.E[0][1] * B.E[1][2] + A.E[0][2] * B.E[2][2] + A.E[0][3] * B.E[3][2];
    Result.E[0][3] = A.E[0][0] * B.E[0][3] + A.E[0][1] * B.E[1][3] + A.E[0][2] * B.E[2][3] + A.E[0][3] * B.E[3][3];
    
    Result.E[1][0] = A.E[1][0] * B.E[0][0] + A.E[1][1] * B.E[1][0] + A.E[1][2] * B.E[2][0] + A.E[1][3] * B.E[3][0];
    Result.E[1][1] = A.E[1][0] * B.E[0][1] + A.E[1][1] * B.E[1][1] + A.E[1][2] * B.E[2][1] + A.E[1][3] * B.E[3][1];
    Result.E[1][2] = A.E[1][0] * B.E[0][2] + A.E[1][1] * B.E[1][2] + A.E[1][2] * B.E[2][2] + A.E[1][3] * B.E[3][2];
    Result.E[1][3] = A.E[1][0] * B.E[0][3] + A.E[1][1] * B.E[1][3] + A.E[1][2] * B.E[2][3] + A.E[1][3] * B.E[3][3];
    
    Result.E[2][0] = A.E[2][0] * B.E[0][0] + A.E[2][1] * B.E[1][0] + A.E[2][2] * B.E[2][0] + A.E[2][3] * B.E[3][0];
    Result.E[2][1] = A.E[2][0] * B.E[0][1] + A.E[2][1] * B.E[1][1] + A.E[2][2] * B.E[2][1] + A.E[2][3] * B.E[3][1];
    Result.E[2][2] = A.E[2][0] * B.E[0][2] + A.E[2][1] * B.E[1][2] + A.E[2][2] * B.E[2][2] + A.E[2][3] * B.E[3][2];
    Result.E[2][3] = A.E[2][0] * B.E[0][3] + A.E[2][1] * B.E[1][3] + A.E[2][2] * B.E[2][3] + A.E[2][3] * B.E[3][3];
    
    Result.E[3][0] = A.E[3][0] * B.E[0][0] + A.E[3][1] * B.E[1][0] + A.E[3][2] * B.E[2][0] + A.E[3][3] * B.E[3][0];
    Result.E[3][1] = A.E[3][0] * B.E[0][1] + A.E[3][1] * B.E[1][1] + A.E[3][2] * B.E[2][1] + A.E[3][3] * B.E[3][1];
    Result.E[3][2] = A.E[3][0] * B.E[0][2] + A.E[3][1] * B.E[1][2] + A.E[3][2] * B.E[2][2] + A.E[3][3] * B.E[3][2];
    Result.E[3][3] = A.E[3][0] * B.E[0][3] + A.E[3][1] * B.E[1][3] + A.E[3][2] * B.E[2][3] + A.E[3][3] * B.E[3][3];
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
inline v4<T>
Transform(mat4x4<T> A, v4<T> B)
{
    v4<T> Result;
    
    Result.x = B.x*A.E[0][0] + B.y*A.E[0][1] + B.z*A.E[0][2] + B.w*A.E[0][3];
    Result.y = B.x*A.E[1][0] + B.y*A.E[1][1] + B.z*A.E[1][2] + B.w*A.E[1][3];
    Result.z = B.x*A.E[2][0] + B.y*A.E[2][1] + B.z*A.E[2][2] + B.w*A.E[2][3];
    Result.w = B.x*A.E[3][0] + B.y*A.E[3][1] + B.z*A.E[3][2] + B.w*A.E[3][3];
    
    return Result;
}

//////////////////////////////////////////////////////////////////////


template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, mat4x4<T>>::type
Identity()
{
    mat4x4<T> Result = {{
            { 1.0, 0.0, 0.0, 0.0 },
            { 0.0, 1.0, 0.0, 0.0 },
            { 0.0, 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        }};
    
    return Result;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, mat4x4<T>>::type
Identity(v4<T> A)
{
    mat4x4<T> Result = {{
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 }
        }};
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, mat4x4<T>>::type
Translate(v4<T> A)
{
    mat4x4<T> Result = {{
            { 1.0, 0.0, 0.0, A.x },
            { 0.0, 1.0, 0.0, A.y },
            { 0.0, 0.0, 1.0, A.z },
            { 0.0, 0.0, 0.0, 1.0 }
        }};
    
    return Result;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, mat4x4<T>>::type
Translate(v4<T> A)
{
    mat4x4<T> Result = {{
            { 1, 0, 0, A.x },
            { 0, 1, 0, A.y },
            { 0, 0, 1, A.z },
            { 0, 0, 0,   1 }
        }};
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, mat4x4<T>>::type
Scale(v4<T> A)
{
    mat4x4<T> Result = {{
            { A.x, 0.0, 0.0, 0.0 },
            { 0.0, A.y, 0.0, 0.0 },
            { 0.0, 0.0, A.z, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        }};
    
    return Result;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, mat4x4<T>>::type
Scale(v4<T> A)
{
    mat4x4<T> Result = {{
            { A.x, 0,   0,  0 },
            {  0, A.y,  0,  0 },
            {  0,  0,  A.z, 0 },
            {  0,  0,   0,  1 }
        }};
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, mat4x4<T>>::type
XRotate(T Radian)
{
    T s = Sine(Radian);
    T c = Cosine(Radian);
    
    mat4x4<T> Result = {{
            { 1.0, 0.0, 0.0, 0.0 },
            { 0.0,   c,  -s, 0.0 },
            { 0.0,   s,   c, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        }};
    
    return Result;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, mat4x4<T>>::type
XRotate(T Radian)
{
    T s = static_cast<T>(Sine(Radian));
    T c = static_cast<T>(Cosine(Radian));
    
    mat4x4<T> Result = {{
            { 1, 0,  0, 0 },
            { 0, c, -s, 0 },
            { 0, s,  c, 0 },
            { 0, 0,  0, 1 }
        }};
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, mat4x4<T>>::type
InverseXRotate(T Radian)
{
    T s = Sine(Radian);
    T c = Cosine(Radian);
    
    mat4x4<T> Result = {{
            { 1.0, 0.0, 0.0, 0.0 },
            { 0.0,   c,   s, 0.0 },
            { 0.0,  -s,   c, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        }};
    
    return Result;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, mat4x4<T>>::type
InverseXRotate(T Radian)
{
    T s = static_cast<T>(Sine(Radian));
    T c = static_cast<T>(Cosine(Radian));
    
    mat4x4<T> Result = {{
            { 1,  0, 0, 0 },
            { 0,  c, s, 0 },
            { 0, -s, c, 0 },
            { 0,  0, 0, 1 }
        }};
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, mat4x4<T>>::type
YRotate(T Radian)
{
    T s = Sine(Radian);
    T c = Cosine(Radian);
    
    mat4x4<T> Result = {{
            {   c, 0.0,   s, 0.0 },
            { 0.0, 1.0, 0.0, 0.0 },
            {  -s, 0.0,   c, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        }};
    
    return Result;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, mat4x4<T>>::type
YRotate(T Radian)
{
    T s = static_cast<T>(Sine(Radian));
    T c = static_cast<T>(Cosine(Radian));
    
    mat4x4<T> Result = {{
            {  c, 0, s, 0 },
            {  0, 1, 0, 0 },
            { -s, 0, c, 0 },
            {  0, 0, 0, 1 }
        }};
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, mat4x4<T>>::type
InverseYRotate(T Radian)
{
    T s = Sine(Radian);
    T c = Cosine(Radian);
    
    mat4x4<T> Result = {{
            {   c, 0.0,  -s, 0.0 },
            { 0.0, 1.0, 0.0, 0.0 },
            {   s, 0.0,   c, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        }};
    
    return Result;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, mat4x4<T>>::type
InverseYRotate(T Radian)
{
    T s = static_cast<T>(Sine(Radian));
    T c = static_cast<T>(Cosine(Radian));
    
    mat4x4<T> Result = {{
            { c, 0, -s, 0 },
            { 0, 1,  0, 0 },
            { s, 0,  c, 0 },
            { 0, 0,  0, 1 }
        }};
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, mat4x4<T>>::type
ZRotate(T Radian)
{
    T s = Sine(Radian);
    T c = Cosine(Radian);
    
    mat4x4<T> Result = {{
            {   c,  -s, 0.0, 0.0 },
            {   s,   c, 0.0, 0.0 },
            { 0.0, 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        }};
    
    return Result;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, mat4x4<T>>::type
ZRotate(T Radian)
{
    T s = static_cast<T>(Sine(Radian));
    T c = static_cast<T>(Cosine(Radian));
    
    mat4x4<T> Result = {{
            { c, -s, 0, 0 },
            { s,  c, 0, 0 },
            { 0,  0, 1, 0 },
            { 0,  0, 0, 1 }
        }};
    
    return Result;
}

//////////////////////////////////////////////////////////////////////

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, mat4x4<T>>::type
InverseZRotate(T Radian)
{
    T s = Sine(Radian);
    T c = Cosine(Radian);
    
    mat4x4<T> Result = {{
            {   c,   s, 0.0, 0.0 },
            {  -s,   c, 0.0, 0.0 },
            { 0.0, 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        }};
    
    return Result;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, mat4x4<T>>::type
InverseZRotate(T Radian)
{
    T s = static_cast<T>(Sine(Radian));
    T c = static_cast<T>(Cosine(Radian));
    
    mat4x4<T> Result = {{
            {  c, s, 0, 0 },
            { -s, c, 0, 0 },
            {  0, 0, 1, 0 },
            {  0, 0, 0, 1 }
        }};
    
    return Result;
}

//////////////////////////////////////////////////////////////////////
