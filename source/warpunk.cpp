#include "warpunk.h"

#include "source/core/matrix.h"
#include "source/core/memory.h"
#include "source/core/queue.h"
#include "source/core/vec.h"
#include "source/platform/keyboard.h"
#include "source/platform/mouse.h"
#include "source/platform/platform.h"

#include "warpunk_world.cpp"

internal void AddConstruction(world *World, aabb Construction)
{
    //Assert(World->Constructions);
    World->Constructions[World->ConstructionCount] = Construction;
    World->ConstructionCount++;
}

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
    
    s32 Width = Input->RenderWidth / 2;
    s32 Height = Input->RenderHeight / 2;
    GameState->Camera.Projection = GetOrthographicProjectionMatrix(-Width, Width,
                                                                   -Height, Height,
                                                                   0.1f, 100.0f);
    u32 ConstructionSize = 256;
    GameState->World.ConstructionSize = ConstructionSize;
    GameState->World.Constructions =  Memory->PushArray<aabb>(ConstructionSize); 
    
    AddConstruction(&GameState->World, { v2<f32>{ 0.0f, 0.0f }, v2<f32>{ 100.0f, 100.0f} });
    AddConstruction(&GameState->World, { v2<f32>{ 150.0f, 150.0f }, v2<f32>{ 250.0f, 250.0f} });
    AddConstruction(&GameState->World, { v2<f32>{ -250.0f, -250.0f }, v2<f32>{ -150.0f, -150.0f} });
    
#if false
    DebugInfo->Mouse = &Input->Mouse;
    DebugInfo->Keyboard = &Input->Keyboard;
    DebugInfo->Camera = &GameState->Camera;
#endif
}

internal bool 
FindRecordInQueue(static_queue<node_record, 256> *Queue, 
                  node *Node, 
                  node_record *Record, 
                  u32 *Index)
{
    for (u32 QueueIndex = Queue->Head; 
         QueueIndex != Queue->Tail; 
         QueueIndex = (QueueIndex + 1) & Queue->Size)
    {
        node_record *CurrentRecord = &Queue->E[QueueIndex];
        if (Node->ID == CurrentRecord->Node->ID)
        {
            *Record = *CurrentRecord;
            *Index = QueueIndex;
            return true;
        } 
    }
    return false;
}

