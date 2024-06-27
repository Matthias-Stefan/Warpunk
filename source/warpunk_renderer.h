#pragma once

#define MAX_FRAMES_IN_FLIGHT 2

#define UNSUCCESSFUL(Result) (Result != VK_SUCCESS) 

#include "source/core/types.h"
#include "source/platform/asset.h"
#include "source/platform/platform.h"


inline VkVertexInputBindingDescription
GetVertexInputBindingDescription()
{
    VkVertexInputBindingDescription VertexInputBindingDescription = {}; 
    VertexInputBindingDescription.binding = 0;
    VertexInputBindingDescription.stride = sizeof(vertex);
    VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    return VertexInputBindingDescription;
}

inline VkVertexInputAttributeDescription *
GetVertexInputAttributeDescription(u32 *AttributeDescriptionCount)
{
    persist VkVertexInputAttributeDescription VertexInputAttributeDescription[3];
    
    VertexInputAttributeDescription[0].binding = 0;
    VertexInputAttributeDescription[0].location = 0;
    VertexInputAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    VertexInputAttributeDescription[0].offset = offsetof(vertex, Pos);
    
    VertexInputAttributeDescription[1].binding = 0;
    VertexInputAttributeDescription[1].location = 1;
    VertexInputAttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    VertexInputAttributeDescription[1].offset = offsetof(vertex, Color);
    
    VertexInputAttributeDescription[2].binding = 0;
    VertexInputAttributeDescription[2].location = 2;
    VertexInputAttributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
    VertexInputAttributeDescription[2].offset = offsetof(vertex, TexCoord0);
    
    *AttributeDescriptionCount= ArraySize(VertexInputAttributeDescription);
    return VertexInputAttributeDescription;
}

struct swap_chain_support_details
{
    VkSurfaceCapabilitiesKHR Capabilities;
    
    VkSurfaceFormatKHR *Formats;
    u32 FormatCount;
    
    VkPresentModeKHR *PresentModes;
    u32 PresentModeCount;
};

struct vulkan_queue
{
    VkQueue Queue;
    u32 FamilyIndex;
};

struct vulkan_context
{
    memory_block *GraphicsMemoryBlock;
    platform_api *PlatformAPI;
    
    VkInstance Instance;
    VkSurfaceKHR Surface;
    VkPhysicalDevice PhysicalDevice;
    VkPhysicalDeviceProperties PhysicalDeviceProperties;
    VkDevice Device;
    
    VkRenderPass Renderpass;
    
    VkDescriptorSetLayout DescriptorSetLayout; 
    VkPipelineLayout PipelineLayout;
    VkPipeline GraphicsPipeline;
    
    u32 QueueCount = 1;
    vulkan_queue GraphicsQueue;
    
    asset_arena AssetArena;


    VkBuffer ViewProjectionBuffer;
    VkDeviceMemory ViewProjectionBufferMemory;
    void *ViewProjectionBufferMapped;

    VkBuffer *ModelBuffers0;
    VkDeviceMemory *ModelBuffersMemory0;
    void **ModelBuffersMapped0;

    VkBuffer *ModelBuffers1;
    VkDeviceMemory *ModelBuffersMemory1;
    void **ModelBuffersMapped1;






    VkBuffer VertexBuffer;
    VkDeviceMemory VertexBufferMemory;
    VkBuffer IndexBuffer;
    VkDeviceMemory IndexBufferMemory;
    
    VkBuffer *UniformBuffers;
    VkDeviceMemory *UniformBuffersMemory;
    void **UniformBuffersMapped;
    
    VkDescriptorPool DescriptorPool;
    VkDescriptorSet *DescriptorSets;
    
    u32 SwapChainCount = 1;
    VkSwapchainKHR SwapChain;
    u32 SwapChainImagesCount;
    VkImage *SwapChainImages;
    VkFormat SwapChainImageFormat;
    VkExtent2D SwapChainExtent;
    VkImageView *SwapChainImageViews;
    
    VkFramebuffer *SwapChainFramebuffers;
    
    VkCommandPool CommandPool;
    VkCommandBuffer *CommandBuffers;
    
    VkSemaphore *ImageAvailableSemaphores;
    VkSemaphore *RenderFinishedSemaphores;
    VkFence *InFlightFences;
    
    b32 FramebufferResized = 0;
    
    VkImage TextureImage;
    VkDeviceMemory TextureImageMemory;
    VkImageView TextureImageView;
    VkSampler TextureSampler;
    
    VkImage DepthImage;
    VkDeviceMemory DepthImageMemory;
    VkImageView DepthImageView;
};

struct view_projection_matrices
{
    glm::mat4 View;
    glm::mat4 Projection;
};

struct camera;
#define RENDERER_DRAW_FRAME(name) void name(vulkan_context *Renderer, \
                                            glm::vec2 RenderDim, \
                                            f32 DtFrame, \
                                            camera *Camera, \
                                            game_debug_info *GameDebugInfo)
typedef RENDERER_DRAW_FRAME(renderer_draw_frame);

internal void DrawDebugInfo(game_debug_info *GameDebugInfo);
