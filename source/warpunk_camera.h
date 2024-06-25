#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "source/core/math.h"

enum camera_movement_type
{
    CameraMovement_Tumble,
    CameraMovement_Track,
    CameraMovement_Dolly,
    
    CameraMovement_Count
};

struct camera
{
    glm::vec3 Pos;
    glm::vec3 Dir;
    glm::vec3 Up;
    glm::vec3 Right;
    
    f32 Yaw; // Gieren
    f32 Pitch; //Nicken
    f32 Roll; // Rollen
    
    f32 FocalLength;
    f32 SensorHeight;

    f32 NearClipPlane;
    f32 FarClipPlane;
    
    glm::mat4 Projection; 
    glm::mat4 View;
    
    f32 Speed;
};

inline void
InitializeCamera(camera *Camera)
{
    *Camera = {};
    Camera->Pos = { 0.0f, 0.0f, 0.0f };
    Camera->Dir = { -1.0f, 0.0f, 0.0f };
    Camera->Up = { 0.0f, 1.0f, 0.0f };
    Camera->Right = glm::normalize(glm::cross(Camera->Dir, Camera->Up));

    Camera->FocalLength = 50.0f; 
    Camera->SensorHeight = 24.0f;
    Camera->NearClipPlane = 0.1f;
    Camera->FarClipPlane = 100.0f;

    Camera->Speed = 0.1f;
}


