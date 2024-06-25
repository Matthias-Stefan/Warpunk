#pragma once

#include "source/core/types.h"

//////////////////////////////////////////////////////////////////////

enum mouse_buttons
{
    MouseButton_Left,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_Wheel,
    
    MouseButton_Count,
};

//////////////////////////////////////////////////////////////////////

struct mouse_state
{
    bool IsDown;
    bool WasDown;
    bool IsDragging;
};

//////////////////////////////////////////////////////////////////////

struct mouse
{
    bool IsPressedOnce(mouse_buttons MouseButton);
    bool IsPressed(mouse_buttons MouseButton);
    bool IsDragging(mouse_buttons MouseButton);
    bool IsWheelTurned();
    
    bool IsMoving;
    mouse_state State[MouseButton_Count];
    
    s32 Px;
    s32 Py;
    s32 DownPx;
    s32 DownPy;
    s32 DraggingInitialDtx;
    s32 DraggingInitialDty;
    s32 DraggingRecentDtx;
    s32 DraggingRecentDty;
    s32 WheelDelta;
};

//////////////////////////////////////////////////////////////////////

