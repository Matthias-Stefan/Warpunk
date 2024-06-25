#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdio.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <windows.h>
#include <windowsx.h>
#include <xaudio2.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "source/thirdparty/tiny_gltf.h"

#include <queue>

#if WARPUNK_DEBUG
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui.cpp"
#include "imgui/imgui_impl_win32.cpp"
#include "imgui/imgui_impl_vulkan.cpp"

#include "imgui/implot_items.cpp"
#include "imgui/implot.cpp"

#include "imgui/imgui_demo.cpp"
#endif



#include "source/core/math.h"
#include "source/core/types.h"

#include "win32_warpunk.h"
#include "win32_warpunk_audio.cpp"

PLATFORM_READ_DATA_FROM_FILE(Win32ReadFileBinary);
#include "warpunk_renderer.h"
#include "warpunk_renderer.cpp"

#include "warpunk_world.h"
#include "warpunk.h"

/// Globals

global b32 GlobalRunning = false; 
global win32_state GlobalWin32State;
global win32_offscreen_buffer GlobalBackbuffer;
global s64 GlobalPerformanceFrequency;

global IXAudio2 *GlobalAudioEngine;
global audio_stream GlobalAudioStreams[MAX_AUDIO_STREAMS];

global u32 CurrentFrame = 0; 
global vulkan_context VulkanContext;
global vertex *Vertices;

/// Error

PLATFORM_ERROR_MESSAGE(Win32ErrorMessage)
{
    char *Caption = "Warpunk Warning";
    
    UINT MBoxType = MB_OK;
    if (Type == PlatformError_Fatal)
    {
        Caption = "Warpunk Fatal Error";
        MBoxType |= MB_ICONSTOP;
    }
    else
    {
        MBoxType |= MB_ICONWARNING;
    }
    
    
    MessageBoxExA(GlobalWin32State.DefaultWindowHandle, Message, Caption, MBoxType, 0);
    
    if (Type == PlatformError_Fatal)
    {
        ExitProcess(1);
    }
}

/// String

internal void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{
    for(int Index = 0;
        Index < SourceACount;
        ++Index)
    {
        *Dest++ = *SourceA++;
    }
    
    for(int Index = 0;
        Index < SourceBCount;
        ++Index)
    {
        *Dest++ = *SourceB++;
    }
    
    *Dest++ = 0;
}

inline unsigned int
StringLength(char *String)
{
    u32 Count = 0;
    while(*String++)
    {
        ++Count;
    }
    return Count;
}

/// Path & Filename

internal void
Win32GetEXEFilename()
{
    DWORD SizeOfFilename = GetModuleFileNameA(0, GlobalWin32State.EXEFilename, 
                                              sizeof(GlobalWin32State.EXEFilename));
    GlobalWin32State.OnePastLastEXEFilenameSlash = GlobalWin32State.EXEFilename;
    for(char *Scan = GlobalWin32State.EXEFilename;
        *Scan;
        ++Scan)
    {
        if(*Scan == '\\')
        {
            GlobalWin32State.OnePastLastEXEFilenameSlash = Scan + 1;
        }
    }
}

internal void
Win32BuildEXEPathFilename(char *Filename, int DestCount, char *Dest)
{
    CatStrings(GlobalWin32State.OnePastLastEXEFilenameSlash - GlobalWin32State.EXEFilename, 
               GlobalWin32State.EXEFilename,
               StringLength(Filename), 
               Filename,
               DestCount, 
               Dest);
}

PLATFORM_READ_DATA_FROM_FILE(Win32ReadFileBinary)
{
    FILE *File = fopen(Filename, "rb");
    
    if (File)
    {
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);
        
        if (FileSize <= GB(8))
        {
            Handle->Size = FileSize;
            Handle->Content = (unsigned char *)VirtualAlloc(0, Handle->Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            
            if (Handle->Content)
            {
                size_t BytesRead = fread(Handle->Content, 1, FileSize, File);
                Handle->Content[BytesRead] = '\0';
            }
            else
            {
                // TODO: Handle memory allocation error.
                //Assert(false);
            }
        }
        else
        {
            // TODO: Handle file too large error.
            //Assert(false);
        }
        
        fclose(File);
    }
    else
    {
        // TODO: Handle file open error.
        //Assert(false);
    }
}

/// Rendering

