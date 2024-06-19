/* ========================================================================
   $File: warpunk.h $
   $Date: 29.10.2023 $
   $Creator: Matthias Stefan $
   ======================================================================== */

#ifndef WARPUNK_H
#define WARPUNK_H

#include <limits>

#include "source/core/math.h"
#include "source/core/queue.h"

#include "warpunk_world.h"
#include "warpunk_camera.h"

struct visual_component; 

struct game_state
{
    world World;
    camera Camera;
};

struct entity
{
    s32 ID;
    b32 IsSelected;
    v3<f32> Pos;
    
    f32 Orientation;
    v2<s32> Geo;
    
    const char *GeoPath;
    const char *TexturePath;
    visual_component *VisualComponent;
    
    v3<f32> Acceleration;
    v3<f32> Velocity;
    
    static_queue<v3<f32>, 256> QueuedPos;
};

struct aabb
{
    v2<f32> Min = { FLT_MAX, FLT_MAX }; // BottomLeft
    v2<f32> Max = { -FLT_MAX, -FLT_MAX }; // TopRight
};

inline f32  
RayIntersectsAABB(v2<f32> RayOrigin, v2<f32> RayD, aabb *Box)
{
    v2<f32> InvRayD = 1.0f / RayD;
    
    v2<f32> tBoxMin = (Box->Min - RayOrigin) * InvRayD;
    v2<f32> tBoxMax = (Box->Max - RayOrigin) * InvRayD;
    
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
SegmentIntersectsAABB(v2<f32> OriginP, v2<f32> DestP, aabb* Box) 
{
#if 0
    *Near = { FLT_MAX, FLT_MAX };
    *Far = { FLT_MAX, FLT_MAX };
#endif
    
    v2<f32> InvRayD = DestP - OriginP;
    InvRayD = Normalize(InvRayD);
    InvRayD.E[0] = (InvRayD.x != 0.0f) ? 1.0f / InvRayD.x : 0.0f;
    InvRayD.E[1] = (InvRayD.y != 0.0f) ? 1.0f / InvRayD.y : 0.0f;
    
    v2<f32> tBoxMin = (Box->Min - OriginP) * InvRayD;
    v2<f32> tBoxMax = (Box->Max - OriginP) * InvRayD;
    
    f32 tMin = Max(Min(tBoxMin.x, tBoxMax.x), 
                   Min(tBoxMin.y, tBoxMax.y));
    f32 tMax = Min(Max(tBoxMin.x, tBoxMax.x), 
                   Max(tBoxMin.y, tBoxMax.y));
    
    f32 Result = false;
    if((tMin > 0.0f) && (tMin < tMax))
    {
        f32 t = (tMin < 0.0f) ? tMax : tMin;
        
        f32 SqDist = MagnitudeSq(DestP - OriginP);
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
    v2<f32> Pos;
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

#endif //WARPUNK_H
