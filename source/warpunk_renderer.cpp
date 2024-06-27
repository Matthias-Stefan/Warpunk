/* TODO
 * Rewrite Renderlayer! 
 * 
 */

#include "warpunk_camera.h"
#include "warpunk_renderer.h"

#include "source/platform/platform.h"

#include <vector>

internal void
CreateInstance(vulkan_context *VulkanContext)
{
    VkApplicationInfo ApplicationInfo = {};
    ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationInfo.pApplicationName = "Warpunk";
    ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ApplicationInfo.pEngineName = "No Engine";
    ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    ApplicationInfo.apiVersion = VK_API_VERSION_1_3;
    
    const char* EnabledLayerNames[] = {
		"VK_LAYER_KHRONOS_validation",
	};
    
    u32 InstanceLayerPropertyCount;
    vkEnumerateInstanceLayerProperties(&InstanceLayerPropertyCount, 0);
    temporary_memory_block<VkLayerProperties> InstanceLayerProperties;
    StartTemporaryMemory(&InstanceLayerProperties, InstanceLayerPropertyCount, VulkanContext->PlatformAPI->AllocateMemory);
    vkEnumerateInstanceLayerProperties(&InstanceLayerPropertyCount, InstanceLayerProperties.Data);
    
    u32 ExtensionPropertyCount;
    vkEnumerateInstanceExtensionProperties(0, &ExtensionPropertyCount, 0);
    temporary_memory_block<VkExtensionProperties> ExtensionProperties;
    StartTemporaryMemory(&ExtensionProperties, ExtensionPropertyCount, VulkanContext->PlatformAPI->AllocateMemory);
    vkEnumerateInstanceExtensionProperties(0, &ExtensionPropertyCount, (VkExtensionProperties *)ExtensionProperties.Data);
    
    temporary_memory_block<const char *> ExtensionPropertyNames;
    StartTemporaryMemory(&ExtensionPropertyNames, ExtensionPropertyCount, VulkanContext->PlatformAPI->AllocateMemory);

    for (u32 ExtensionIndex = 0; 
         ExtensionIndex < ExtensionPropertyCount; 
         ++ExtensionIndex)
    {
        OutputDebugStringA(ExtensionProperties.Data[ExtensionIndex].extensionName);
        OutputDebugStringA("\n");
        
        ExtensionPropertyNames.Data[ExtensionIndex] = ExtensionProperties.Data[ExtensionIndex].extensionName;
    }
    
    VkInstanceCreateInfo InstanceCreateInfo = {};
    InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;
    InstanceCreateInfo.enabledLayerCount = ArraySize(EnabledLayerNames);
    InstanceCreateInfo.ppEnabledLayerNames = EnabledLayerNames;
    InstanceCreateInfo.enabledExtensionCount = ExtensionPropertyCount;
    InstanceCreateInfo.ppEnabledExtensionNames = ExtensionPropertyNames.Data;
    
    VkResult CreateInstanceResult = vkCreateInstance(&InstanceCreateInfo, 0, &VulkanContext->Instance); 

    EndTemporaryMemory(&InstanceLayerProperties, VulkanContext->PlatformAPI->DeallocateMemory);
    EndTemporaryMemory(&ExtensionProperties, VulkanContext->PlatformAPI->DeallocateMemory);
    EndTemporaryMemory(&ExtensionPropertyNames, VulkanContext->PlatformAPI->DeallocateMemory);
    if (UNSUCCESSFUL(CreateInstanceResult))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkInstance.");
    }
}

internal void
CreateSurface(vulkan_context *VulkanContext,
              HWND Window,
              HINSTANCE Instance)
{
    VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {};
    SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    SurfaceCreateInfo.hinstance = Instance;
    SurfaceCreateInfo.hwnd = Window;
    
    VkResult CreateWin32SurfaceResult = vkCreateWin32SurfaceKHR(VulkanContext->Instance, 
                                                                &SurfaceCreateInfo, 
                                                                0, 
                                                                &VulkanContext->Surface);
    if (UNSUCCESSFUL(CreateWin32SurfaceResult))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkSurfaceKHR.");
    }
}

internal void
SelectPhysicalDevice(vulkan_context *VulkanContext)
{
    VulkanContext->PhysicalDevice = VK_NULL_HANDLE;
    u32 DeviceCount = 0;
    vkEnumeratePhysicalDevices(VulkanContext->Instance, &DeviceCount, 0);
    
    if (DeviceCount == 0)
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to select a VkPhysicalDevice.");
    }
    
    temporary_memory_block<VkPhysicalDevice> PhysicalDevices;
    StartTemporaryMemory(&PhysicalDevices, DeviceCount, VulkanContext->PlatformAPI->AllocateMemory);
    vkEnumeratePhysicalDevices(VulkanContext->Instance, &DeviceCount, PhysicalDevices.Data);
    for (int DeviceIndex = 0; 
         DeviceIndex < DeviceCount; 
         ++DeviceIndex)
    {
        VkPhysicalDevice PhysicalDevice = PhysicalDevices.Data[DeviceIndex]; 
        
        VkPhysicalDeviceProperties PhysicalDeviceProperties = {}; 
        vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);
        
        if(PhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            VkPhysicalDeviceFeatures SupportedFeatures;
            vkGetPhysicalDeviceFeatures(PhysicalDevice, 
                                        &SupportedFeatures);
            if (!SupportedFeatures.samplerAnisotropy)
            {
                // TODO: Instead of enforcing the availability of anisotropic filtering, 
                // change settings!  
                continue;
            }
            
            VulkanContext->PhysicalDevice = PhysicalDevice;
            VulkanContext->PhysicalDeviceProperties = PhysicalDeviceProperties; 
            break;
        }
        else if(PhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            VulkanContext->PhysicalDevice = PhysicalDevice;
            VulkanContext->PhysicalDeviceProperties = PhysicalDeviceProperties;
        }
    }
    
    EndTemporaryMemory(&PhysicalDevices, VulkanContext->PlatformAPI->DeallocateMemory);
    if (VulkanContext->PhysicalDevice == VK_NULL_HANDLE)
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkPhysicalDevice.");
    }
}

inline void
FindQueueFamilies(vulkan_context *VulkanContext)
{
    u32 QueueFamiliesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(VulkanContext->PhysicalDevice, 
                                             &QueueFamiliesCount, 
                                             0);
    temporary_memory_block<VkQueueFamilyProperties> QueueFamilyProperties;
    StartTemporaryMemory(&QueueFamilyProperties, QueueFamiliesCount, VulkanContext->PlatformAPI->AllocateMemory);
    vkGetPhysicalDeviceQueueFamilyProperties(VulkanContext->PhysicalDevice, 
                                             &QueueFamiliesCount, 
                                             QueueFamilyProperties.Data);
    u32 GraphicsQueueIndex = 0;
    for(u32 QueueFamilyIndex = 0;
        QueueFamilyIndex < QueueFamiliesCount;
        ++QueueFamilyIndex)
    {
        VkQueueFamilyProperties QueueFamilyProperty = QueueFamilyProperties.Data[QueueFamilyIndex];
        if(QueueFamilyProperty.queueCount > 0)
        {
            if(QueueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                VulkanContext->GraphicsQueue.FamilyIndex = QueueFamilyIndex; 
            }
        }
    }

    EndTemporaryMemory(&QueueFamilyProperties, VulkanContext->PlatformAPI->DeallocateMemory);
}

internal void
CreateLogicalDevice(vulkan_context *VulkanContext,
                    u32 DeviceExtensionCount,
                    const char **DeviceExtensions)
{
    FindQueueFamilies(VulkanContext);
    u32 QueueCount = VulkanContext->QueueCount;
    
    temporary_memory_block<VkDeviceQueueCreateInfo> DeviceQueueCreateInfos;
    StartTemporaryMemory(&DeviceQueueCreateInfos, QueueCount, VulkanContext->PlatformAPI->AllocateMemory);

    f32 Priorities[] = { 1.0f };
    for (u32 DeviceQueueCreateInfosIndex = 0;
         DeviceQueueCreateInfosIndex < QueueCount;
         ++DeviceQueueCreateInfosIndex)
    {
        VkDeviceQueueCreateInfo DeviceQueueCreateInfo = {};
        DeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        DeviceQueueCreateInfo.queueFamilyIndex = VulkanContext->GraphicsQueue.FamilyIndex;
        DeviceQueueCreateInfo.queueCount = 1;
        DeviceQueueCreateInfo.pQueuePriorities = Priorities;
        
        DeviceQueueCreateInfos.Data[DeviceQueueCreateInfosIndex] = DeviceQueueCreateInfo;
    }
    
    VkPhysicalDeviceFeatures PhysicalDeviceFeatures = {};
    PhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    
    VkDeviceCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    CreateInfo.queueCreateInfoCount = QueueCount;
    CreateInfo.pQueueCreateInfos = DeviceQueueCreateInfos.Data;
    CreateInfo.enabledExtensionCount = DeviceExtensionCount;
    CreateInfo.ppEnabledExtensionNames = DeviceExtensions;
    CreateInfo.pEnabledFeatures = &PhysicalDeviceFeatures;
    
    VkResult CreateDeviceResult = vkCreateDevice(VulkanContext->PhysicalDevice,
                                                 &CreateInfo, 
                                                 0,
                                                 &VulkanContext->Device);
    
    EndTemporaryMemory(&DeviceQueueCreateInfos, VulkanContext->PlatformAPI->DeallocateMemory);
    if (UNSUCCESSFUL(CreateDeviceResult))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkDevice.");
    }
    
    vkGetDeviceQueue(VulkanContext->Device,
                     VulkanContext->GraphicsQueue.FamilyIndex,
                     0,
                     &VulkanContext->GraphicsQueue.Queue);
}

