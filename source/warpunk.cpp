#include "warpunk.h"

#include "source/core/queue.h"
#include "source/platform/keyboard.h"
#include "source/platform/mouse.h"
#include "source/platform/platform.h"

#include "warpunk_world.cpp"

extern "C" GAME_INITIALIZE(GameInitialize)
{
    memory_block *Memory = GameContext->Memory;
    game_state *GameState = Memory->PushStruct<game_state>();
    GameContext->GameState = GameState;
    platform_api *PlatformAPI = GameContext->PlatformAPI;
    
    u32 EntityCount = 1;
    GameState->World.EntityCount = EntityCount;
    GameState->World.Entities = Memory->PushArray<entity>(EntityCount);
    GameState->World.IsValid = true;
    
    u32 _x = 0;
    entity *Entity = GameState->World.Entities;
    for (f32 EntityIndex = 0; EntityIndex < EntityCount; ++EntityIndex)
    { 
        Entity->ID = EntityIndex;
        Entity->Pos = { -(_x * EntityIndex + _x)-300.0f, -200.0f, 0.0f }; 
        
        Entity->GeoPath = "W:/Warpunk/assets/geo/untitled.obj";
        Entity->TexturePath = "W:/Warpunk/assets/textures/viking_room.png";
        Entity->Orientation = 0.0f;
        
        Entity++;
    }
    
    //
    // Camera
    //
    
    InitializeCamera(&GameState->Camera);
    GameState->Camera.Pos = { 45.1697464f, 25.2711525f, 14.8278751f };
    GameState->Camera.Dir = { -0.84403336f, -0.427358776f, -0.323994398f };
    GameState->Camera.Up = { -0.398973674f, 0.904082119f, -0.153151825f };
    GameState->Camera.Right = { 0.358368307f, 0.0f, -0.933580279f };
    GameState->Camera.Yaw = -21.0000229f;
    GameState->Camera.Pitch = 25.3000546f;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    memory_block *Memory = GameContext->Memory;
    game_state *GameState = GameContext->GameState;
    platform_api *PlatformAPI = GameContext->PlatformAPI;
    
    world *World = &GameState->World;
    camera *Camera = &GameState->Camera;
    
    mouse *Mouse = &Input->Mouse;
    keyboard *Keyboard = &Input->Keyboard;
    
    /// Selection cast
    temporary_memory_block<entity> Selection;
    StartTemporaryMemory(&Selection, 32, PlatformAPI->AllocateMemory);


    EndTemporaryMemory(&Selection, PlatformAPI->DeallocateMemory);
    /// Update camera
    
    if (Input->Mouse.IsPressed(MouseButton_Left))
    {
        // Tumble
        if (Input->Mouse.IsMoving)
        {
            // Update Camera
            Camera->Yaw += Mouse->DraggingRecentDtx * Camera->Speed;
            Camera->Pitch += Mouse->DraggingRecentDty * Camera->Speed;
            
            glm::vec3 Dir;
            Dir.x = -cos(glm::radians(Camera->Yaw)) * cos(glm::radians(Camera->Pitch));
            Dir.y = -sin(glm::radians(Camera->Pitch));
            Dir.z = sin(glm::radians(Camera->Yaw)) * cos(glm::radians(Camera->Pitch));
            Camera->Dir = glm::normalize(Dir);
            Camera->Right = glm::normalize(glm::cross(Dir, glm::vec3(0.0f, 1.0f, 0.0f)));
            Camera->Up = glm::normalize(glm::cross(Camera->Right, Camera->Dir));
        }
    }
    else if (Input->Mouse.IsPressed(MouseButton_Middle))
    {
        // Track
        if (Input->Mouse.IsMoving)
        {
            f32 MouseXSpeed = Mouse->DraggingRecentDtx * Camera->Speed;
            f32 MouseYSpeed = Mouse->DraggingRecentDty * Camera->Speed;
            
            glm::vec3 MoveVectorHorizontal = -Camera->Right * MouseXSpeed;
            glm::vec3 MoveVectorVertical = Camera->Up * MouseYSpeed;
            
            Camera->Pos += MoveVectorHorizontal;
            Camera->Pos -= MoveVectorVertical;
        }
    }
    else if (Input->Mouse.IsPressed(MouseButton_Right))
    {
        // Dolly
        if (Input->Mouse.IsMoving)
        {
            Camera->Pos = Camera->Pos + Camera->Dir * (Mouse->DraggingRecentDtx * Camera->Speed);
        }
    }
    
    //
    //
    //
    
#if false
    entity *Entity = (entity  *)GameState->World.Entities;
    for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; ++EntityIndex)
    {
        if (Input->Mouse.State[PlatformMouseButton_Left].IsDown)
        {
            if ((Entity->Pos.y + Entity->Geo.Height > Mouse->DownPy) &&
                (Entity->Pos.y < Mouse->DownPy) &&
                (Entity->Pos.x + Entity->Geo.Width > Mouse->DownPx) &&
                (Entity->Pos.x < Mouse->DownPx))
            {
                Entity->IsSelected = true;
            }
            else
            {
                Entity->IsSelected = false;
            }
        }
        
        if (Input->Mouse.State[PlatformMouseButton_Left].IsDragging)
        {
            if ((Entity->Pos.y + Entity->Geo.Height > Min(Mouse->DownPy, Mouse->Py)) && 
                (Entity->Pos.y < Max(Mouse->DownPy, Mouse->Py)) &&
                (Entity->Pos.x + Entity->Geo.Width > Min(Mouse->DownPx, Mouse->Px)) &&
                (Entity->Pos.x < Max(Mouse->DownPx, Mouse->Px)))
            {
                Entity->IsSelected = true;
            }
            else
            {
                Entity->IsSelected = false;
            }
        }
        
        Entity++;
    }
    
    Entity = (entity *)GameState->World.Entities;
    aabb SelectionGroup = {};
    for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; ++EntityIndex)
    {
        if (!Entity->IsSelected)
        {
            Entity++;
            continue;
        }
        SelectionGroup.Min.x = Min(SelectionGroup.Min.x, 
                                   Entity->Pos.x);
        SelectionGroup.Min.y = Min(SelectionGroup.Min.y, 
                                   Entity->Pos.y);
        SelectionGroup.Max.x = Max(SelectionGroup.Max.x, 
                                   Entity->Pos.x + Entity->Geo.Width);
        SelectionGroup.Max.y = Max(SelectionGroup.Max.y, 
                                   Entity->Pos.y + Entity->Geo.Height);
        
        Entity++;
    }
    
    v2<f32> CenterSelectionGroup = {};
    CenterSelectionGroup.x = (SelectionGroup.Max.x + SelectionGroup.Min.x) * 0.5f;
    CenterSelectionGroup.y = (SelectionGroup.Max.y + SelectionGroup.Min.y) * 0.5f;
    
    v2<f32> DeltaSelectionGroup = {};
    if (Input->Mouse.State[PlatformMouseButton_Right].IsDown)
    {
        DeltaSelectionGroup.x = Mouse->DownPx - CenterSelectionGroup.x;
        DeltaSelectionGroup.y = Mouse->DownPy - CenterSelectionGroup.y;
    }
    
    Entity = (entity *)GameState->World.Entities;
    for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; ++EntityIndex)
    {
        if (Input->Mouse.State[PlatformMouseButton_Right].IsDown)
        {
            if (Entity->IsSelected)
            {
                FlushQueue(&Entity->QueuedPos);
                v2<f32> Destination = {
                    Entity->Pos.x + DeltaSelectionGroup.x,
                    Entity->Pos.y + DeltaSelectionGroup.y
                };
                PathfindAStar(Entity, &Destination, GameState);
            }
        }
        
        if (Entity->QueuedPos.Size > 0)
        {
            v4 CurrentTarget = *GetQueueItemAt(&Entity->QueuedPos, 0);  
            
            v4 Target =  CurrentTarget - Entity->Pos;
            v4 Dir = Transform(ZRotate(Entity->Orientation), { 1.0f, 0.0f, 0.0f, 0.0f });
            f32 MagnitudeDir = Magnitude(Dir);
            
            f32 Angle = ArcTangent2(Target.y, Target.x) - ArcTangent2(Dir.y, Dir.x); 
            f32 ComplementAngle = 2 * PI - Abs(Angle);
            if (ComplementAngle < PI)
            {
                if (Angle < 0.0f)
                {
                    Angle = ComplementAngle; 
                }
                else
                {
                    Angle = -ComplementAngle;
                }
            }
            
            Entity->Orientation += Angle * 10 * Input->dtForFrame;
            
            f32 DestRadius = 5.25f;
            
            f32 PointingAway = Inner(Target, Normalize(Target));
            f32 InsideDist  = Inner(Target, Target) - DestRadius * DestRadius;
            
            f32 Threshold = 0.9f;
            if (InsideDist > 0.0f && PointingAway > 0.0f)
            {
                if (Abs(Angle) < Threshold)
                {
                    Entity->Acceleration = 8000.0f * Dir;
                    Entity->Velocity = Normalize(Entity->Acceleration * Input->dtForFrame + Entity->Velocity);
                    f32 MaxSpeed = 20000.0f;
                    Entity->Velocity *= MaxSpeed * Input->dtForFrame;
                    Entity->Pos = (0.5f * Entity->Acceleration * Input->dtForFrame * Input->dtForFrame + 
                                   Entity->Velocity * Input->dtForFrame + 
                                   Entity->Pos);
                }
            }
            else
            {
                ReleaseQueueItem(&Entity->QueuedPos);
            }
            
            Entity->Velocity = {};
        }
        
        Entity++;
    }
#endif
    //
    // Rendering
    //
    
    
    
}