internal void
Win32InitializeVulkan(HDC DeviceContext, HWND Window, HINSTANCE Instance, memory_block *RendererMemory, platform_api *PlatformAPI)
{
    VulkanContext = {};
    VulkanContext.GraphicsMemoryBlock = RendererMemory; 
    VulkanContext.PlatformAPI = PlatformAPI;

    //DEBUGGetVertices(&VulkanContext); 
    
    CreateInstance(&VulkanContext);
    CreateSurface(&VulkanContext, Window, Instance);
    SelectPhysicalDevice(&VulkanContext);
    const char* EnabledDeviceExtension[] =
    { 
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    CreateLogicalDevice(&VulkanContext,
                        ArraySize(EnabledDeviceExtension), 
                        EnabledDeviceExtension);
    CreateSwapChain(&VulkanContext, Window);
    
    CreateImageViews(&VulkanContext);
    CreateRenderPass(&VulkanContext);
    CreateDescriptorSetLayout(&VulkanContext);
    CreateGraphicsPipeline(&VulkanContext);
    CreateDepthResources(&VulkanContext);
    CreateFramebuffers(&VulkanContext);
    CreateCommandPool(&VulkanContext);
    CreateTextureImage(&VulkanContext, "W:/Warpunk/assets/textures/viking_room.png");
    CreateTextureImageView(&VulkanContext);
    CreateTextureSampler(&VulkanContext);
    Win32LoadAsset("W:/Warpunk/assets/gltf/2CylinderEngine.glb", RendererMemory);
    LoadModel(&VulkanContext, "W:/Warpunk/assets/geo/untitled.obj");
    CreateVertexBuffer(&VulkanContext);
    CreateIndexBuffer(&VulkanContext);
    CreateUniformBuffers(&VulkanContext);
    CreateDescriptorPool(&VulkanContext);
    CreateDescriptorSets(&VulkanContext);
    CreateCommandBuffers(&VulkanContext);
    CreateSyncObjects(&VulkanContext);
    InitializeImGui(&VulkanContext, Window);
}

internal RENDERER_DRAW_FRAME(Win32DrawFrame)
{
    vkWaitForFences(VulkanContext.Device, 1, &VulkanContext.InFlightFences[CurrentFrame], VK_TRUE, MAX_U64);
    
    u32 ImageIndex;
    VkResult Result = vkAcquireNextImageKHR(VulkanContext.Device,
                                            VulkanContext.SwapChain,
                                            MAX_U64,
                                            VulkanContext.ImageAvailableSemaphores[CurrentFrame],
                                            VK_NULL_HANDLE,
                                            &ImageIndex);
    
    if (Result == VK_ERROR_OUT_OF_DATE_KHR) 
    {
        HWND Window = GetActiveWindow();
        
        RecreateSwapChain(&VulkanContext, Window);
        return;
    } 
    else if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR) 
    {
        OutputDebugStringA("Failed to submit draw command buffer.\n");
    }
    
    UpdateUniformBuffer(&VulkanContext, CurrentFrame, Camera);
    
    vkResetFences(VulkanContext.Device, 1, &VulkanContext.InFlightFences[CurrentFrame]);
    
    vkResetCommandBuffer(VulkanContext.CommandBuffers[CurrentFrame], 0);
    RecordCommandBuffer(&VulkanContext, 
                        VulkanContext.CommandBuffers[CurrentFrame], 
                        ImageIndex,
                        CurrentFrame,
                        GameDebugInfo);
    
    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore WaitSemaphores[] = { VulkanContext.ImageAvailableSemaphores[CurrentFrame] };
    VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = WaitSemaphores;
    SubmitInfo.pWaitDstStageMask = WaitStages;
    
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &VulkanContext.CommandBuffers[CurrentFrame];
    
    VkSemaphore SignalSemaphores[] = { VulkanContext.RenderFinishedSemaphores[CurrentFrame] };
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores = SignalSemaphores;
    
    if (UNSUCCESSFUL(vkQueueSubmit(VulkanContext.GraphicsQueue.Queue, 
                                   1, 
                                   &SubmitInfo, 
                                   VulkanContext.InFlightFences[CurrentFrame])))
        
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to submit draw command buffer.");
    }
    
    VkPresentInfoKHR PresentInfo = {};
    PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores = SignalSemaphores;
    
    VkSwapchainKHR SwapChains[] = { VulkanContext.SwapChain };
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = SwapChains;
    
    PresentInfo.pImageIndices = &ImageIndex;
    
    Result = vkQueuePresentKHR(VulkanContext.GraphicsQueue.Queue, &PresentInfo);
    
    if (Result == VK_ERROR_OUT_OF_DATE_KHR ||
        Result== VK_SUBOPTIMAL_KHR ||
        VulkanContext.FramebufferResized == 1)
    {
        HWND Window = GetActiveWindow();
        
        VulkanContext.FramebufferResized = 0;
        RecreateSwapChain(&VulkanContext, Window);
    }
    else if(UNSUCCESSFUL(Result))
    {
        Win32ErrorMessage(PlatformError_Fatal, "Failed to acquire swap chain image.");
    }
    
    CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/// Backbuffer

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    
    return Result;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    } 
    
    Buffer->Width = Width;
    Buffer->Height = Height;
    
    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;
    
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    Buffer->Pitch = Buffer->Width * BytesPerPixel;
    int BitmapMemorySize = Buffer->Pitch * Buffer->Height;
    
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext)
{
    StretchDIBits(DeviceContext,
                  0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height,
                  0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height,
                  GlobalBackbuffer.Memory,
                  &GlobalBackbuffer.Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

/// Audio

internal void
Win32InitializeXAudio2()
{
    HMODULE XAudio2Library = LoadLibraryA("XAudio2_9.dll");
    if (!XAudio2Library)
    {
        LoadLibraryA("XAudio2_8.dll");
    }
    
    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        x_audio2_create *XAudio2Create = (x_audio2_create *)GetProcAddress(XAudio2Library, "XAudio2Create");
        if (SUCCEEDED(XAudio2Create(&GlobalAudioEngine, 0, XAUDIO2_DEFAULT_PROCESSOR)))
        {
            GlobalAudioEngine->StartEngine();
            
#if WARPUNK_DEBUG
            XAUDIO2_DEBUG_CONFIGURATION AudioEngineDebugInfo = {};
            AudioEngineDebugInfo.TraceMask = XAUDIO2_LOG_ERRORS|XAUDIO2_LOG_WARNINGS|XAUDIO2_LOG_INFO;
            AudioEngineDebugInfo.BreakMask = XAUDIO2_LOG_WARNINGS;
            AudioEngineDebugInfo.LogThreadID;
            AudioEngineDebugInfo.LogFileline;
            AudioEngineDebugInfo.LogFunctionName;
            AudioEngineDebugInfo.LogTiming;
            GlobalAudioEngine->SetDebugConfiguration(&AudioEngineDebugInfo);
#endif
            IXAudio2MasteringVoice *MasteringVoice;
            
            HRESULT CreateResult = GlobalAudioEngine->CreateMasteringVoice(&MasteringVoice);
            if (SUCCEEDED(CreateResult))
            {
                OutputDebugStringA("IXAudio2MasteringVoice created successfully.\n");
            }
            
        }
        else
        {
            // TODO(matthias): Diagnostic
        }
    }
}

internal void
Win32ProcessAudioStreams(std::queue<u32> *PendingAudios)
{
    if (PendingAudios->size() <= 0)
    {
        return;
    }
    
    for (int StreamIndex = 0; StreamIndex < MAX_AUDIO_STREAMS; ++StreamIndex)
    {
        audio_stream &AudioStream = GlobalAudioStreams[StreamIndex]; 
        switch (AudioStream.State)
        {
            case AudioStreamState_Free:
            {
                if (PendingAudios->size() > 0)
                {
                    AudioStream = {};
                    AudioStream.FilenameIndex = (s32)(PendingAudios->front());  
                    
                    StartAudioStream(&AudioStream, GlobalAudioEngine);
                    
                    AudioStream.SourceVoice->SetVolume(1.0f);
                    AudioStream.SourceVoice->Start(0);
                    AudioStream.State |= AudioStreamState_Playing;
                    
                    PendingAudios->pop();
                }
            } break;
            
            case AudioStreamState_Playing:
            {
                XAUDIO2_VOICE_STATE VoiceState;
                AudioStream.SourceVoice->GetState(&VoiceState);
                
                if (VoiceState.BuffersQueued <= 2)
                {
                    LoadAudio(&AudioStream);
                }
            } break;
            
            case AudioStreamState_Closed:
            {
                StopAudioStream(&AudioStream);
            } break;
        }
    }
};

/// Timing

inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline f32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    f32 Result = ((f32)(End.QuadPart - Start.QuadPart) /
                  (f32)GlobalPerformanceFrequency);
    return Result;
}