internal swap_chain_support_details
QuerySwapChainSupport(vulkan_context *VulkanContext)
{
    swap_chain_support_details SwapChainSupportDetails = {};
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanContext->PhysicalDevice, 
                                              VulkanContext->Surface, 
                                              &SwapChainSupportDetails.Capabilities);
    
    u32 FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanContext->PhysicalDevice, 
                                         VulkanContext->Surface,
                                         &FormatCount,
                                         0);
    if (FormatCount > 0)
    {
        SwapChainSupportDetails.FormatCount = FormatCount;
        SwapChainSupportDetails.Formats = VulkanContext->GraphicsMemoryBlock->PushArray<VkSurfaceFormatKHR>(FormatCount); 
        
        vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanContext->PhysicalDevice,
                                             VulkanContext->Surface,
                                             &FormatCount,
                                             SwapChainSupportDetails.Formats);
    }
    
    u32 PresentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanContext->PhysicalDevice,
                                              VulkanContext->Surface,
                                              &PresentModeCount,
                                              0);
    if (PresentModeCount > 0)
    {
        SwapChainSupportDetails.PresentModeCount = PresentModeCount;
        SwapChainSupportDetails.PresentModes = VulkanContext->GraphicsMemoryBlock->PushArray<VkPresentModeKHR>(PresentModeCount); 
        
        vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanContext->PhysicalDevice,
                                                  VulkanContext->Surface,
                                                  &PresentModeCount,
                                                  SwapChainSupportDetails.PresentModes);
    }
    
    return SwapChainSupportDetails;
}

internal VkSurfaceFormatKHR 
ChooseSwapSurfaceFormat(VkSurfaceFormatKHR *AvailableFormats, 
                        u32 FormatCount)
{
    for (u32 FormatIndex = 0;
         FormatIndex < FormatCount;
         ++FormatIndex)
    {
        VkSurfaceFormatKHR AvailableFormat = *AvailableFormats++;
        
        if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
        {
            return AvailableFormat;
        }
    }
    
    return AvailableFormats[0];
}

internal VkPresentModeKHR
ChooseSwapPresentMode(VkPresentModeKHR *AvailablePresentModes, 
                      u32 PresentModeCount)
{
    for (u32 PresentModeIndex = 0;
         PresentModeIndex< PresentModeCount;
         ++PresentModeIndex)
    {
        VkPresentModeKHR AvailablePresentMode = *AvailablePresentModes++; 
        
        if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
        {
            return AvailablePresentMode;
        }
    }
    
    return VK_PRESENT_MODE_FIFO_KHR;
}

inline void
GetFramebufferSize(HWND Window, 
                   s32 *Width, 
                   s32 *Height)
{
    RECT WindowRect, ClientRect;
    GetWindowRect(Window, &WindowRect);
    GetClientRect(Window, &ClientRect);
    
    s32 BorderWidth = (WindowRect.right - WindowRect.left) - ClientRect.right;
    s32 BorderHeight = (WindowRect.bottom - WindowRect.top) - ClientRect.bottom;
    *Width = ClientRect.right - BorderWidth;
    *Height = ClientRect.bottom - BorderHeight;
    
    HDC hDC = GetDC(Window);
    s32 ScaleFactorX = GetDeviceCaps(hDC, LOGPIXELSX) / 96.0f;
    s32 ScaleFactorY = GetDeviceCaps(hDC, LOGPIXELSY) / 96.0f;
    ReleaseDC(Window, hDC);
    *Width = (s32)((f32)*Width * ScaleFactorX);
    *Height = (s32)((f32)*Height * ScaleFactorY);
}

internal VkExtent2D 
ChooseSwapExtent(VkSurfaceCapabilitiesKHR Capabilities, 
                 HWND Window)
{
    VkExtent2D ActualExtent = {};
    if (Capabilities.currentExtent.width != MAX_U32)
    {
        return Capabilities.currentExtent;
    }
    else
    {
        s32 Width, Height;
        GetFramebufferSize(Window, &Width, &Height);
        ActualExtent = { (u32)Width, (u32)Height };
        
        ActualExtent.width = Clamp<s32>(ActualExtent.width,
                                        Capabilities.minImageExtent.width,
                                        Capabilities.maxImageExtent.width);
        ActualExtent.height = Clamp<s32>(ActualExtent.height,
                                         Capabilities.minImageExtent.height,
                                         Capabilities.maxImageExtent.height);
    }
    
    return ActualExtent;
}