#include <intrin.h>
internal void
PathfindAStar(entity *Entity, v2<f32> *Destination, game_state *GameState)
{
#if false
    wc::memory_block *Memory = GameState->Memory;
    v2<f32> EntityPos = { Entity->Pos.x, Entity->Pos.y };
    
    //
    //  Init Node-Graph
    //
    
    u32 NodeCount = 2; 
    for (u32 ConstructionIndex = 0;
         ConstructionIndex < GameState->World.ConstructionCount;
         ++ConstructionIndex)
    {
        if (SegmentIntersectsAABB(EntityPos, *Destination, &GameState->World.Constructions[ConstructionIndex]))
        {
            NodeCount += 4;
        }
    }
    
    if (NodeCount > 2)
    {
        node *Graph = (node *)PushSizeTemporary(Memory, sizeof(node) * NodeCount);
        u32 GraphSize = 0;
        Graph[GraphSize++] = { GraphSize, EntityPos, false, nullptr, 0, nullptr };
        for (u32 ConstructionIndex = 0;
             ConstructionIndex < GameState->World.ConstructionCount;
             ++ConstructionIndex)
        {
            if (SegmentIntersectsAABB(EntityPos, *Destination, &GameState->World.Constructions[ConstructionIndex]))
            {
                aabb ConstructionExtruded = GetExtrudedBox(&GameState->World.Constructions[ConstructionIndex], 
                                                           Entity->Geo.Width + 20);
                Graph[GraphSize++] = { 
                    GraphSize, ConstructionExtruded.Min, false, nullptr, 0, nullptr };
                Graph[GraphSize++] = {
                    GraphSize, { ConstructionExtruded.Max.x, ConstructionExtruded.Min.y }, false, nullptr, 0, nullptr };
                Graph[GraphSize++] = { 
                    GraphSize, ConstructionExtruded.Max, false, nullptr, 0, nullptr };
                Graph[GraphSize++] = {
                    GraphSize, { ConstructionExtruded.Min.x, ConstructionExtruded.Max.y }, false, nullptr, 0, nullptr };
            }
        }
        Graph[GraphSize++] = { GraphSize, *Destination, true, nullptr, 0, nullptr }; 
        
        //
        // Init Connections
        //
        
        for (u32 FromNodeIndex = 0;
             FromNodeIndex< GraphSize;
             ++FromNodeIndex)
        {
            node *FromNode = &Graph[FromNodeIndex];
            FromNode->ConnectionCount = 0;
            for (u32 ToNodeIndex = 0;
                 ToNodeIndex < GraphSize;
                 ++ToNodeIndex)
            {
                if (FromNodeIndex == ToNodeIndex)
                {
                    continue;
                }
                
                node *ToNode = &Graph[ToNodeIndex];
                
                bool IsReachable = true;
                for (u32 ConstructionIndex = 0;
                     ConstructionIndex < GameState->World.ConstructionCount;
                     ++ConstructionIndex)
                {
                    if (SegmentIntersectsAABB(FromNode->Pos, ToNode->Pos, 
                                              &GameState->World.Constructions[ConstructionIndex]))
                    {
                        IsReachable = false;
                        break;
                    }
                }
                if (IsReachable)
                {
                    FromNode->ConnectionCount++;
                }
            }
            
            FromNode->Connection = (connection *)PushSizeTemporary(Memory, sizeof(connection)*FromNode->ConnectionCount);
            u32 ConnectionIndex = 0;
            for (u32 ToNodeIndex = 0;
                 ToNodeIndex < GraphSize;
                 ++ToNodeIndex)
            {
                if (FromNodeIndex == ToNodeIndex)
                {
                    continue;
                }
                
                node *ToNode = &Graph[ToNodeIndex];
                
                bool IsReachable = true;
                for (u32 ConstructionIndex = 0;
                     ConstructionIndex < GameState->World.ConstructionCount;
                     ++ConstructionIndex)
                {
                    if (SegmentIntersectsAABB(FromNode->Pos, ToNode->Pos, 
                                              &GameState->World.Constructions[ConstructionIndex]))
                    {
                        IsReachable = false;
                        break;
                    }
                }
                if (IsReachable)
                {
                    FromNode->Connection[ConnectionIndex++] = { FromNode, ToNode, DistanceSq(FromNode->Pos, ToNode->Pos) };
                }
            }
        }
        
        //
        // A*
        //
        
        queue<node_record> OpenQueue(Memory, 1024, MallocType_Temporary);
        queue<node_record> ClosedQueue(Memory, 1024, MallocType_Temporary);
        
        node *StartNode = &Graph[0];
        
        node_record StartRecord = {};
        StartRecord.Node = StartNode;
        StartRecord.Connection = nullptr;
        StartRecord.CostSoFar = 0.0f;
        StartRecord.EstimatedTotalCost = DistanceSq(StartNode->Pos, Graph[GraphSize-1].Pos);
        StartRecord.Category = NodeCategory_Open;
        
        AddQueueItem(&OpenQueue, StartRecord);
        
        node_record *CurrentRecord = nullptr;
        while (OpenQueue.Fill > 0)
        {
            CurrentRecord = GetQueueItemAt(&OpenQueue, 0);
            if (CurrentRecord->Node->IsTarget)
            {
                break;
            }
            
            for (u32 ConnectionIndex = 0;
                 ConnectionIndex < CurrentRecord->Node->ConnectionCount;
                 ++ConnectionIndex)
            {
                connection *Connection = &CurrentRecord->Node->Connection[ConnectionIndex];
                node *ToNode = Connection->ToNode;
                f32 ToNodeCost = CurrentRecord->CostSoFar + Connection->Cost;
                
                node_record ToNodeRecord;
                f32 ToNodeHeuristic;
                u32 QueueIndex = 0;
                if (FindRecordInQueue(&ClosedQueue, ToNode, &ToNodeRecord, &QueueIndex))
                {
                    if (ToNodeRecord.CostSoFar <= ToNodeCost)
                    {
                        continue;
                    }
                    
                    ReleaseQueueItemAt(&ClosedQueue, QueueIndex);
                    ToNodeHeuristic = ToNodeRecord.EstimatedTotalCost - ToNodeRecord.CostSoFar;
                }
                else if (FindRecordInQueue(&OpenQueue, ToNode, &ToNodeRecord, &QueueIndex))
                {
                    if (ToNodeRecord.CostSoFar <= ToNodeCost)
                    {
                        continue;
                    }
                    
                    ToNodeHeuristic = ToNodeRecord.EstimatedTotalCost - ToNodeRecord.Connection->Cost;
                }
                else
                {
                    ToNodeRecord.Node = ToNode;
                    ToNodeRecord.Connection = Connection;
                    ToNodeHeuristic = DistanceSq(ToNode->Pos, Graph[GraphSize-1].Pos); 
                }
                
                ToNodeRecord.Connection->Cost = ToNodeCost;
                ToNodeRecord.Connection = Connection;
                Connection->ToNode->Prev = Connection->FromNode;
                ToNodeRecord.EstimatedTotalCost = ToNodeCost + ToNodeHeuristic;
                
                if (!FindRecordInQueue(&OpenQueue, ToNode, &ToNodeRecord, &QueueIndex))
                {
                    AddQueueItem(&OpenQueue, ToNodeRecord);
                } 
            } 
            
            u32 OpenQueueIndex;
            FindRecordInQueue(&OpenQueue, CurrentRecord->Node, CurrentRecord, &OpenQueueIndex);
            ReleaseQueueItemAt(&OpenQueue, OpenQueueIndex);
            
            u32 ClosedQueueIndex;
            AddQueueItem(&ClosedQueue, *CurrentRecord);
        }
        
        if (CurrentRecord->Node->IsTarget)
        {
            //AssignRoute(Entity, CurrentRecord->Node);
        }
    }
    else
    {
        AddQueueItem(&Entity->QueuedPos, { Destination->x, Destination->y, 0.0f, 1.0f });
    }
    
    FlushTemporary(Memory);
#endif
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
    
    v2f RenderDim = { 
        static_cast<f32>(Input->RenderWidth), 
        static_cast<f32>(Input->RenderWidth) 
    }; 
    
#if false
    MouseP = ScreenPosToWorldPos((f32)Input->RenderWidth, (f32)Input->RenderHeight, 
                                 Input->Mouse.P, Camera->Pos);
    MouseDownP = ScreenPosToWorldPos(RenderDim.Width, RenderDim.Height, 
                                     Input->Mouse.DownP, Camera->Pos);
#endif
    //
    // Update Camera
    //
    
    v3f Dir = Normalize(Camera->Target - Camera->Pos);
    f32 Dist = SquareRoot(MagnitudeSq(Camera->Target - Camera->Pos)); 
    if (Input->Mouse.IsPressed(MouseButton_Left))
    {
        // Tumble
        const v4f Up = { 0.0f, 1.0f, 0.0f, 0.0f }; 
        
        if (Input->Mouse.IsMoving)
        {
            
            Camera->Yaw += Mouse->DraggingRecentDtx * Camera->Speed * 0.01f;
            if (Camera->Yaw > TAU)
            {
                Camera->Yaw = Camera->Yaw - TAU; 
            }
            else if (Camera->Yaw < 0.0f)
            {
                Camera->Yaw = TAU - Camera->Yaw; 
            }
            
            Camera->Pitch += Mouse->DraggingRecentDty * Camera->Speed * 0.01f;
            if (Camera->Pitch > PI_2)
            {
                Camera->Pitch = PI_2; 
            }
            else if (Camera->Pitch < -PI_2)
            {
                Camera->Pitch = -PI_2; 
            }
            mat4x4<f32> RotYaw = YRotate(Camera->Yaw);
            mat4x4<f32> RotPitch = XRotate(-Camera->Pitch);
            
            mat4x4<f32> CombinedRotation = RotYaw * RotPitch;
            Dir = Normalize((v3<f32>)(CombinedRotation * ((v4f)Camera->Dir)));
            
            Camera->Target = Camera->Pos - Dir * Dist;
            Camera->Up = Normalize((v3f)(CombinedRotation * Up));
            
            Camera->Right = Normalize(Cross(Dir, Camera->Up));
        }
    }
    else if (Input->Mouse.IsPressed(MouseButton_Middle))
    {
        // Track
        if (Input->Mouse.IsMoving)
        {
            f32 MouseXSpeed = Mouse->DraggingRecentDtx * Camera->Speed;
            f32 MouseYSpeed = Mouse->DraggingRecentDty * Camera->Speed;
            
            v3<f32> MoveVectorHorizontal = Camera->Right * MouseXSpeed;
            v3<f32> MoveVectorVertical = Camera->Up * MouseYSpeed;
            
            Camera->Pos += MoveVectorHorizontal;
            Camera->Pos -= MoveVectorVertical;
            Camera->Target += MoveVectorHorizontal;
            Camera->Target -= MoveVectorVertical;
        }
    }
    else if (Input->Mouse.IsPressed(MouseButton_Right))
    {
        // Dolly
        if (Input->Mouse.IsMoving)
        {
            Camera->Pos = Camera->Pos + Dir * (Mouse->DraggingRecentDtx * Camera->Speed);
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