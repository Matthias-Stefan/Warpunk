#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

#include "source/core/math.h"
#include "source/core/queue.h"
#include "source/platform/asset.h"

#include "warpunk_world.h"
#include "warpunk_camera.h"

struct game_state
{
    world World;
    camera Camera;
};

struct entity
{
    char *Name;
    uuid Id;

    glm::mat4 Transform = glm::mat4(1);
    asset *Asset;
#if true
    s32 ID;
    b32 IsSelected;
    glm::vec3 Pos;
    
    f32 Orientation;
    glm::vec2 Geo;
    
    const char *GeoPath;
    const char *TexturePath;
    
    glm::vec3 Acceleration;
    glm::vec3 Velocity;
    
    static_queue<glm::vec3, 256> QueuedPos;
#endif
};

struct aabb
{
    glm::vec2 Min = { FLT_MAX, FLT_MAX }; // BottomLeft
    glm::vec2 Max = { -FLT_MAX, -FLT_MAX }; // TopRight
};

inline f32  
RayIntersectsAABB(glm::vec2 RayOrigin, glm::vec2 RayD, aabb *Box)
{
    glm::vec2 InvRayD = 1.0f / RayD;
    
    glm::vec2 tBoxMin = (Box->Min - RayOrigin) * InvRayD;
    glm::vec2 tBoxMax = (Box->Max - RayOrigin) * InvRayD;
    
    f32 tMin = Max(Min(tBoxMin.x, tBoxMax.x), 
                   Min(tBoxMin.y, tBoxMax.y));
    f32 tMax = Min(Max(tBoxMin.x, tBoxMax.x), 
                   Max(tBoxMin.y, tBoxMax.y));
    
    f32 Result = FLT_MAX;
    if((tMin > 0.0f) && (tMin < tMax))
    {
        Result = tMin;
    }
    
    return Result;
}

inline b32 
SegmentIntersectsAABB(glm::vec2 OriginP, glm::vec2 DestP, aabb* Box)
{
#if 0
    *Near = { FLT_MAX, FLT_MAX };
    *Far = { FLT_MAX, FLT_MAX };
#endif
    
    glm::vec2 InvRayD = glm::normalize(DestP - OriginP);
    InvRayD.x = (InvRayD.x != 0.0f) ? 1.0f / InvRayD.x : 0.0f;
    InvRayD.y = (InvRayD.y != 0.0f) ? 1.0f / InvRayD.y : 0.0f;
    
    glm::vec2 tBoxMin = (Box->Min - OriginP) * InvRayD;
    glm::vec2 tBoxMax = (Box->Max - OriginP) * InvRayD;
    
    f32 tMin = Max(Min(tBoxMin.x, tBoxMax.x), 
                   Min(tBoxMin.y, tBoxMax.y));
    f32 tMax = Min(Max(tBoxMin.x, tBoxMax.x), 
                   Max(tBoxMin.y, tBoxMax.y));
    
    f32 Result = false;
    if((tMin > 0.0f) && (tMin < tMax))
    {
        f32 t = (tMin < 0.0f) ? tMax : tMin;
        
        glm::vec2 Diff = DestP - OriginP;
        f32 SqDist = glm::dot(Diff, Diff);
        if (t > 0.0f && (t * t) < SqDist)
        {
#if 0
            *Near = OriginP + tMin * InvRayD;
            *Far = OriginP + tMax * InvRayD;
#endif
            Result = true;
        }
    }
    
    return Result;
}

enum node_category
{
    NodeCategory_Closed,
    NodeCategory_Open,
    NodeCategory_Unvisited,
};

struct connection;
struct node
{
    u64 ID;
    glm::vec2 Pos;
    bool IsTarget;
    
    node *Prev;
    u32 ConnectionCount;
    connection *Connection;
};

struct connection 
{
    node *FromNode;
    node *ToNode;
    f32 Cost;
};

struct node_record
{
    node *Node;
    connection *Connection;
    f32 CostSoFar;
    f32 EstimatedTotalCost;
    node_category Category;
};

inline void
AssignRoute(entity *Entity, node *Node)
{
    if (Node->Prev)
    {
        AssignRoute(Entity, Node->Prev);
    }
    
    Entity->QueuedPos.Enqueue({ Node->Pos.x, Node->Pos.y, 0.0f });
}
