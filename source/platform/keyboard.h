#pragma once

//////////////////////////////////////////////////////////////////////

enum keyboard_buttons
{
    KeyboardButton_A,
    KeyboardButton_B,
    KeyboardButton_C,
    KeyboardButton_D,
    KeyboardButton_E,
    KeyboardButton_F,
    KeyboardButton_G,
    KeyboardButton_H,
    KeyboardButton_I,
    KeyboardButton_J,
    KeyboardButton_K,
    KeyboardButton_L,
    KeyboardButton_M,
    KeyboardButton_N, 
    KeyboardButton_O,
    KeyboardButton_P,
    KeyboardButton_Q,
    KeyboardButton_R,
    KeyboardButton_S,
    KeyboardButton_T,
    KeyboardButton_U,
    KeyboardButton_V,
    KeyboardButton_W,
    KeyboardButton_X,
    KeyboardButton_Y,
    KeyboardButton_Z,
    KeyboardButton_ArrowLeft,
    KeyboardButton_ArrowUp,
    KeyboardButton_ArrowRight,
    KeyboardButton_ArrowDown,
    KeyboardButton_Alt,
    
    KeyboardButton_Count,
};

//////////////////////////////////////////////////////////////////////

struct keyboard_state
{
    bool IsDown;
    bool WasDown;
};

//////////////////////////////////////////////////////////////////////

struct keyboard
{
    bool IsPressedOnce(keyboard_buttons KeyboardButton);
    bool IsPressed(keyboard_buttons KeyboardButton);
    
    keyboard_state State[KeyboardButton_Count];
};

//////////////////////////////////////////////////////////////////////
