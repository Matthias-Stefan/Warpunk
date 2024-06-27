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

struct asset_resources_buffer_view
{
    size_t BufferIndex;
    size_t ByteOffset;
    size_t Count;
};

struct asset_loading_info
{
    char *Name;
    char *ModelFilepath;
    char *TextureFilepath;
};

struct asset
{
    asset_resources_buffer_view Vertices;
    asset_resources_buffer_view Indices;
    asset_resources_buffer_view Textures;
    asset_resources_buffer_view Materials;
};

struct asset_arena
{
    size_t VertexCount;
    typed_memory_block<vertex> Vertices;
    size_t IndexCount;
    typed_memory_block<u32> Indices;
    size_t TextureCount;
    typed_memory_block<texture> Textures;
    size_t MaterialCount;
    typed_memory_block<pbr_material> Materials;
};
