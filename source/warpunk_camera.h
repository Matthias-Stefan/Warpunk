#pragma once

#include "source/core/math.h"
#include "source/core/vec.h"

enum camera_movement_type
{
    CameraMovement_Tumble,
    CameraMovement_Track,
    CameraMovement_Dolly,
    
    CameraMovement_Count
};

struct camera
{
    v3<f32> Pos;
    v3<f32> Target;
    v3<f32> Up;
    v3<f32> Right;
    v3<f32> Dir;
    
    f32 Yaw; // Gieren
    f32 Pitch; //Nicken
    f32 Roll; // Rollen
    
    f32 FocalLength;
    f32 NearClipPlane;
    f32 FarClipPlane;
    
    m4x4 Projection; 
    m4x4 View;
    
    f32 Speed;
};

inline void
InitializeCamera(camera *Camera)
{
    *Camera = {};
    Camera->Pos = { 0.0f, 0.0f, 3.0f };
    Camera->Target = { 0.0f, 0.0f, 0.0f };
    Camera->Dir = Normalize(Camera->Pos - Camera->Target);
    Camera->Up = { 0.0f, 1.0f, 0.0f };
    Camera->Right = Normalize(Cross(Camera->Dir, Camera->Up));
    Camera->Speed = 0.1f;
}

inline m4x4
GetOrthographicProjectionMatrix(f32 Left, f32 Right, 
                                f32 Bottom, f32 Top, 
                                f32 Near, f32 Far)
{
    m4x4 Result = {};
    
    Result.E[0][0] = 2.0f / (Right - Left);
    Result.E[0][1] = 0.0f;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = -(Right + Left) / (Right - Left);
    
    Result.E[1][0] = 0.0f;
    Result.E[1][1] = 2.0f / (Top - Bottom);
    Result.E[1][2] = 0.0f;
    Result.E[1][3] = -(Top + Bottom) / (Top - Bottom);
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = 0.0f;
    Result.E[2][2] = -2.0f / (Far - Near);
    Result.E[2][3] = -(Far + Near) / (Far - Near);
    
    Result.E[3][0] = 0.0f; 
    Result.E[3][1] = 0.0f;
    Result.E[3][2] = 0.0f;
    Result.E[3][3] = 1.0f;
    
    return Result;
}