/// Platform messages

internal void
Win32ProcessPendingMessages(HWND Window, game_input *GameInput)
{
    MSG Message = {};
    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        ImGui_ImplWin32_WndProcHandler(Window, Message.message, Message.wParam, Message.lParam);
        
        switch(Message.message)
        {
            case WM_DESTROY:
            {
                GlobalRunning = false;
            } break;
            
            case WM_KEYUP:
            case WM_KEYDOWN:
            case WM_SYSKEYUP:
            case WM_SYSKEYDOWN:
            {
                s32 VKCode = (s32)Message.wParam;
                
                switch (VKCode)
                {
                    case 'A': 
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_A].IsDown = true;
                    } break;
                    case 'B':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_B].IsDown = true;
                    } break;
                    case 'C':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_C].IsDown = true;
                    } break;
                    case 'D':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_D].IsDown = true;
                    } break;
                    case 'E':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_E].IsDown = true;
                    } break;
                    case 'F':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_F].IsDown = true;
                    } break;
                    case 'G':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_G].IsDown = true;
                    } break;
                    case 'H': 
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_H].IsDown = true;
                    } break;
                    case 'I':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_I].IsDown = true;
                    } break;
                    case 'J':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_J].IsDown = true;
                    } break;
                    case 'K':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_K].IsDown = true;
                    } break;
                    case 'L':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_L].IsDown = true;
                    } break;
                    case 'M':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_M].IsDown = true;
                    } break;
                    case 'N':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_N].IsDown = true;
                    } break;
                    case 'O':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_O].IsDown = true;
                    } break;
                    case 'P':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_P].IsDown = true;
                    } break;
                    case 'Q':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_Q].IsDown = true;
                    } break;
                    case 'R':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_R].IsDown = true;
                    } break;
                    case 'S':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_S].IsDown = true;
                    } break;
                    case 'T':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_T].IsDown = true;
                    } break;
                    case 'U':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_U].IsDown = true;
                    } break;
                    case 'V':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_V].IsDown = true;
                    } break;
                    case 'W':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_W].IsDown = true;
                    } break;
                    case 'X':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_X].IsDown = true;
                    } break;
                    case 'Y':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_Y].IsDown = true;
                    } break;
                    case 'Z':
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_Z].IsDown = true;
                    } break;
                    
                    case VK_LEFT:
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_ArrowLeft].IsDown = true;
                    } break;
                    case VK_UP:
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_ArrowUp].IsDown = true;
                    } break;
                    case VK_RIGHT:
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_ArrowRight].IsDown = true;
                    } break;
                    case VK_DOWN:
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_ArrowDown].IsDown = true;
                    } break;
                    case VK_MENU:
                    {
                        GameInput->Keyboard.State[keyboard_buttons::KeyboardButton_Alt].IsDown = true;
                    } break;
                }
            } break;
            
            case WM_LBUTTONDOWN:
            {
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                
                GameInput->Mouse.State[MouseButton_Left].IsDown = true;
                
                GameInput->Mouse.DownPx = GET_X_LPARAM(Message.lParam);
                GameInput->Mouse.DownPy = Dimension.Height - GET_Y_LPARAM(Message.lParam);
                
                GameInput->Mouse.Px = GameInput->Mouse.DownPx;
                GameInput->Mouse.Py = GameInput->Mouse.DownPy;
            } break;
            
            case WM_LBUTTONUP:
            {
                GameInput->Mouse.State[MouseButton_Left].IsDown = false;
                GameInput->Mouse.State[MouseButton_Left].IsDragging = false;
                GameInput->Mouse.State[MouseButton_Left].WasDown = false;
            } break;
            
            case WM_MBUTTONDOWN:
            {
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                
                GameInput->Mouse.State[MouseButton_Middle].IsDown = true;
                
                GameInput->Mouse.DownPx = GET_X_LPARAM(Message.lParam);
                GameInput->Mouse.DownPy = Dimension.Height - GET_Y_LPARAM(Message.lParam);
                
                GameInput->Mouse.Px = GameInput->Mouse.DownPx;
                GameInput->Mouse.Py = GameInput->Mouse.DownPy;
            } break;
            
            case WM_MBUTTONUP:
            {
                GameInput->Mouse.State[MouseButton_Middle].IsDown = false;
                GameInput->Mouse.State[MouseButton_Middle].IsDragging = false;
                GameInput->Mouse.State[MouseButton_Middle].WasDown = false;
            } break;
            
            case WM_RBUTTONDOWN:
            {
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                
                GameInput->Mouse.State[MouseButton_Right].IsDown = true;
                
                GameInput->Mouse.DownPx = GET_X_LPARAM(Message.lParam);
                GameInput->Mouse.DownPy = Dimension.Height - GET_Y_LPARAM(Message.lParam); 
                
                GameInput->Mouse.Px = GameInput->Mouse.DownPx;
                GameInput->Mouse.Py = GameInput->Mouse.DownPy;
            } break;
            
            case WM_RBUTTONUP:
            {
                GameInput->Mouse.State[MouseButton_Right].IsDown = false;
                GameInput->Mouse.State[MouseButton_Right].IsDragging = false;
                GameInput->Mouse.State[MouseButton_Right].WasDown = false;
            } break;
            
            case WM_MOUSEWHEEL:
            {
                GameInput->Mouse.State[MouseButton_Wheel].IsDown = true;
                GameInput->Mouse.WheelDelta = GET_WHEEL_DELTA_WPARAM(Message.wParam);
            } break;
            
            case WM_MOUSEMOVE:
            {
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                
                GameInput->Mouse.IsMoving = true;
                
                GameInput->Mouse.Px = GET_X_LPARAM(Message.lParam);
                GameInput->Mouse.Py = Dimension.Height - GET_Y_LPARAM(Message.lParam);
                
                s32 DraggingOldDtx = GameInput->Mouse.DraggingInitialDtx; 
                s32 DraggingOldDty = GameInput->Mouse.DraggingInitialDty; 
                
                s32 dtX = GameInput->Mouse.Px - GameInput->Mouse.DownPx;
                s32 dtY = GameInput->Mouse.Py - GameInput->Mouse.DownPy;
                
                GameInput->Mouse.DraggingInitialDtx = dtX;
                GameInput->Mouse.DraggingInitialDty = dtY;
                
                GameInput->Mouse.DraggingRecentDtx = GameInput->Mouse.DraggingInitialDtx - DraggingOldDtx;  
                GameInput->Mouse.DraggingRecentDty = GameInput->Mouse.DraggingInitialDty - DraggingOldDty;  
                
                int Distance = Max(abs(dtX), abs(dtY));
                
                for(u32 MouseStateIndex = 0;
                    MouseStateIndex < MouseButton_Count;
                    ++MouseStateIndex)
                {
                    if (GameInput->Mouse.State[MouseStateIndex].WasDown &&
                        Distance > GetSystemMetrics(SM_CXDRAG))
                    {
                        GameInput->Mouse.State[MouseStateIndex].IsDragging = true; 
                    }
                }
            } break;
            
            //
            //
            //
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
        }
    }
}