internal void
CreateSwapChain(vulkan_context *VulkanContext,
                HWND Window)
{
    
    
    
    swap_chain_support_details SwapChainSupportDetails = QuerySwapChainSupport(VulkanContext);
    
    VkSurfaceFormatKHR SurfaceFormat = ChooseSwapSurfaceFormat(SwapChainSupportDetails.Formats,
                                                               SwapChainSupportDetails.FormatCount);
    VkPresentModeKHR PresentMode = ChooseSwapPresentMode(SwapChainSupportDetails.PresentModes,
                                                         SwapChainSupportDetails.PresentModeCount);
    VkExtent2D Extent = ChooseSwapExtent(SwapChainSupportDetails.Capabilities,
                                         Window);
    
    u32 ImageCount = SwapChainSupportDetails.Capabilities.minImageCount + 1;
    if (SwapChainSupportDetails.Capabilities.maxImageCount > 0 &&
        ImageCount > SwapChainSupportDetails.Capabilities.maxImageCount) 
    {
        ImageCount = SwapChainSupportDetails.Capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR SwapchainCreateInfo = {};
    SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapchainCreateInfo.surface = VulkanContext->Surface;
    
    SwapchainCreateInfo.minImageCount = ImageCount;
    SwapchainCreateInfo.imageFormat = SurfaceFormat.format;
    SwapchainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
    SwapchainCreateInfo.imageExtent = Extent;
    SwapchainCreateInfo.imageArrayLayers = 1;
    SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
#if 0
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
#endif
    
    SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    SwapchainCreateInfo.preTransform = SwapChainSupportDetails.Capabilities.currentTransform;
    SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCreateInfo.presentMode = PresentMode;
    SwapchainCreateInfo.clipped = VK_TRUE;
    
    SwapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    
    if (vkCreateSwapchainKHR(VulkanContext->Device, 
                             &SwapchainCreateInfo, 
                             0, 
                             &VulkanContext->SwapChain) == VK_SUCCESS) 
    {
        vkGetSwapchainImagesKHR(VulkanContext->Device, 
                                VulkanContext->SwapChain, 
                                &ImageCount, 
                                0);
        
        
        VulkanContext->SwapChainImages = VulkanContext->GraphicsMemoryBlock->PushArray<VkImage>(ImageCount);
        VulkanContext->SwapChainImagesCount = ImageCount;
        vkGetSwapchainImagesKHR(VulkanContext->Device, 
                                VulkanContext->SwapChain, 
                                &ImageCount, 
                                VulkanContext->SwapChainImages);
        
        VulkanContext->SwapChainImageFormat = SurfaceFormat.format;
        VulkanContext->SwapChainExtent = Extent;
    }
}

internal VkImageView
CreateImageView(vulkan_context *VulkanContext, 
                VkImage Image, 
                VkFormat Format,
                VkImageAspectFlags AspectFlags)
{
    VkImageViewCreateInfo ImageViewCreateInfo = {};
    
    ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.image = Image;
    ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.format = Format;
    
    ImageViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
    ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ImageViewCreateInfo.subresourceRange.levelCount = 1;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount = 1;
    
    VkImageView ImageView = {}; 
    if (UNSUCCESSFUL(vkCreateImageView(VulkanContext->Device,
                                       &ImageViewCreateInfo,
                                       0,
                                       &ImageView)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkImageView.");
    }
    
    return ImageView;
}

internal void
CreateImageViews(vulkan_context *VulkanContext)
{
    
    VulkanContext->SwapChainImageViews = 
        VulkanContext->GraphicsMemoryBlock->PushArray<VkImageView>(VulkanContext->SwapChainImagesCount);
    
    for (u32 ImageViewIndex = 0;
         ImageViewIndex < VulkanContext->SwapChainImagesCount;
         ++ImageViewIndex)
    {
        VulkanContext->SwapChainImageViews[ImageViewIndex] = 
            CreateImageView(VulkanContext,
                            VulkanContext->SwapChainImages[ImageViewIndex],
                            VulkanContext->SwapChainImageFormat,
                            VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

inline VkShaderModule
CreateShaderModule(vulkan_context *VulkanContext, platform_file_handle *ShaderCode)
{
    VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
    ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCreateInfo.codeSize = ShaderCode->Size;
    ShaderModuleCreateInfo.pCode = (u32 *)ShaderCode->Content;
    
    VkShaderModule ShaderModule = {};
    if (UNSUCCESSFUL(vkCreateShaderModule(VulkanContext->Device, 
                                          &ShaderModuleCreateInfo, 
                                          0, 
                                          &ShaderModule)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkShaderModule.");
    }
    
    
    return ShaderModule;
}

inline VkFormat
FindSupportedFormat(vulkan_context *VulkanContext,
                    VkFormat *Candidates,
                    u32 CandidatesCount,
                    VkImageTiling Tiling,
                    VkFormatFeatureFlags FeaturesFlags)
{
    VkFormat Format = {};
    for (u32 FormatIndex = 0;
         FormatIndex< CandidatesCount;
         ++FormatIndex)
    {
        VkFormat Format = Candidates[FormatIndex];
        
        VkFormatProperties FormatProperties = {};
        vkGetPhysicalDeviceFormatProperties(VulkanContext->PhysicalDevice,
                                            Format,
                                            &FormatProperties);
        if (Tiling == VK_IMAGE_TILING_LINEAR && 
            (FormatProperties.linearTilingFeatures & FeaturesFlags) == FeaturesFlags) 
        {
            return Format;
        } 
        else if (Tiling == VK_IMAGE_TILING_OPTIMAL && 
                 (FormatProperties.optimalTilingFeatures & FeaturesFlags) == FeaturesFlags) 
        {
            return Format;
        }
    }
    
    Win32ErrorMessage(PlatformError_Fatal, "Failed to find VkFormat.");
    return Format;
}

inline bool
HasStencilComponent(VkFormat Format)
{
    return Format == VK_FORMAT_D32_SFLOAT_S8_UINT || Format == VK_FORMAT_D24_UNORM_S8_UINT;
}


inline VkFormat
FindDepthFormat(vulkan_context *VulkanContext)
{
    u32 CandidatesCount = 3;
    VkFormat Candidates[] = { VK_FORMAT_D32_SFLOAT, 
        VK_FORMAT_D32_SFLOAT_S8_UINT, 
        VK_FORMAT_D24_UNORM_S8_UINT }; 
    
    return 
        FindSupportedFormat(VulkanContext,
                            Candidates,
                            3,
                            VK_IMAGE_TILING_OPTIMAL,
                            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}


internal void
CreateRenderPass(vulkan_context *VulkanContext)
{
    VkAttachmentDescription ColorAttachment = {};
    ColorAttachment.format = VulkanContext->SwapChainImageFormat;
    ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentDescription DepthAttachment = {}; 
    DepthAttachment.format = FindDepthFormat(VulkanContext);
    DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference ColorAttachmentReference = {};
    ColorAttachmentReference.attachment = 0;
    ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference DepthAttachmentReference = {};
    DepthAttachmentReference.attachment = 1;
    DepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription Subpass = {}; 
    Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount = 1;
    Subpass.pColorAttachments = &ColorAttachmentReference;
    Subpass.pDepthStencilAttachment = &DepthAttachmentReference;
    
    VkSubpassDependency SubpassDependency = {}; 
    SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    SubpassDependency.dstSubpass = 0;
    SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    SubpassDependency.srcAccessMask = 0;
    SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    VkAttachmentDescription Attachments[] = { ColorAttachment, DepthAttachment };
    
    VkRenderPassCreateInfo RenderPassCreateInfo = {};
    RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCreateInfo.attachmentCount = 2;
    RenderPassCreateInfo.pAttachments = Attachments;
    RenderPassCreateInfo.subpassCount = 1;
    RenderPassCreateInfo.pSubpasses = &Subpass;
    RenderPassCreateInfo.dependencyCount = 1;
    RenderPassCreateInfo.pDependencies = &SubpassDependency;
    
    if (UNSUCCESSFUL(vkCreateRenderPass(VulkanContext->Device, 
                                        &RenderPassCreateInfo, 
                                        0, 
                                        &VulkanContext->Renderpass))) 
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkRenderPass.");
    }
}

inline void
DEBUGGetVertices(vulkan_context *VulkanContext)
{
    u32 VertexCount = 8;
    
    
    VulkanContext->Vertices = 
        VulkanContext->GraphicsMemoryBlock->PushArray<vertex>(VertexCount);
    
    VulkanContext->VerticesSize = sizeof(vertex) * VertexCount;
    VulkanContext->VertexCount = VertexCount;
    

    VulkanContext->Vertices[0].Pos = { -0.5f, -0.5f, 0.0f };
    VulkanContext->Vertices[0].Color = { 1.0f, 0.0f, 0.0f, 1.0f };
    VulkanContext->Vertices[0].TexCoord0 = { 1.0f, 0.0f };
    
    VulkanContext->Vertices[1].Pos = { 0.5f, -0.5f, 0.0f };
    VulkanContext->Vertices[1].Color = { 0.0f, 1.0f, 0.0f, 1.0f };
    VulkanContext->Vertices[1].TexCoord0 = { 0.0f, 0.0f };
    
    VulkanContext->Vertices[2].Pos = { 0.5f, 0.5f, 0.0f };
    VulkanContext->Vertices[2].Color = { 0.0f, 0.0f, 1.0f, 1.0f };
    VulkanContext->Vertices[2].TexCoord0 = { 0.0f, 1.0f };
    
    VulkanContext->Vertices[3].Pos = { -0.5f, 0.5f, 0.0f };
    VulkanContext->Vertices[3].Color = { 0.0f, 1.0f, 1.0f, 1.0f };
    VulkanContext->Vertices[3].TexCoord0 = { 1.0f, 1.0f };
    
    VulkanContext->Vertices[4].Pos = { -0.5f, -0.5f, -0.5f };
    VulkanContext->Vertices[4].Color = { 1.0f, 0.0f, 0.0f, 1.0f };
    VulkanContext->Vertices[4].TexCoord0 = { 0.0f, 0.0f };
    
    VulkanContext->Vertices[5].Pos = { 0.5f, -0.5f, -0.5f };
    VulkanContext->Vertices[5].Color = { 0.0f, 1.0f, 0.0f, 1.0f };
    VulkanContext->Vertices[5].TexCoord0 = { 1.0f, 0.0f };
    
    VulkanContext->Vertices[6].Pos = { 0.5f, 0.5f, -0.5f };
    VulkanContext->Vertices[6].Color = { 0.0f, 0.0f, 1.0f, 1.0f };
    VulkanContext->Vertices[6].TexCoord0 = { 1.0f, 1.0f };
    
    VulkanContext->Vertices[7].Pos = { -0.5f, 0.5f, -0.5f };
    VulkanContext->Vertices[7].Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    VulkanContext->Vertices[7].TexCoord0 = { 0.0f, 1.0f };
    
    u32 IndexCount = 12;
    
    
    VulkanContext->Indices = VulkanContext->GraphicsMemoryBlock->PushArray<u32>(IndexCount);
    VulkanContext->IndicesSize = sizeof(u32) * IndexCount;
    VulkanContext->IndexCount = IndexCount;
    
    VulkanContext->Indices[0] = 0;
    VulkanContext->Indices[1] = 1;
    VulkanContext->Indices[2] = 2;
    VulkanContext->Indices[3] = 2;
    VulkanContext->Indices[4] = 3;
    VulkanContext->Indices[5] = 0;
    
    VulkanContext->Indices[6] = 4;
    VulkanContext->Indices[7] = 5;
    VulkanContext->Indices[8] = 6;
    VulkanContext->Indices[9] = 6;
    VulkanContext->Indices[10] = 7;
    VulkanContext->Indices[11] = 4;
}

internal void
CreateDescriptorSetLayout(vulkan_context *VulkanContext)
{
    VkDescriptorSetLayoutBinding UniformBufferObjectLayoutBinding = {};
    UniformBufferObjectLayoutBinding.binding = 0;
    UniformBufferObjectLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UniformBufferObjectLayoutBinding.descriptorCount = 1;
    UniformBufferObjectLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    UniformBufferObjectLayoutBinding.pImmutableSamplers = 0;
    
    VkDescriptorSetLayoutBinding SamplerLayoutBinding = {};
    SamplerLayoutBinding.binding = 1;
    SamplerLayoutBinding.descriptorCount = 1;
    SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    SamplerLayoutBinding.pImmutableSamplers = 0;
    SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutBinding Bindings[2] = { UniformBufferObjectLayoutBinding,
        SamplerLayoutBinding };
    
    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {};
    DescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorSetLayoutCreateInfo.bindingCount = ArraySize(Bindings);
    DescriptorSetLayoutCreateInfo.pBindings = Bindings;
    
    if (UNSUCCESSFUL(vkCreateDescriptorSetLayout(VulkanContext->Device,
                                                 &DescriptorSetLayoutCreateInfo,
                                                 0,
                                                 &VulkanContext->DescriptorSetLayout)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkDescriptorSetLayout.");
    }
}

internal void
CreateGraphicsPipeline(vulkan_context *VulkanContext)
{
    platform_file_handle VertexShaderCode = {}; 
    Win32ReadFileBinary("W:/Warpunk/shader/vert.spv", &VertexShaderCode);
    platform_file_handle FragmentShaderCode = {}; 
    Win32ReadFileBinary("W:/Warpunk/shader/frag.spv", &FragmentShaderCode);
    
    VkShaderModule VertexShaderModule = CreateShaderModule(VulkanContext, &VertexShaderCode); 
    VkShaderModule FragmentShaderModule = CreateShaderModule(VulkanContext, &FragmentShaderCode);
    
    VkPipelineShaderStageCreateInfo VertexPipelineShaderStageCreateInfo = {};
    VertexPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VertexPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VertexPipelineShaderStageCreateInfo.module = VertexShaderModule;
    VertexPipelineShaderStageCreateInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo FragmentPipelineShaderStageCreateInfo = {};
    FragmentPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    FragmentPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragmentPipelineShaderStageCreateInfo.module = FragmentShaderModule;
    FragmentPipelineShaderStageCreateInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo ShaderStages[] = {
        VertexPipelineShaderStageCreateInfo,
        FragmentPipelineShaderStageCreateInfo
    };
    
    // NOTE(matthias): Vertex input
    VkPipelineVertexInputStateCreateInfo VertexInput = {};
    VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    VkVertexInputBindingDescription VertexInputBindingDescription 
        = GetVertexInputBindingDescription();
    u32 AttributeDescriptionCount = 0;
    VkVertexInputAttributeDescription *VertexInputAttributeDescription 
        = GetVertexInputAttributeDescription(&AttributeDescriptionCount);
    
    VertexInput.vertexBindingDescriptionCount = 1;
    VertexInput.vertexAttributeDescriptionCount = AttributeDescriptionCount;
    VertexInput.pVertexBindingDescriptions = &VertexInputBindingDescription;
    VertexInput.pVertexAttributeDescriptions = VertexInputAttributeDescription;
    
    // NOTE(matthias): Input assembly
    VkPipelineInputAssemblyStateCreateInfo InputAssembly = {};
    InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // NOTE(matthias): Viewports and scissors
    VkPipelineViewportStateCreateInfo ViewportState = {};
    ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount = 1;
    ViewportState.scissorCount = 1;
    
    // NOTE(matthias): Rasterizer
    VkPipelineRasterizationStateCreateInfo Rasterizer = {};
    Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    Rasterizer.depthClampEnable = VK_FALSE;
    Rasterizer.rasterizerDiscardEnable = VK_FALSE;
    Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    Rasterizer.lineWidth = 1.0f;
    Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    Rasterizer.depthBiasEnable = VK_FALSE;
    
    // NOTE(matthias): Multisampling
    VkPipelineMultisampleStateCreateInfo Multisampling = {};
    Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    Multisampling.sampleShadingEnable = VK_FALSE;
    Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // NOTE(matthias): Depth and stencil testing
    VkPipelineDepthStencilStateCreateInfo DepthStencilState = {};
    DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilState.depthTestEnable = VK_TRUE;
    DepthStencilState.depthWriteEnable = VK_TRUE;
    DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    DepthStencilState.stencilTestEnable = VK_FALSE;
    
    // NOTE(matthias): Color blending
    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;
    ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
    ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; 
    ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; 
    
    VkPipelineColorBlendStateCreateInfo ColorBlending = {};
    ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlending.logicOpEnable = VK_FALSE;
    ColorBlending.logicOp = VK_LOGIC_OP_COPY; 
    ColorBlending.attachmentCount = 1;
    ColorBlending.pAttachments = &ColorBlendAttachment;
    ColorBlending.blendConstants[0] = 0.0f;
    ColorBlending.blendConstants[1] = 0.0f;
    ColorBlending.blendConstants[2] = 0.0f;
    ColorBlending.blendConstants[3] = 0.0f;
    
    // NOTE(matthias): Dynamic State
    VkDynamicState DynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    u32 DynamicStateCount = ArraySize(DynamicStates);
    
    VkPipelineDynamicStateCreateInfo DynamicState = {};
    DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.dynamicStateCount = DynamicStateCount;
    DynamicState.pDynamicStates = DynamicStates;
    
    // NOTE(matthias): Pipeline layout
    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutInfo.setLayoutCount = 1;
    PipelineLayoutInfo.pSetLayouts = &VulkanContext->DescriptorSetLayout;
    PipelineLayoutInfo.pushConstantRangeCount = 0;
    PipelineLayoutInfo.pPushConstantRanges = 0;
    
    
    if (UNSUCCESSFUL(vkCreatePipelineLayout(VulkanContext->Device, 
                                            &PipelineLayoutInfo, 
                                            0, 
                                            &VulkanContext->PipelineLayout)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkPipelineLayout.");
    }
    
    VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {}; 
    GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    GraphicsPipelineCreateInfo.stageCount = 2;
    GraphicsPipelineCreateInfo.pStages = ShaderStages;
    
    GraphicsPipelineCreateInfo.pVertexInputState = &VertexInput;
    GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssembly;
    GraphicsPipelineCreateInfo.pViewportState = &ViewportState;
    GraphicsPipelineCreateInfo.pRasterizationState = &Rasterizer;
    GraphicsPipelineCreateInfo.pMultisampleState = &Multisampling;
    GraphicsPipelineCreateInfo.pDepthStencilState = &DepthStencilState; 
    GraphicsPipelineCreateInfo.pColorBlendState = &ColorBlending;
    GraphicsPipelineCreateInfo.pDynamicState = &DynamicState;
    
    GraphicsPipelineCreateInfo.layout = VulkanContext->PipelineLayout;
    GraphicsPipelineCreateInfo.renderPass = VulkanContext->Renderpass;
    GraphicsPipelineCreateInfo.subpass = 0;
    
    GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; 
    GraphicsPipelineCreateInfo.basePipelineIndex = -1; 
    if (UNSUCCESSFUL(vkCreateGraphicsPipelines(VulkanContext->Device, 
                                               VK_NULL_HANDLE, 
                                               1, 
                                               &GraphicsPipelineCreateInfo, 
                                               0, 
                                               &VulkanContext->GraphicsPipeline))) 
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkPipeline.\n");
    }
    
    vkDestroyShaderModule(VulkanContext->Device, FragmentShaderModule, 0);
    VirtualFree(FragmentShaderCode.Content, 0, MEM_RELEASE);
    vkDestroyShaderModule(VulkanContext->Device, VertexShaderModule, 0);
    VirtualFree(VertexShaderCode.Content, 0, MEM_RELEASE);
}

internal void
CreateFramebuffers(vulkan_context *VulkanContext)
{
    VulkanContext->SwapChainFramebuffers = 
        VulkanContext->GraphicsMemoryBlock->PushArray<VkFramebuffer>(VulkanContext->SwapChainImagesCount);
    
    VkFramebuffer* CurrentSwapChainFramebuffer = VulkanContext->SwapChainFramebuffers;
    
    for (u32 ImageViewIndex = 0; ImageViewIndex < VulkanContext->SwapChainImagesCount; ++ImageViewIndex)
    {
        VkImageView Attachments[] = { VulkanContext->SwapChainImageViews[ImageViewIndex], 
            VulkanContext->DepthImageView };
        
        VkFramebufferCreateInfo FramebufferCreateInfo = {};
        FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferCreateInfo.renderPass = VulkanContext->Renderpass;
        FramebufferCreateInfo.attachmentCount = ArraySize(Attachments);
        FramebufferCreateInfo.pAttachments = Attachments;
        FramebufferCreateInfo.width = VulkanContext->SwapChainExtent.width;
        FramebufferCreateInfo.height = VulkanContext->SwapChainExtent.height;
        FramebufferCreateInfo.layers = 1;
        
        if (UNSUCCESSFUL(vkCreateFramebuffer(VulkanContext->Device, 
                                             &FramebufferCreateInfo, 
                                             0, 
                                             &CurrentSwapChainFramebuffer[ImageViewIndex])))
        {
            Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkFramebuffer.");
        }
    }
}

inline void
CleanupSwapChain(vulkan_context *VulkanContext)
{
    vkDestroyImageView(VulkanContext->Device, 
                       VulkanContext->DepthImageView, 0);
    vkDestroyImage(VulkanContext->Device, 
                   VulkanContext->DepthImage, 0);
    vkFreeMemory(VulkanContext->Device, 
                 VulkanContext->DepthImageMemory, 0);
    
    for (u32 FramebufferIndex = 0;
         FramebufferIndex< ArraySize(VulkanContext->SwapChainFramebuffers);
         ++FramebufferIndex)
    {
        vkDestroyFramebuffer(VulkanContext->Device, 
                             VulkanContext->SwapChainFramebuffers[FramebufferIndex],
                             0);
    }
    
    for (u32 ImageViewIndex = 0;
         ImageViewIndex< ArraySize(VulkanContext->SwapChainImageViews);
         ++ImageViewIndex)
    {
        vkDestroyImageView(VulkanContext->Device, 
                           VulkanContext->SwapChainImageViews[ImageViewIndex],
                           0);
    }
    
    vkDestroySwapchainKHR(VulkanContext->Device,
                          VulkanContext->SwapChain,
                          0);
}

internal void
CreateCommandPool(vulkan_context *VulkanContext)
{
    VkCommandPoolCreateInfo CommandPoolCreateInfo = {};
    CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    CommandPoolCreateInfo.queueFamilyIndex = VulkanContext->GraphicsQueue.FamilyIndex;
    
    if (UNSUCCESSFUL(vkCreateCommandPool(VulkanContext->Device,
                                         &CommandPoolCreateInfo,
                                         0,
                                         &VulkanContext->CommandPool)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkCommandPool.");
    }
}

inline u32 
FindMemoryType(u32 TypeFilter, 
               VkMemoryPropertyFlags MemoryPropertyFlags,
               vulkan_context *VulkanContext) 
{
    VkPhysicalDeviceMemoryProperties MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(VulkanContext->PhysicalDevice, 
                                        &MemoryProperties);
    
    for (u32 Index = 0; 
         Index < MemoryProperties.memoryTypeCount; 
         ++Index) 
    {
        if ((TypeFilter & (1 << Index)) && 
            (MemoryProperties.memoryTypes[Index].propertyFlags & MemoryPropertyFlags) == MemoryPropertyFlags) 
        {
            return Index;
        }
    }
    
    return -1;
}

inline void
CreateBuffer(vulkan_context *VulkanContext,
             VkDeviceSize DeviceSize,
             VkBufferUsageFlags BufferUsageFlags,
             VkMemoryPropertyFlags MemoryPropertyFlags,
             VkBuffer *Buffer,
             VkDeviceMemory *DeviceMemory)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size = DeviceSize;
    BufferCreateInfo.usage = BufferUsageFlags;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (UNSUCCESSFUL(vkCreateBuffer(VulkanContext->Device,
                                    &BufferCreateInfo,
                                    0,
                                    Buffer)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkBuffer.");
    }
    
    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(VulkanContext->Device,
                                  *Buffer,
                                  &MemoryRequirements);
    
    VkMemoryAllocateInfo MemoryAllocateInfo = {};
    MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = FindMemoryType(MemoryRequirements.memoryTypeBits, 
                                                        MemoryPropertyFlags,
                                                        VulkanContext);
    
    if (UNSUCCESSFUL(vkAllocateMemory(VulkanContext->Device, 
                                      &MemoryAllocateInfo, 
                                      0, 
                                      DeviceMemory))) 
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to invoke vkAllocateMemory.");
    }
    
    vkBindBufferMemory(VulkanContext->Device, 
                       *Buffer, 
                       *DeviceMemory, 
                       0);
}

inline void
CopyVkBuffer(vulkan_context *VulkanContext,
             VkBuffer *Source, 
             VkBuffer *Destination, 
             size_t Size)
{
    VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
    CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocateInfo.commandPool = VulkanContext->CommandPool;
    CommandBufferAllocateInfo.commandBufferCount = 1;
    
    VkCommandBuffer CommandBuffer;
    vkAllocateCommandBuffers(VulkanContext->Device, 
                             &CommandBufferAllocateInfo, 
                             &CommandBuffer);
    
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(CommandBuffer, 
                         &CommandBufferBeginInfo);
    
    VkBufferCopy BufferCopy = {};
    BufferCopy.size = Size;
    vkCmdCopyBuffer(CommandBuffer, 
                    *Source, 
                    *Destination, 
                    1, 
                    &BufferCopy);
    
    vkEndCommandBuffer(CommandBuffer);
    
    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer;
    
    vkQueueSubmit(VulkanContext->GraphicsQueue.Queue, 
                  1, 
                  &SubmitInfo, 
                  VK_NULL_HANDLE);
    vkQueueWaitIdle(VulkanContext->GraphicsQueue.Queue);
    
    vkFreeCommandBuffers(VulkanContext->Device, 
                         VulkanContext->CommandPool, 
                         1, 
                         &CommandBuffer);
}

internal void
CreateVertexBuffer(vulkan_context *VulkanContext)
{
    VkDeviceSize DeviceSize = VulkanContext->VerticesSize;
    
    VkBuffer StagingBuffer = {};
    VkDeviceMemory StagingBufferMemory;
    CreateBuffer(VulkanContext,
                 DeviceSize, 
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 &StagingBuffer, 
                 &StagingBufferMemory);
    
    void* Data;
    vkMapMemory(VulkanContext->Device, 
                StagingBufferMemory, 
                0, 
                DeviceSize, 
                0, 
                &Data);
    CopyArray(VulkanContext->Vertices, 
              Data,
              DeviceSize);
    vkUnmapMemory(VulkanContext->Device, 
                  StagingBufferMemory);
    
    CreateBuffer(VulkanContext,
                 DeviceSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 &VulkanContext->VertexBuffer, 
                 &VulkanContext->VertexBufferMemory);
    
    CopyVkBuffer(VulkanContext, &StagingBuffer, &VulkanContext->VertexBuffer, DeviceSize);
    vkDestroyBuffer(VulkanContext->Device, StagingBuffer, 0);
    vkFreeMemory(VulkanContext->Device, StagingBufferMemory, 0);
}

internal void
CreateIndexBuffer(vulkan_context *VulkanContext)
{
    VkDeviceSize DeviceSize = VulkanContext->IndicesSize;
    
    VkBuffer StagingBuffer = {};
    VkDeviceMemory StagingBufferMemory;
    CreateBuffer(VulkanContext,
                 DeviceSize, 
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 &StagingBuffer, 
                 &StagingBufferMemory);
    
    void* Data;
    vkMapMemory(VulkanContext->Device, 
                StagingBufferMemory, 
                0, 
                DeviceSize, 
                0, 
                &Data);
    CopyArray(VulkanContext->Indices, 
              Data,
              DeviceSize);
    vkUnmapMemory(VulkanContext->Device, 
                  StagingBufferMemory);
    
    CreateBuffer(VulkanContext,
                 DeviceSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                 &VulkanContext->IndexBuffer, 
                 &VulkanContext->IndexBufferMemory);
    
    CopyVkBuffer(VulkanContext, &StagingBuffer, &VulkanContext->IndexBuffer, DeviceSize);
    vkDestroyBuffer(VulkanContext->Device, StagingBuffer, 0);
    vkFreeMemory(VulkanContext->Device, StagingBufferMemory, 0);
}

internal void
CreateCommandBuffers(vulkan_context *VulkanContext)
{
    VulkanContext->CommandBuffers = 
        VulkanContext->GraphicsMemoryBlock->PushArray<VkCommandBuffer>(MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
    CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocateInfo.commandPool = VulkanContext->CommandPool;
    CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    
    if (UNSUCCESSFUL(vkAllocateCommandBuffers(VulkanContext->Device,
                                              &CommandBufferAllocateInfo,
                                              VulkanContext->CommandBuffers))) 
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkCommandBuffer.");
    }
}

internal void
RecordCommandBuffer(vulkan_context *VulkanContext,
                    VkCommandBuffer &CommandBuffer,
                    u32 ImageIndex,
                    u32 CurrentFrame,
                    game_debug_info *GameDebugInfo)
{
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    if (UNSUCCESSFUL(vkBeginCommandBuffer(CommandBuffer,
                                          &CommandBufferBeginInfo)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to invoke vkBeginCommandBuffer.");
    }
    
    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = VulkanContext->Renderpass;
    RenderPassBeginInfo.framebuffer = VulkanContext->SwapChainFramebuffers[ImageIndex];
    RenderPassBeginInfo.renderArea.offset = {0, 0};
    RenderPassBeginInfo.renderArea.extent = VulkanContext->SwapChainExtent;
    
    VkClearValue ClearValues[2] = {};
    ClearValues[0].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    ClearValues[1].depthStencil = { 1.0f, 0 };
    
    RenderPassBeginInfo.clearValueCount = ArraySize(ClearValues);
    RenderPassBeginInfo.pClearValues = ClearValues;
    
    vkCmdBeginRenderPass(CommandBuffer, 
                         &RenderPassBeginInfo, 
                         VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(CommandBuffer, 
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      VulkanContext->GraphicsPipeline);
    VkViewport Viewport = {};
    Viewport.x = 0.0f;
    Viewport.y = 0.0f;
    Viewport.width = (f32)VulkanContext->SwapChainExtent.width;
    Viewport.height = (f32)VulkanContext->SwapChainExtent.height;
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;
    vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);
    
    VkRect2D Scissor = {};
    Scissor.offset = {0, 0};
    Scissor.extent = VulkanContext->SwapChainExtent;
    vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);
    
#if WARPUNK_DEBUG
    DrawDebugInfo(GameDebugInfo);
#endif
    
    VkBuffer VertexBuffers[] = { VulkanContext->VertexBuffer };
    VkDeviceSize Offsets[] = { 0 };
    vkCmdBindVertexBuffers(CommandBuffer, 
                           0, 
                           1, 
                           &VulkanContext->VertexBuffer, 
                           Offsets);
    vkCmdBindIndexBuffer(CommandBuffer, 
                         VulkanContext->IndexBuffer, 
                         0, 
                         VK_INDEX_TYPE_UINT32);
    
    vkCmdBindDescriptorSets(CommandBuffer, 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            VulkanContext->PipelineLayout, 
                            0, 
                            1, 
                            &VulkanContext->DescriptorSets[CurrentFrame], 
                            0, 
                            0);
    
    vkCmdDrawIndexed(CommandBuffer, VulkanContext->IndexCount, 1, 0, 0, 0);
    
#if WARPUNK_DEBUG
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VulkanContext->CommandBuffers[CurrentFrame]);
#endif
    vkCmdEndRenderPass(CommandBuffer);
    
    if (UNSUCCESSFUL(vkEndCommandBuffer(CommandBuffer)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to invoke vkEndCommandBuffer.");
    }
}

internal void
CreateSyncObjects(vulkan_context *VulkanContext)
{
    VulkanContext->ImageAvailableSemaphores = 
        VulkanContext->GraphicsMemoryBlock->PushArray<VkSemaphore>(MAX_FRAMES_IN_FLIGHT);
    
    VulkanContext->RenderFinishedSemaphores = 
        VulkanContext->GraphicsMemoryBlock->PushArray<VkSemaphore>(MAX_FRAMES_IN_FLIGHT);
    
    VulkanContext->InFlightFences = 
        VulkanContext->GraphicsMemoryBlock->PushArray<VkFence>(MAX_FRAMES_IN_FLIGHT);
    
    VkSemaphoreCreateInfo SemaphoreCreateInfo = {}; 
    SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo FenceCreateInfo = {};
    FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (u32 Index = 0; 
         Index < MAX_FRAMES_IN_FLIGHT; 
         ++Index)
    {
        if (UNSUCCESSFUL(vkCreateSemaphore(VulkanContext->Device, 
                                           &SemaphoreCreateInfo, 
                                           0, 
                                           &VulkanContext->ImageAvailableSemaphores[Index])) ||
            UNSUCCESSFUL(vkCreateSemaphore(VulkanContext->Device, 
                                           &SemaphoreCreateInfo, 
                                           0, 
                                           &VulkanContext->RenderFinishedSemaphores[Index])) ||
            UNSUCCESSFUL(vkCreateFence(VulkanContext->Device, 
                                       &FenceCreateInfo, 
                                       0, 
                                       &VulkanContext->InFlightFences[Index])))
        {
            Win32ErrorMessage(PlatformError_Fatal, "Failed to create synchronization object for a frame.");
        }
    }
}

internal void
CreateUniformBuffers(vulkan_context *VulkanContext)
{
    VkDeviceSize DeviceSize = sizeof(uniform_buffer_object);
    
    VulkanContext->UniformBuffers =
        VulkanContext->GraphicsMemoryBlock->PushArray<VkBuffer>(MAX_FRAMES_IN_FLIGHT);
    
    VulkanContext->UniformBuffersMemory = 
        VulkanContext->GraphicsMemoryBlock->PushArray<VkDeviceMemory>(MAX_FRAMES_IN_FLIGHT);
    
    VulkanContext->UniformBuffersMapped = 
        VulkanContext->GraphicsMemoryBlock->PushArray<void *>(MAX_FRAMES_IN_FLIGHT);
    
    for (u32 PushIndex = 0;
         PushIndex < MAX_FRAMES_IN_FLIGHT;
         ++PushIndex)
    {
        CreateBuffer(VulkanContext,
                     DeviceSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &VulkanContext->UniformBuffers[PushIndex],
                     &VulkanContext->UniformBuffersMemory[PushIndex]);
        
        void** pUniformBuffersMapped = (void **)(&VulkanContext->UniformBuffersMapped[PushIndex]);
        vkMapMemory(VulkanContext->Device, 
                    VulkanContext->UniformBuffersMemory[PushIndex], 
                    0, 
                    DeviceSize, 
                    0, 
                    pUniformBuffersMapped);
    }
}

internal void
CreateDescriptorPool(vulkan_context *VulkanContext)
{
    VkDescriptorPoolSize PoolSizes[2] = {}; 
    PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    PoolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    PoolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = ArraySize(PoolSizes);
    PoolInfo.pPoolSizes = PoolSizes;
    PoolInfo.maxSets = (u32)MAX_FRAMES_IN_FLIGHT;
    
    if (UNSUCCESSFUL(vkCreateDescriptorPool(VulkanContext->Device, 
                                            &PoolInfo, 
                                            0, 
                                            &VulkanContext->DescriptorPool)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkDescriptorPool.");
    }
}

internal void 
CreateDescriptorSets(vulkan_context *VulkanContext) 
{
    VkDescriptorSetLayout Layouts[MAX_FRAMES_IN_FLIGHT];
    for (u32 LayoutIndex = 0; LayoutIndex < MAX_FRAMES_IN_FLIGHT; ++LayoutIndex)
    {
        Layouts[LayoutIndex] = VulkanContext->DescriptorSetLayout;
    }
    
    VkDescriptorSetAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocInfo.descriptorPool = VulkanContext->DescriptorPool;
    AllocInfo.descriptorSetCount = (u32)MAX_FRAMES_IN_FLIGHT;
    AllocInfo.pSetLayouts = Layouts;
    
    VulkanContext->DescriptorSets = 
        VulkanContext->GraphicsMemoryBlock->PushArray<VkDescriptorSet>(MAX_FRAMES_IN_FLIGHT);
    
    if (UNSUCCESSFUL(vkAllocateDescriptorSets(VulkanContext->Device, 
                                              &AllocInfo, 
                                              VulkanContext->DescriptorSets))) 
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkDescriptorSet.");
    }
    
    for (size_t UpdateIndex = 0; 
         UpdateIndex < MAX_FRAMES_IN_FLIGHT; 
         ++UpdateIndex) 
    {
        VkDescriptorBufferInfo BufferInfo = {};
        BufferInfo.buffer = VulkanContext->UniformBuffers[UpdateIndex];
        BufferInfo.offset = 0;
        BufferInfo.range = sizeof(uniform_buffer_object);
        
        VkDescriptorImageInfo ImageInfo = {};
        ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ImageInfo.imageView = VulkanContext->TextureImageView;
        ImageInfo.sampler = VulkanContext->TextureSampler;
        
        VkWriteDescriptorSet DescriptorWrites[2] = {};
        
        DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[0].dstSet = VulkanContext->DescriptorSets[UpdateIndex];
        DescriptorWrites[0].dstBinding = 0;
        DescriptorWrites[0].dstArrayElement = 0;
        DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescriptorWrites[0].descriptorCount = 1;
        DescriptorWrites[0].pBufferInfo = &BufferInfo;
        
        DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[1].dstSet = VulkanContext->DescriptorSets[UpdateIndex];
        DescriptorWrites[1].dstBinding = 1;
        DescriptorWrites[1].dstArrayElement = 0;
        DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorWrites[1].descriptorCount = 1;
        DescriptorWrites[1].pImageInfo = &ImageInfo;
        
        vkUpdateDescriptorSets(VulkanContext->Device, 2, DescriptorWrites, 0, 0);
    }
}

internal void 
UpdateUniformBuffer(vulkan_context *VulkanContext,
                    u32 CurrentImage,
                    camera *Camera)
{
    uniform_buffer_object UniformBufferObject = {};
    
    UniformBufferObject.Model = glm::rotate(glm::mat4(1.0f), 
                                            glm::radians(10.0f),
                                            glm::vec3(0.0f, 1.0f, 0.0f));
    
    UniformBufferObject.View = glm::lookAt(Camera->Pos,
                                           Camera->Pos + Camera->Dir, 
                                           Camera->Up);


    f32 AspectRatio = VulkanContext->SwapChainExtent.width / (f32)VulkanContext->SwapChainExtent.height;
    UniformBufferObject.Projection = glm::perspective(ToRadian(45.0f), AspectRatio, 0.1f, 100.0f);
    UniformBufferObject.Projection[1][1] *= -1;
    
    memcpy(VulkanContext->UniformBuffersMapped[CurrentImage], 
           &UniformBufferObject, 
           sizeof(UniformBufferObject));
}

internal void 
CreateImage(vulkan_context *VulkanContext,
            u32 Width,
            u32 Height,
            VkFormat Format,
            VkImageTiling Tiling,
            VkImageUsageFlags UsageFlags,
            VkMemoryPropertyFlags PropertyFlags,
            VkImage *Image,
            VkDeviceMemory *ImageMemory)
{
    VkImageCreateInfo ImageCreateInfo = {}; 
    ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.format = Format;
    ImageCreateInfo.extent.width = Width;
    ImageCreateInfo.extent.height = Height;
    ImageCreateInfo.extent.depth = 1;
    ImageCreateInfo.mipLevels = 1;
    ImageCreateInfo.arrayLayers = 1;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling = Tiling;
    ImageCreateInfo.usage = UsageFlags;
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    if (UNSUCCESSFUL(vkCreateImage(VulkanContext->Device,
                                   &ImageCreateInfo,
                                   0,
                                   Image)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkImage.");
    }
    
    VkMemoryRequirements MemoryRequirements = {};
    vkGetImageMemoryRequirements(VulkanContext->Device,
                                 *Image,
                                 &MemoryRequirements);
    
    VkMemoryAllocateInfo MemoryAllocateInfo = {};
    MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = FindMemoryType(MemoryRequirements.memoryTypeBits,
                                                        PropertyFlags,
                                                        VulkanContext);
    
    if (UNSUCCESSFUL(vkAllocateMemory(VulkanContext->Device, 
                                      &MemoryAllocateInfo, 
                                      0, 
                                      ImageMemory)))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to allocate VkDeviceMemory.");
    }
    
    vkBindImageMemory(VulkanContext->Device, 
                      *Image,
                      *ImageMemory, 
                      0);
}

internal VkCommandBuffer
BeginSingleTimeCommands(vulkan_context *VulkanContext)
{
    VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
    CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocateInfo.commandPool = VulkanContext->CommandPool;
    CommandBufferAllocateInfo.commandBufferCount = 1;
    
    VkCommandBuffer CommandBuffer = {};
    vkAllocateCommandBuffers(VulkanContext->Device, 
                             &CommandBufferAllocateInfo, 
                             &CommandBuffer);
    
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo);
    
    return CommandBuffer;
}

internal void
EndSingleTimeCommands(vulkan_context *VulkanContext,
                      VkCommandBuffer CommandBuffer)
{
    vkEndCommandBuffer(CommandBuffer);
    
    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CommandBuffer;
    
    vkQueueSubmit(VulkanContext->GraphicsQueue.Queue, 
                  1,
                  &SubmitInfo,
                  VK_NULL_HANDLE);
    vkQueueWaitIdle(VulkanContext->GraphicsQueue.Queue);
    vkFreeCommandBuffers(VulkanContext->Device,
                         VulkanContext->CommandPool,
                         1,
                         &CommandBuffer);
}

internal void
CopyBuffer(vulkan_context *VulkanContext,
           VkBuffer SrcBuffer, 
           VkBuffer DstBuffer,
           VkDeviceSize Size)
{
    VkCommandBuffer CommandBuffer = BeginSingleTimeCommands(VulkanContext);
    
    VkBufferCopy BufferCopy = {};
    BufferCopy.size = Size;
    vkCmdCopyBuffer(CommandBuffer, SrcBuffer, DstBuffer, 1, &BufferCopy);
    
    EndSingleTimeCommands(VulkanContext, CommandBuffer);
}

internal void
TransitionImageLayout(vulkan_context *VulkanContext, 
                      VkImage Image, 
                      VkFormat Format, 
                      VkImageLayout OldImageLayout,
                      VkImageLayout NewImageLayout)
{
    VkCommandBuffer CommandBuffer = BeginSingleTimeCommands(VulkanContext);
    
    VkImageMemoryBarrier MemoryBarrier = {};
    MemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    MemoryBarrier.oldLayout = OldImageLayout;
    MemoryBarrier.newLayout = NewImageLayout;
    MemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    MemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    MemoryBarrier.image = Image;
    MemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    MemoryBarrier.subresourceRange.baseMipLevel = 0;
    MemoryBarrier.subresourceRange.levelCount = 1;
    MemoryBarrier.subresourceRange.baseArrayLayer = 0;
    MemoryBarrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags SrcStage;
    VkPipelineStageFlags DstStage;
    
    if (OldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        NewImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        MemoryBarrier.srcAccessMask = 0;
        MemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } 
    else if (OldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
        MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } 
    else 
    {
        Win32ErrorMessage(PlatformError_Fatal, "Unsupported layout transition.");
    }
    
    vkCmdPipelineBarrier(CommandBuffer,
                         SrcStage, 
                         DstStage,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, 
                         &MemoryBarrier);
    
    EndSingleTimeCommands(VulkanContext, CommandBuffer);
    
}

internal void
CopyBufferToImage(VkBuffer Buffer, 
                  VkImage Image, 
                  u32 Width, 
                  u32 Height,
                  vulkan_context *VulkanContext)
{
    VkCommandBuffer CommandBuffer = BeginSingleTimeCommands(VulkanContext);
    
    VkBufferImageCopy Region = {};
    Region.bufferOffset = 0;
    Region.bufferRowLength = 0;
    Region.bufferImageHeight = 0;
    Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Region.imageSubresource.mipLevel = 0;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = 1;
    Region.imageOffset = {0, 0, 0};
    Region.imageExtent = {
        Width,
        Height,
        1
    };
    
    vkCmdCopyBufferToImage(CommandBuffer, 
                           Buffer, 
                           Image, 
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                           1, 
                           &Region);
    
    EndSingleTimeCommands(VulkanContext, CommandBuffer);
}

internal void 
CreateTextureImage(vulkan_context *VulkanContext, asset *Asset)
{
    texture *Texture = &Asset->Textures[0];

    VkDeviceSize ImageSize = Texture->Width * Texture->Height * 4;

    VkBuffer StagingBuffer = {};
    VkDeviceMemory StagingBufferMemory = {};

    CreateBuffer(VulkanContext,
                 ImageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &StagingBuffer,
                 &StagingBufferMemory);

    void *Data;
    vkMapMemory(VulkanContext->Device,
                StagingBufferMemory,
                0,
                ImageSize,
                0,
                &Data);

    CopyArray(Texture->Data, Data, ImageSize);
    vkUnmapMemory(VulkanContext->Device, StagingBufferMemory);

    CreateImage(VulkanContext,
                Texture->Width,
                Texture->Height,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &VulkanContext->TextureImage,
                &VulkanContext->TextureImageMemory);

    TransitionImageLayout(VulkanContext,
                          VulkanContext->TextureImage,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(StagingBuffer,
                      VulkanContext->TextureImage,
                      (u32)Texture->Width,
                      (u32)Texture->Height,
                      VulkanContext);

    TransitionImageLayout(VulkanContext,
                          VulkanContext->TextureImage,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(VulkanContext->Device,
                    StagingBuffer,
                    0);
    vkFreeMemory(VulkanContext->Device,
                 StagingBufferMemory,
                 0);
}

internal void
CreateTextureImage(vulkan_context *VulkanContext, char *TexturePath)
{
    s32 TextureWidth;
    s32 TextureHeight;
    s32 TextureChannels;
    
    stbi_uc *Pixels = stbi_load(TexturePath, 
                                &TextureWidth,
                                &TextureHeight,
                                &TextureChannels,
                                STBI_rgb_alpha);
    
    VkDeviceSize ImageSize = TextureWidth * TextureHeight * 4;
    
    if (!Pixels)
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to load texture image.");
    }
    
    VkBuffer StagingBuffer = {};
    VkDeviceMemory StagingBufferMemory = {};
    
    CreateBuffer(VulkanContext,
                 ImageSize, 
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &StagingBuffer,
                 &StagingBufferMemory);
    
    void *Data;
    vkMapMemory(VulkanContext->Device, 
                StagingBufferMemory, 
                0, 
                ImageSize,
                0, 
                &Data);
    CopyArray(Pixels, Data, ImageSize);
    vkUnmapMemory(VulkanContext->Device, StagingBufferMemory);
    
    stbi_image_free(Pixels);
    
    CreateImage(VulkanContext,
                TextureWidth, 
                TextureHeight, 
                VK_FORMAT_R8G8B8A8_SRGB, 
                VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &VulkanContext->TextureImage,
                &VulkanContext->TextureImageMemory);
    
    TransitionImageLayout(VulkanContext,
                          VulkanContext->TextureImage,
                          VK_FORMAT_R8G8B8A8_SRGB, 
                          VK_IMAGE_LAYOUT_UNDEFINED, 
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    CopyBufferToImage(StagingBuffer, 
                      VulkanContext->TextureImage, 
                      (u32)TextureWidth, 
                      (u32)TextureHeight,
                      VulkanContext);
    
    TransitionImageLayout(VulkanContext,
                          VulkanContext->TextureImage, 
                          VK_FORMAT_R8G8B8A8_SRGB, 
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    vkDestroyBuffer(VulkanContext->Device, 
                    StagingBuffer, 
                    0);
    vkFreeMemory(VulkanContext->Device, 
                 StagingBufferMemory, 
                 0);
}

internal void
CreateTextureImageView(vulkan_context *VulkanContext)
{
    VulkanContext->TextureImageView = CreateImageView(VulkanContext,
                                                      VulkanContext->TextureImage,
                                                      VK_FORMAT_R8G8B8A8_SRGB,
                                                      VK_IMAGE_ASPECT_COLOR_BIT);
}

internal void
CreateTextureSampler(vulkan_context *VulkanContext)
{
    VkPhysicalDeviceProperties PhysicalDeviceProperties = {};
    vkGetPhysicalDeviceProperties(VulkanContext->PhysicalDevice,
                                  &PhysicalDeviceProperties);
    
    VkSamplerCreateInfo SamplerCreateInfo = {};
    SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCreateInfo.anisotropyEnable = VK_TRUE;
    SamplerCreateInfo.maxAnisotropy = PhysicalDeviceProperties.limits.maxSamplerAnisotropy;
    SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerCreateInfo.compareEnable = VK_FALSE;
    SamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    if (UNSUCCESSFUL(vkCreateSampler(VulkanContext->Device, 
                                     &SamplerCreateInfo, 
                                     0, 
                                     &VulkanContext->TextureSampler))) 
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to create VkSamplerCreateInfo.");
    }
}

internal void
CreateDepthResources(vulkan_context *VulkanContext)
{
    VkFormat DepthFormat = FindDepthFormat(VulkanContext);
    CreateImage(VulkanContext,
                VulkanContext->SwapChainExtent.width, 
                VulkanContext->SwapChainExtent.height, 
                DepthFormat, 
                VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &VulkanContext->DepthImage,
                &VulkanContext->DepthImageMemory);
    
    VulkanContext->DepthImageView = CreateImageView(VulkanContext, 
                                                    VulkanContext->DepthImage, 
                                                    DepthFormat,
                                                    VK_IMAGE_ASPECT_DEPTH_BIT);
}

// TODO(matthias): make sure that the memory is released and reallocated correctly
internal void
RecreateSwapChain(vulkan_context *VulkanContext,
                  HWND Window)

{
    vkDeviceWaitIdle(VulkanContext->Device);
    
    CleanupSwapChain(VulkanContext);
    
    CreateSwapChain(VulkanContext, Window);
    CreateImageViews(VulkanContext);
    CreateDepthResources(VulkanContext);
    CreateFramebuffers(VulkanContext);
}

internal void
LoadModel(vulkan_context *VulkanContext, char *ModelPath)
{
    tinyobj::attrib_t Attrib;
    std::vector<tinyobj::shape_t> Shapes;
    std::vector<tinyobj::material_t> Materials;
    std::string Warn; 
    std::string Err;
    
    if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err, ModelPath)) 
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to load object."); 
    }
    
    std::vector<vertex> Vertices = {};
    std::vector<u32> Indices = {};
    u32 VertexIndex = 0;
    
    for (const auto& Shape : Shapes) 
    {
        for (const auto& Index : Shape.mesh.indices) {
            vertex Vertex = {};
            
            Vertex.Pos = {
                Attrib.vertices[3 * Index.vertex_index + 0],
                Attrib.vertices[3 * Index.vertex_index + 1],
                Attrib.vertices[3 * Index.vertex_index + 2]
            };
            
            Vertex.TexCoord0 = {
                Attrib.texcoords[2 * Index.texcoord_index + 0],
                1.0f - Attrib.texcoords[2 * Index.texcoord_index + 1]
            };
            
            Vertex.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
            
            Vertices.push_back(Vertex);
            Indices.push_back(VertexIndex++);
        }
    }
    
    u32 VertexCount = Vertices.size();
    VulkanContext->VertexCount = VertexCount;
    VulkanContext->VerticesSize = sizeof(vertex) * VertexCount;
    VulkanContext->Vertices = VulkanContext->GraphicsMemoryBlock->PushArray<vertex>(VertexCount);
    CopyArray(Vertices.data(), VulkanContext->Vertices, VulkanContext->VerticesSize);
    
    u32 IndexCount = Indices.size();
    VulkanContext->IndexCount = IndexCount;
    VulkanContext->IndicesSize = sizeof(u32) * IndexCount;
    VulkanContext->Indices = VulkanContext->GraphicsMemoryBlock->PushArray<u32>(IndexCount);
    CopyArray(Indices.data(), VulkanContext->Indices, VulkanContext->IndicesSize);
}

internal void
ShutdownRenderer(vulkan_context *VulkanContext)
{
    CleanupSwapChain(VulkanContext);
    
    vkDestroySampler(VulkanContext->Device, VulkanContext->TextureSampler, 0);
    vkDestroyImageView(VulkanContext->Device, VulkanContext->TextureImageView, 0);
    
    vkDestroyImage(VulkanContext->Device, VulkanContext->TextureImage, 0);
    vkFreeMemory(VulkanContext->Device, VulkanContext->TextureImageMemory, 0);
    
    for (u32 Index = 0;
         Index < MAX_FRAMES_IN_FLIGHT;
         ++Index)
    {
        vkDestroyBuffer(VulkanContext->Device, VulkanContext->UniformBuffers[Index], 0);
        vkFreeMemory(VulkanContext->Device, VulkanContext->UniformBuffersMemory[Index], 0);
    }
    
    vkDestroyDescriptorPool(VulkanContext->Device, VulkanContext->DescriptorPool, 0);
    vkDestroyDescriptorSetLayout(VulkanContext->Device, VulkanContext->DescriptorSetLayout, 0);
    
    vkDestroyBuffer(VulkanContext->Device, VulkanContext->IndexBuffer, 0);
    vkFreeMemory(VulkanContext->Device, VulkanContext->IndexBufferMemory, 0);
    
    vkDestroyBuffer(VulkanContext->Device, VulkanContext->VertexBuffer, 0);
    vkFreeMemory(VulkanContext->Device, VulkanContext->VertexBufferMemory, 0);
    
    vkDestroyPipeline(VulkanContext->Device, VulkanContext->GraphicsPipeline, 0);
    vkDestroyPipelineLayout(VulkanContext->Device, VulkanContext->PipelineLayout, 0);
    
    vkDestroyRenderPass(VulkanContext->Device, VulkanContext->Renderpass, 0);
    
    for (u32 Index = 0; 
         Index < MAX_FRAMES_IN_FLIGHT; 
         ++Index)
    {
        vkDestroySemaphore(VulkanContext->Device, 
                           VulkanContext->ImageAvailableSemaphores[Index], 
                           0);
        vkDestroySemaphore(VulkanContext->Device, 
                           VulkanContext->RenderFinishedSemaphores[Index], 
                           0);
        vkDestroyFence(VulkanContext->Device, 
                       VulkanContext->InFlightFences[Index], 
                       0);
    }
    
    vkDestroyCommandPool(VulkanContext->Device,
                         VulkanContext->CommandPool,
                         0);
    
    vkDestroyDevice(VulkanContext->Device, 0);
    
    vkDestroySurfaceKHR(VulkanContext->Instance, VulkanContext->Surface, 0);
    vkDestroyInstance(VulkanContext->Instance, 0);
}

#if WARPUNK_DEBUG
internal void
InitializeImGui(vulkan_context *Vulkan, HWND Window)
{
    IMGUI_CHECKVERSION();
    
    VkDescriptorPoolSize PoolSize[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    
    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    PoolInfo.maxSets = 1000 * ArraySize(PoolSize);
    PoolInfo.poolSizeCount = (u32)ArraySize(PoolSize);
    PoolInfo.pPoolSizes = PoolSize;
    
    VkDescriptorPool ImGuiPool;
    vkCreateDescriptorPool(Vulkan->Device, &PoolInfo, nullptr, &ImGuiPool);
    
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO GuiIO = ImGui::GetIO();
    GuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplWin32_Init(Window);
    
    ImGui_ImplVulkan_InitInfo ImGuiInitInfo = {};
    ImGuiInitInfo.Instance = Vulkan->Instance;
    ImGuiInitInfo.PhysicalDevice = Vulkan->PhysicalDevice;
    ImGuiInitInfo.Device = Vulkan->Device;
    ImGuiInitInfo.Queue = Vulkan->GraphicsQueue.Queue;
    ImGuiInitInfo.DescriptorPool = ImGuiPool;
    ImGuiInitInfo.MinImageCount = 2;
    ImGuiInitInfo.ImageCount = 2;
    ImGuiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGuiInitInfo.Allocator = nullptr;
    
    ImGui_ImplVulkan_Init(&ImGuiInitInfo, Vulkan->Renderpass);
    
    ImGui_ImplVulkan_CreateFontsTexture();
    //ImGui_ImplVulkan_DestroyFontUploadObjects();
}
#else 
internal void
InitializeImGui(vulkan_context *Vulkan, HWND Window){}
#endif

#if WARPUNK_DEBUG
internal void
DrawDebugInfo(game_debug_info *GameDebugInfo)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    if (ImGui::Begin("Mouse", nullptr, ImGuiWindowFlags_NoCollapse ))
    {
        ImGui::Text("IsMoving: %i", GameDebugInfo->Mouse->IsMoving);
        ImGui::Text("PlatformMouseButton_Left: (IsDown: %i, WasDown: %i, IsDragging: %i)", 
                    GameDebugInfo->Mouse->State[MouseButton_Left].IsDown,
                    GameDebugInfo->Mouse->State[MouseButton_Left].WasDown,
                    GameDebugInfo->Mouse->State[MouseButton_Left].IsDragging);
        ImGui::Text("PlatformMouseButton_Middle: (IsDown: %i, WasDown: %i, IsDragging: %i)", 
                    GameDebugInfo->Mouse->State[MouseButton_Middle].IsDown,
                    GameDebugInfo->Mouse->State[MouseButton_Middle].WasDown,
                    GameDebugInfo->Mouse->State[MouseButton_Middle].IsDragging);
        ImGui::Text("PlatformMouseButton_Right: (IsDown: %i, WasDown: %i, IsDragging: %i)", 
                    GameDebugInfo->Mouse->State[MouseButton_Right].IsDown,
                    GameDebugInfo->Mouse->State[MouseButton_Right].WasDown,
                    GameDebugInfo->Mouse->State[MouseButton_Right].IsDragging);
        ImGui::Text("PlatformMouseButton_Wheel: (IsDown: %i, WasDown: %i, IsDragging: %i)", 
                    GameDebugInfo->Mouse->State[MouseButton_Wheel].IsDown,
                    GameDebugInfo->Mouse->State[MouseButton_Wheel].WasDown,
                    GameDebugInfo->Mouse->State[MouseButton_Wheel].IsDragging);
        ImGui::Text("P: (%i, %i)", 
                    GameDebugInfo->Mouse->Px, 
                    GameDebugInfo->Mouse->Py);
        ImGui::Text("DownP: (%i, %i)", 
                    GameDebugInfo->Mouse->DownPx, 
                    GameDebugInfo->Mouse->DownPy);
        ImGui::Text("dtDraggingInitial: (%i, %i)", 
                    GameDebugInfo->Mouse->DraggingInitialDtx, 
                    GameDebugInfo->Mouse->DraggingInitialDty);
        ImGui::Text("dtDraggingRecent: (%i, %i)", 
                    GameDebugInfo->Mouse->DraggingRecentDtx, 
                    GameDebugInfo->Mouse->DraggingRecentDty);
        ImGui::Text("Alt: %i",
                    GameDebugInfo->Keyboard->State[KeyboardButton_Alt]);
    }
    ImGui::End();
    
    if (ImGui::Begin("Camera", nullptr, ImGuiWindowFlags_NoCollapse ))
    {
        ImGui::Text("Pos: (%g, %g, %g)", 
                    GameDebugInfo->Camera->Pos.x, 
                    GameDebugInfo->Camera->Pos.y,
                    GameDebugInfo->Camera->Pos.z);
        ImGui::Text("Dir: (%g, %g, %g)", 
                    GameDebugInfo->Camera->Dir.x, 
                    GameDebugInfo->Camera->Dir.y,
                    GameDebugInfo->Camera->Dir.z);
        ImGui::Text("Up: (%g, %g, %g)", 
                    GameDebugInfo->Camera->Up.x, 
                    GameDebugInfo->Camera->Up.y,
                    GameDebugInfo->Camera->Up.z);
        ImGui::Text("Right: (%g, %g, %g)", 
                    GameDebugInfo->Camera->Right.x, 
                    GameDebugInfo->Camera->Right.y,
                    GameDebugInfo->Camera->Right.z);
        ImGui::Text("YPR: (%g, %g, %g)", 
                    GameDebugInfo->Camera->Yaw, 
                    GameDebugInfo->Camera->Pitch,
                    GameDebugInfo->Camera->Roll);
        
        static ImGuiSliderFlags Flags = ImGuiSliderFlags_None;
        ImGui::DragFloat("Speed: %g", &GameDebugInfo->Camera->Speed, 0.1f, 0.0f, FLT_MAX, "%0.3f", Flags);
        
        if (ImGui::Button("Reset Camera"))
        {
            InitializeCamera(GameDebugInfo->Camera);
        }
        
    }
    ImGui::End();
    
    
    
    ImGui::ShowDemoWindow();
    ImGui::Render();
}
#else
internal void
DrawDebugInfo(game_debug_info *GameDebugInfo){}
#endif
