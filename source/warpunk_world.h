#pragma once

struct entity;
struct aabb;

struct world
{
    bool IsValid;
    
    u32 EntityCount;
    entity *Entities;
    
    u32 ConstructionSize;
    u32 ConstructionCount;
    aabb *Constructions;
};

internal void AddConstruction(world *World, aabb Construction);