#if WARPUNK_DEBUG
extern LRESULT 
ImGui_ImplWin32_WndProcHandler(HWND Window, 
                               UINT Message,
                               WPARAM WParam, 
                               LPARAM LParam);
#endif
LRESULT CALLBACK 
Win32MainWindowCallback(HWND Window, 
                        UINT Message,
                        WPARAM WParam, 
                        LPARAM LParam)
{
    if (ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam))
    {
		return true;
    }
    
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            EndPaint(Window, &Paint);
        } break;
        
        case WM_SIZE:
        {
            u32 ClientWidth = LParam & 0xFFFF;
            u32 ClientHeight = (LParam >> 16) & 0xFFFF;
            
            GlobalBackbuffer.Width = ClientWidth;
            GlobalBackbuffer.Height = ClientHeight;
            
        } break;
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        }
    }
    
    return Result;
}

/// Gamecode loading

internal FILETIME
Win32GetLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime = {};
    
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }
    
    return LastWriteTime;
}

internal win32_game_code
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName, char *LockFileName)
{
    win32_game_code Result = {};
    
    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if (!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
    {
        Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);
        
        CopyFile(SourceDLLName, TempDLLName, FALSE);
        
        Result.GameCodeDLL = LoadLibraryA(TempDLLName);
        if (Result.GameCodeDLL)
        {
            Result.Initialize = (game_initialize *)
                GetProcAddress(Result.GameCodeDLL, "GameInitialize");
            
            Result.UpdateAndRender = (game_update_and_render *)
                GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
            
            if (Result.Initialize && Result.UpdateAndRender)
            {
                Result.IsValid = true;
            }
        }
    }
    
    if (!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
    }
    
    return Result;
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
    if (GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }
    
    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
}

/// Multi-threading

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
    work_thread *Thread = (work_thread *)lpParameter;
    work_queue *Queue = Thread->Queue;

    u32 ThreadID = GetThreadID();
    Assert(ThreadID == GetCurrentThreadId());

    for (;;)
    {
        if (Win32DoNextWorkQueueEntry(Queue))
        {
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }
}

internal PLATFORM_CREATE_WORK_QUEUE(Win32CreateWorkQueue)
{
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;

    Queue->NextEntryToWrite = 0;
    Queue->NextEntryToRead = 0;

    u32 InitialCount = 0;
    Queue->SemaphoreHandle = CreateSemaphoreEx(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

    for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
    {
        work_thread *WorkThread = WorkThreads + ThreadIndex;
        WorkThread->Queue = Queue;

        DWORD ThreadID;
        WorkThread->Handle = CreateThread(0, MB(1), ThreadProc, WorkThread, 0, &ThreadID);
        CloseHandle(WorkThread->Handle);
    }
}

internal PLATFORM_WORK_QUEUE_CALLBACK(Win32WorkQueueCallback)
{
    char Buffer[256];
    wsprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char *)Data);
    OutputDebugStringA(Buffer);
}

internal PLATFORM_ADD_WORK_QUEUE_ENTRY(Win32AddWorkQueueEntry)
{
    u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArraySize(Queue->Entries);
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    ++Queue->CompletionGoal;
    _WriteBarrier();
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal PLATFORM_DO_NEXT_WORK_QUEUE_ENTRY(Win32DoNextWorkQueueEntry)
{
    b32 ShouldSleep = false;

    u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArraySize(Queue->Entries);
    if (OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        u32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                               NewNextEntryToRead,
                                               OriginalNextEntryToRead);
        if (Index == OriginalNextEntryToRead)
        {
            // TODO(matthias): For improved threading efficiency, consider implementing a 
            // thread-specific structure. This structure should include a scratch arena 
            // that the thread can utilize for all its temporary operations.
            work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
        }
    }
    else
    {
        ShouldSleep = true;
    }

    return ShouldSleep;
}

internal PLATFORM_COMPLETE_ALL_WORK(Win32CompleteAllWork)
{
    while (Queue->CompletionGoal != Queue->CompletionCount)
    {
        Win32DoNextWorkQueueEntry(Queue);
    }

    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
}

/// Memory allocation services

internal PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    Memory->Allocated = Size;
    Memory->Used = 0;
#if defined(_WIN32) || defined(_WIN64)
    Memory->Data = (void *)VirtualAlloc(NULL, Memory->Allocated, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#elif defined(__linux__)
    Memory->Data = (void *)mmap(NULL, Memory->Allocated, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    Assert(Memory->Data != nullptr);
}

internal PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
    if (Memory->Data)
    {
        if (Memory->Data)
        {
#if defined(_WIN32) || defined(_WIN64)
            VirtualFree(Memory->Data, 0, MEM_RELEASE);
#elif defined(__linux__)
            munmap(Memory->Data, Allocated);
#endif
        }

        Memory->Allocated = 0;
        Memory->Used = 0;
        Memory->Data = nullptr;
    }
}

