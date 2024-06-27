#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "source/core/types.h"

struct vertex
{
    glm::vec3 Pos;
    glm::vec3 Normal;
    glm::vec4 Color;
    glm::vec4 TangentSpace;
    glm::vec2 TexCoord0;
    glm::vec2 TexCoord1;
};

inline void
ApplyTransform(vertex *Vertex, glm::mat4 *Matrix)
{
    auto NewPosition = *Matrix * glm::vec4(Vertex->Pos, 1.0);
    Vertex->Pos = glm::vec3(NewPosition.x, NewPosition.y, NewPosition.z);
    glm::mat3 NormalMatrix = glm::inverseTranspose(glm::mat3(*Matrix));
    Vertex->Normal = NormalMatrix * Vertex->Normal;
    Vertex->TangentSpace = glm::inverseTranspose(*Matrix) * Vertex->TangentSpace;
}

struct texture
{
    s32 Width;
    s32 Height;
    void *Data; 
};

struct pbr_material
{
    texture *BaseColorTexture;
    glm::vec4 BaseColorFactor;

    texture *MetallicRoughnessTexture;
    f32 MetallicFactor;
    f32 RoughnessFactor;
};

struct mesh
{
    u32 VertextCount;
    u32 VerticesSize;
    vertex *Vertices;

    u32 IndexCount;
    u32 IndicesSize;
    u32 *Indices;

    u32 MaterialIndex;
};

struct asset
{
    size_t Id;
    size_t Size = sizeof(asset);

    size_t MeshCount;
    size_t MaterialCount;
    size_t TextureCount;
    
    mesh *Meshes;
    pbr_material *Material;
    texture *Textures;
};