internal PLATFORM_ENLARGE_MEMORY(Win32EnlargeMemory)
{
    size_t NewAllocationSize = Memory->Allocated + Size;
    if (Memory->Data)
    {
#if defined(_WIN32) || defined(_WIN64)
        void *Temp = (void *)VirtualAlloc(NULL, NewAllocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#elif defined(__linux__)
        void *Temp = (void *)mmap(NULL, NewAllocationSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif

        Assert(Temp != nullptr);
        memcpy(Temp, Memory->Data, Memory->Allocated);

#if defined(_WIN32) || defined(_WIN64)
        VirtualFree(Memory->Data, 0, MEM_RELEASE);
#elif defined(__linux__)
        munmap(Memory->Data, Memory->Allocated);
#endif

        Memory->Allocated = NewAllocationSize;
        Memory->Data = Temp;
    }
    else
    {
        Win32AllocateMemory(Memory, Size);
    }
}

/// Asset loading services

inline u32
glTFGetComponentSize(s32 ComponentType)
{
    u32 Result = 0;

    switch (ComponentType)
    {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        {
            Result = sizeof(char);
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        {
            Result = sizeof(unsigned char);
        } break;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        {
            Result = sizeof(short);
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        {
            Result = sizeof(unsigned short);
        } break;
        case TINYGLTF_COMPONENT_TYPE_INT:
        {
            Result = sizeof(int);
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        {
            Result = sizeof(unsigned int);
        } break;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
        {
            Result = sizeof(float);
        } break;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
        {
            Result = sizeof(double);
        } break;
    }

    return Result;
}

template <typename T, typename S>
inline S
glTFGetBufferValue_(tinygltf::Buffer *Buffer,
                    tinygltf::BufferView *BufferView,
                    size_t Index)
{
    S Result = static_cast<S>(*reinterpret_cast<T *>(&Buffer->data[Index]));
    return Result;
}

template <typename S>
inline S
glTFGetBufferValue(s32 ComponentType,
                   tinygltf::Buffer *Buffer,
                   tinygltf::BufferView *BufferView,
                   size_t Index)
{
    S Result;

    switch (ComponentType)
    {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        {
            Result = glTFGetBufferValue_<char, S>(Buffer, BufferView, Index);
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        {
            Result = glTFGetBufferValue_<unsigned char, S>(Buffer, BufferView, Index);
        } break;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        {
            Result = glTFGetBufferValue_<short, S>(Buffer, BufferView, Index);
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        {
            Result = glTFGetBufferValue_<unsigned short, S>(Buffer, BufferView, Index);
        } break;
        case TINYGLTF_COMPONENT_TYPE_INT:
        {
            Result = glTFGetBufferValue_<int, S>(Buffer, BufferView, Index);
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        {
            Result = glTFGetBufferValue_<unsigned int, S>(Buffer, BufferView, Index);
        } break;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
        {
            Result = glTFGetBufferValue_<float, S>(Buffer, BufferView, Index);
        } break;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
        {
            Result = glTFGetBufferValue_<double, S>(Buffer, BufferView, Index);
        } break;
    }

    return Result;
}

internal PLATFORM_LOAD_ASSET(Win32LoadAsset)
{
#if true
    tinygltf::TinyGLTF TinyLoader;

    std::string Error;
    std::string Warning;
    tinygltf::Model TinyModel;
    if (!TinyLoader.LoadBinaryFromFile(&TinyModel, &Error, &Warning, Filename))
    {
        Win32ErrorMessage(PlatformError_Nonfatal, "Failed to load gltf.");
    }
#endif

    asset *Result = Memory->PushStruct<asset>();
    *Result = {};
    Result->Id = 0;
    
    u32 MeshCount = 0;
    for (size_t NodeIndex = 0; NodeIndex < TinyModel.nodes.size(); ++NodeIndex)
    {
        tinygltf::Node TinyNode = TinyModel.nodes[NodeIndex];

        if (TinyNode.mesh < 0) // this node is not a mesh
        {
            continue;
        }

        MeshCount++;
    }

    Result->Meshes = Memory->PushArray<mesh>(MeshCount);
    Result->Size += sizeof(mesh) * MeshCount;

    for (size_t NodeIndex = 0, AssetIndex = 0;
         NodeIndex < TinyModel.nodes.size();
         ++NodeIndex)
    {
        tinygltf::Node TinyNode = TinyModel.nodes.at(NodeIndex);

        if (TinyNode.mesh < 0) // this node is not a mesh
        {
            continue;
        }

        glm::mat4 Matrix(1.0f);
        if (TinyNode.matrix.size() > 0) // no identity matrix
        {
            Assert(TinyNode.matrix.size() == 16);

            Matrix[0][0] = (f32)TinyNode.matrix[0];
            Matrix[0][1] = (f32)TinyNode.matrix[1];
            Matrix[0][2] = (f32)TinyNode.matrix[2];
            Matrix[0][3] = (f32)TinyNode.matrix[3];

            Matrix[1][0] = (f32)TinyNode.matrix[4];
            Matrix[1][1] = (f32)TinyNode.matrix[5];
            Matrix[1][2] = (f32)TinyNode.matrix[6];
            Matrix[1][3] = (f32)TinyNode.matrix[7];

            Matrix[2][0] = (f32)TinyNode.matrix[8];
            Matrix[2][1] = (f32)TinyNode.matrix[9];
            Matrix[2][2] = (f32)TinyNode.matrix[10];
            Matrix[2][3] = (f32)TinyNode.matrix[11];

            Matrix[3][0] = (f32)TinyNode.matrix[12];
            Matrix[3][1] = (f32)TinyNode.matrix[13];
            Matrix[3][2] = (f32)TinyNode.matrix[14];
            Matrix[3][3] = (f32)TinyNode.matrix[15];
        }
        else
        {
            glm::mat4 MatrixScale(1.0f);
            glm::mat4 MatrixRotation(1.0f);
            glm::mat4 MatrixTranslation(1.0f);

            if (TinyNode.scale.size() > 0) // no identity matrix
            {
                assert(TinyNode.scale.size() == 3);
                MatrixScale = glm::scale(glm::mat4(1.0f),
                                         glm::vec3((f32)TinyNode.scale[0],
                                                   (f32)TinyNode.scale[1],
                                                   (f32)TinyNode.scale[2]));
            }

            if (TinyNode.rotation.size() > 0) // no identity matrix
            {
                assert(TinyNode.rotation.size() == 4);
                MatrixRotation = glm::mat4_cast(
                    glm::quat((f32)TinyNode.rotation[3],
                              (f32)TinyNode.rotation[0],
                              (f32)TinyNode.rotation[1],
                              (f32)TinyNode.rotation[2]));
            }

            if (TinyNode.translation.size() > 0) // no identity matrix
            {
                assert(TinyNode.translation.size() == 3);
                MatrixTranslation = glm::translate(glm::mat4(1.0f),
                                                   glm::vec3((f32)TinyNode.translation[0],
                                                             (f32)TinyNode.translation[1],
                                                             (f32)TinyNode.translation[2]));
            }

            Matrix = MatrixTranslation * MatrixRotation * MatrixScale;
        }

        s32 MeshIndex = TinyNode.mesh;
        tinygltf::Mesh TinyMesh = TinyModel.meshes.at(MeshIndex);

        mesh *Mesh = &Result->Meshes[AssetIndex++];

        temporary_memory_block<u32> Indices = {};
        temporary_memory_block<f32> Positions = {};
        temporary_memory_block<f32> Normals = {};
        temporary_memory_block<f32> Tangents = {};
        temporary_memory_block<f32> UVs = {};
        temporary_memory_block<f32> UVs2 = {};

        for (size_t PrimitiveIndex = 0; 
             PrimitiveIndex < TinyMesh.primitives.size(); 
             ++PrimitiveIndex)
        {
            tinygltf::Primitive& TinyPrimitive = TinyMesh.primitives[PrimitiveIndex];
            const auto PositionIt = TinyPrimitive.attributes.find("POSITION");
            const auto NormalIt = TinyPrimitive.attributes.find("NORMAL");
            const auto TangentIt = TinyPrimitive.attributes.find("TANGENT");
            const auto Texcoord0It = TinyPrimitive.attributes.find("TEXCOORD_0");
            const auto Texcoord1It = TinyPrimitive.attributes.find("TEXCOORD_1");

            const bool HasPosition = PositionIt != TinyPrimitive.attributes.end();
            const bool HasNormal = NormalIt != TinyPrimitive.attributes.end();
            const bool HasTangent = TangentIt != TinyPrimitive.attributes.end();
            const bool HasTexcoord0 = Texcoord0It != TinyPrimitive.attributes.end();
            const bool HasTexcoord1 = Texcoord1It != TinyPrimitive.attributes.end();

            if (TinyPrimitive.indices > 0 &&
                HasPosition &&
                HasNormal)
            {
                s32 Idx = -1;

                // Indices
                Idx = TinyPrimitive.indices;
                Assert(Idx >= 0);

                tinygltf::Accessor Accessor = TinyModel.accessors[Idx];
                tinygltf::BufferView BufferView = TinyModel.bufferViews[Accessor.bufferView];
                tinygltf::Buffer Buffer = TinyModel.buffers[BufferView.buffer];
                
                EnlargeTemporaryMemory(&Indices, Accessor.count, Win32EnlargeMemory);

                s32 ComponentSize = glTFGetComponentSize(Accessor.componentType);
                s32 StartIndex = BufferView.byteOffset + Accessor.byteOffset;
                for (size_t BufferIndex = StartIndex;
                     BufferIndex < StartIndex + Accessor.count * ComponentSize;
                     BufferIndex += ComponentSize)
                {
                    u32 Index = glTFGetBufferValue<u32>(Accessor.componentType, 
                                                        &Buffer, 
                                                        &BufferView, 
                                                        BufferIndex);
                    u32 *pIndices = Indices.Memory.PushStruct<u32>();
                    *pIndices = Index;
                }

                // Position
                Idx = PositionIt->second;
                Assert(Idx >= 0);

                Accessor = TinyModel.accessors[Idx];
                BufferView = TinyModel.bufferViews[Accessor.bufferView];
                Buffer = TinyModel.buffers[BufferView.buffer];

                EnlargeTemporaryMemory(&Positions, Accessor.count, Win32EnlargeMemory);

                ComponentSize = glTFGetComponentSize(Accessor.componentType);
                StartIndex = BufferView.byteOffset + Accessor.byteOffset;
                for (size_t BufferIndex = StartIndex;
                     BufferIndex < StartIndex + Accessor.count * ComponentSize;
                     BufferIndex += ComponentSize)
                {
                    f32 Position = glTFGetBufferValue<f32>(Accessor.componentType, 
                                                           &Buffer,
                                                           &BufferView,
                                                           BufferIndex);
                    f32 *pPosition = Positions.Memory.PushStruct<f32>();
                    *pPosition = Position;
                }

                // Normals
                Idx = NormalIt->second;
                Assert(Idx >= 0);
                
                Accessor = TinyModel.accessors[Idx];
                BufferView = TinyModel.bufferViews[Accessor.bufferView];
                Buffer = TinyModel.buffers[BufferView.buffer];

                EnlargeTemporaryMemory(&Normals, Accessor.count, Win32EnlargeMemory);

                ComponentSize = glTFGetComponentSize(Accessor.componentType);
                StartIndex = BufferView.byteOffset + Accessor.byteOffset;
                for (size_t BufferIndex = StartIndex;
                     BufferIndex < StartIndex + Accessor.count * ComponentSize;
                     BufferIndex += ComponentSize)
                {
                    f32 Normal = glTFGetBufferValue<f32>(Accessor.componentType,
                                                         &Buffer,
                                                         &BufferView,
                                                         BufferIndex);
                    f32 *pNormal = Normals.Memory.PushStruct<f32>();
                    *pNormal = Normal;
                }

                size_t VertexCount = TinyModel.accessors[PositionIt->second].count;               
                if (HasTangent)
                {
                    EnlargeTemporaryMemory(&Tangents, VertexCount * 4, Win32EnlargeMemory);

                    Idx = TangentIt->second;
                    Assert(Idx >= 0);

                    Accessor = TinyModel.accessors[Idx];
                    BufferView = TinyModel.bufferViews[Accessor.bufferView];
                    Buffer = TinyModel.buffers[BufferView.buffer];

                    ComponentSize = glTFGetComponentSize(Accessor.componentType);
                    StartIndex = BufferView.byteOffset + Accessor.byteOffset;
                    for (size_t BufferIndex = StartIndex;
                         BufferIndex < StartIndex + Accessor.count * ComponentSize;
                         BufferIndex += ComponentSize)
                    {
                        f32 Tangent = glTFGetBufferValue<f32>(Accessor.componentType,
                                                              &Buffer,
                                                              &BufferView,
                                                              BufferIndex);
                        f32 *pTangent = Tangents.Memory.PushStruct<f32>();
                        *pTangent = Tangent;
                    }
                }

                if (HasTexcoord0)
                {
                    EnlargeTemporaryMemory(&UVs, VertexCount * 2, Win32EnlargeMemory);

                    Idx = Texcoord0It->second;
                    Assert(Idx >= 0);
                    
                    Accessor = TinyModel.accessors[Idx];
                    BufferView = TinyModel.bufferViews[Accessor.bufferView];
                    Buffer = TinyModel.buffers[BufferView.buffer];

                    ComponentSize = glTFGetComponentSize(Accessor.componentType);
                    StartIndex = BufferView.byteOffset + Accessor.byteOffset;
                    for (size_t BufferIndex = StartIndex;
                         BufferIndex < StartIndex + Accessor.count * ComponentSize;
                         BufferIndex += ComponentSize)
                    {
                        f32 Texcoord0 = glTFGetBufferValue<f32>(Accessor.componentType,
                                                                &Buffer,
                                                                &BufferView,
                                                                BufferIndex);
                        f32 *pTexcoord0 = UVs.Memory.PushStruct<f32>();
                        *pTexcoord0 = Texcoord0;
                    }
                }

                if (HasTexcoord1)
                {
                    EnlargeTemporaryMemory(&UVs2, VertexCount * 2, Win32EnlargeMemory);

                    Idx = Texcoord1It->second;
                    Assert(Idx >= 0);

                    Accessor = TinyModel.accessors[Idx];
                    BufferView = TinyModel.bufferViews[Accessor.bufferView];
                    Buffer = TinyModel.buffers[BufferView.buffer];

                    ComponentSize = glTFGetComponentSize(Accessor.componentType);
                    StartIndex = BufferView.byteOffset + Accessor.byteOffset;
                    for (size_t BufferIndex = StartIndex;
                         BufferIndex < StartIndex + Accessor.count * ComponentSize;
                         BufferIndex += ComponentSize)
                    {
                        f32 Texcoord1 = glTFGetBufferValue<f32>(Accessor.componentType,
                                                                &Buffer,
                                                                &BufferView,
                                                                BufferIndex);
                        f32 *pTexcoord1 = UVs2.Memory.PushStruct<f32>();
                        *pTexcoord1 = Texcoord1;
                    }
                }

                Mesh->VertextCount = VertexCount;
                Mesh->VerticesSize = sizeof(u32) * VertexCount;
                Mesh->Vertices = Memory->PushArray<vertex>(VertexCount);
                Result->Size += sizeof(vertex) * VertexCount;

                for (size_t VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
                {
                    u64 FourElementsProp[4] =
                    {
                        4 * VertexIndex,
                        4 * VertexIndex + 1,
                        4 * VertexIndex + 2,
                        4 * VertexIndex + 3
                    };

                    u64 ThreeElementsProp[3] =
                    {
                        3 * VertexIndex,
                        3 * VertexIndex + 1,
                        3 * VertexIndex + 2
                    };

                    u64 TwoElementsProp[2] =
                    {
                        2 * VertexIndex,
                        2 * VertexIndex + 1
                    };

                    vertex *Vertex = &Mesh->Vertices[VertexIndex];
                    if (HasPosition)
                    {
                        Vertex->Pos = glm::vec3(Positions.Data[ThreeElementsProp[0]],
                                                Positions.Data[ThreeElementsProp[1]],
                                                Positions.Data[ThreeElementsProp[2]]);
                    }
                    if (HasNormal)
                    {
                        Vertex->Normal = glm::vec3(Normals.Data[ThreeElementsProp[0]],
                                                   Normals.Data[ThreeElementsProp[1]],
                                                   Normals.Data[ThreeElementsProp[2]]);
                    }
                    if (HasTangent)
                    {
                        Vertex->TangentSpace = glm::vec4(Tangents.Data[FourElementsProp[0]],
                                                         Tangents.Data[FourElementsProp[1]],
                                                         Tangents.Data[FourElementsProp[2]],
                                                         Tangents.Data[FourElementsProp[3]]);
                    }
                    if (HasTexcoord0)
                    {
                        Vertex->TexCoord0 = glm::vec2(UVs.Data[TwoElementsProp[0]],
                                                      UVs.Data[TwoElementsProp[1]]);
                    }
                    if (HasTexcoord1)
                    {
                        Vertex->TexCoord1 = glm::vec2(UVs2.Data[TwoElementsProp[0]],
                                                      UVs2.Data[TwoElementsProp[1]]);
                    }

                    ApplyTransform(Vertex, &Matrix);
                    
                }

            }

            EndTemporaryMemory(&Indices, Win32DeallocateMemory);
            EndTemporaryMemory(&Positions, Win32DeallocateMemory);
            EndTemporaryMemory(&Normals, Win32DeallocateMemory);
            EndTemporaryMemory(&Tangents, Win32DeallocateMemory);
            EndTemporaryMemory(&UVs, Win32DeallocateMemory);
            EndTemporaryMemory(&UVs2, Win32DeallocateMemory);
        }
    }

    int END = 5;

    return Result;
}

internal PLATFORM_UNLOAD_ASSET(Win32UnloadAsset)
{
}

/// WinMain

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    LARGE_INTEGER QueryPerformanceFrequencyResult;
    QueryPerformanceFrequency(&QueryPerformanceFrequencyResult);
    GlobalPerformanceFrequency = QueryPerformanceFrequencyResult.QuadPart;
    
    Win32GetEXEFilename();
    
    char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename("warpunk.dll",
                              sizeof(SourceGameCodeDLLFullPath),
                              SourceGameCodeDLLFullPath);
    
    char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename("handmade_temp.dll",
                              sizeof(TempGameCodeDLLFullPath),
                              TempGameCodeDLLFullPath);
    
    char GameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename("lock.tmp",
                              sizeof(GameCodeLockFullPath),
                              GameCodeLockFullPath);
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.lpszClassName = "WarpunkWindowClass";
    
    platform_api PlatformAPI = {};
    PlatformAPI.AddWorkQueueEntry = Win32AddWorkQueueEntry;
    PlatformAPI.CompleteAllWork = Win32CompleteAllWork;
    PlatformAPI.AllocateMemory = Win32AllocateMemory;
    PlatformAPI.DeallocateMemory = Win32DeallocateMemory;
    PlatformAPI.EnlargeMemory = Win32EnlargeMemory;

    glm::vec2 RenderDim = { 1424, 728 };
    Win32ResizeDIBSection(&GlobalBackbuffer, RenderDim.x, RenderDim.y);
    
    work_thread HighPriorityThreads[12] = {};
    work_queue HighPriorityQueue = {};
    Win32CreateWorkQueue(&HighPriorityQueue, ArraySize(HighPriorityThreads), HighPriorityThreads);

    work_thread LowPriorityThreads[4] = {};
    work_queue LowPriorityQueue = {};
    Win32CreateWorkQueue(&LowPriorityQueue, ArraySize(LowPriorityThreads), LowPriorityThreads);

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0,
                                      WindowClass.lpszClassName,
                                      "Warpunk",
                                      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      0,
                                      0,
                                      Instance,
                                      0);
        
        GlobalWin32State.DefaultWindowHandle = Window;

        if (Window)
        {
            HDC RendererDC = GetDC(Window);
            
            // TODO(matthias): implement platform api call!
            memory_block RendererMemory = {};
            RendererMemory.Allocated = GB(2);
            RendererMemory.Used = 0;
            RendererMemory.Data = VirtualAlloc(0, GB(2), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            memory_block GameMemory = {};
            GameMemory.Allocated = GB(2);
            GameMemory.Used = 0;
            GameMemory.Data = VirtualAlloc(0, GB(2), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            Win32InitializeVulkan(RendererDC, Window, Instance, &RendererMemory, &PlatformAPI);
            Win32InitializeXAudio2();

            int MonitorRefreshHz = 60;
            int Win32RefreshRate = GetDeviceCaps(RendererDC, VREFRESH);
            if (Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            f32 GameUpadteHz = (f32)(MonitorRefreshHz);
            
            game_context GameContext = {};
            GameContext.Memory = &GameMemory;
            GameContext.HighPriorityQueue = &HighPriorityQueue;
            GameContext.LowPriorityQueue = &LowPriorityQueue;
            GameContext.PlatformAPI = &PlatformAPI;

            game_input GameInput = {};
            GameInput.RenderWidth = RenderDim.x;
            GameInput.RenderHeight = RenderDim.y;
            
            game_debug_info GameDebugInfo = {};
            GameDebugInfo.Keyboard = &GameInput.Keyboard; 
            GameDebugInfo.Mouse = &GameInput.Mouse; 
            GameDebugInfo.Keyboard = &GameInput.Keyboard; 
            
            win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                     TempGameCodeDLLFullPath,
                                                     GameCodeLockFullPath);

            LARGE_INTEGER LastCounter = Win32GetWallClock();
            LARGE_INTEGER FlipWallClock = Win32GetWallClock();
            if (Game.Initialize)
            {
                Game.Initialize(&GameContext, &GameInput, &GameDebugInfo);
            }
            // TODO: Move in Game!
            std::queue<u32> PendingAudios = {};
            
            GlobalRunning = true;
            ShowWindow(Window, SW_SHOW);
            
            u32 ExpectedFramesPerUpdate = 1;
            f32 TargetSecondsPerFrame = (f32)ExpectedFramesPerUpdate / (f32)GameUpadteHz;
            while (GlobalRunning)
            {
                GameInput.dtForFrame = TargetSecondsPerFrame;
                GameInput.Keyboard = {};
                
                Win32ProcessAudioStreams(&PendingAudios);
                Win32ProcessPendingMessages(Window, &GameInput);
                
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                HDC DeviceContext = GetDC(Window);
                
                //
                // Update
                //
                
                if (Game.UpdateAndRender)
                {
                    Game.UpdateAndRender(&GameContext, &GameInput, &GameDebugInfo);
                    game_state *GameState = GameContext.GameState;
                    camera *Camera = &GameState->Camera;
                    GameDebugInfo.Camera = Camera;
                    
                    Win32DrawFrame(&VulkanContext, 
                                   RenderDim, 
                                   GameInput.dtForFrame, 
                                   Camera, 
                                   &GameDebugInfo);
                    
                    if (GameInput.Keyboard.IsPressedOnce(keyboard_buttons::KeyboardButton_A))
                    {
                        PendingAudios.push(0);
                    }
                }
                
                //
                //
                
                if (GameInput.Mouse.State[mouse_buttons::MouseButton_Left].IsDragging) 
                {
                    s32 SelectionRectLeft = Min(GameInput.Mouse.DownPx, GameInput.Mouse.Px);
                    s32 SelectionRectTop = Max(GameInput.Mouse.DownPy, GameInput.Mouse.Py);
                    s32 SelectionRectRight = Max(GameInput.Mouse.DownPx, GameInput.Mouse.Px);
                    s32 SelectionRectBottom = Min(GameInput.Mouse.DownPy, GameInput.Mouse.Py);
                }
                
                GameInput.Mouse.IsMoving = false;
                if (!GameInput.Mouse.IsPressed(MouseButton_Left) &&
                    !GameInput.Mouse.IsPressed(MouseButton_Middle) &&
                    !GameInput.Mouse.IsPressed(MouseButton_Right))
                {
                    GameInput.Mouse.DraggingInitialDtx = 0;
                    GameInput.Mouse.DraggingInitialDty = 0;
                    GameInput.Mouse.DraggingRecentDtx = 0;
                    GameInput.Mouse.DraggingRecentDty = 0;
                }
                for(u32 MouseStateIndex = 0;
                    MouseStateIndex < MouseButton_Count;
                    ++MouseStateIndex)
                {
                    GameInput.Mouse.State[MouseStateIndex].WasDown = GameInput.Mouse.State[MouseStateIndex].IsDown;
                }
                for(u32 KeyboardStateIndex = 0;
                    KeyboardStateIndex < keyboard_buttons::KeyboardButton_Count;
                    ++KeyboardStateIndex)
                {
                    GameInput.Keyboard.State[KeyboardStateIndex].WasDown =
                        GameInput.Keyboard.State[KeyboardStateIndex].IsDown;
                    GameInput.Keyboard.State[KeyboardStateIndex].IsDown = false;
                }
                
                //
                //
                
                FlipWallClock = Win32GetWallClock();
                
                LARGE_INTEGER EndCounter = Win32GetWallClock();
                f32 MeasuredSecondsPerFrame = Win32GetSecondsElapsed(LastCounter, EndCounter);
                f32 ExactTargetFramesPerUpdate = MeasuredSecondsPerFrame * (f32)MonitorRefreshHz;
                
                TargetSecondsPerFrame = ExactTargetFramesPerUpdate / MonitorRefreshHz;
                char OutputBuffer[256];
                snprintf(OutputBuffer, sizeof(OutputBuffer), "FPS: %.1f\n", 1/TargetSecondsPerFrame);
                OutputDebugStringA(OutputBuffer);
                
                LastCounter = EndCounter;
            }
        }
    }
    ExitProcess(0);
}

