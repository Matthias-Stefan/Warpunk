#include "source/platform/keyboard.h" 

#include "source/core/types.h"

bool keyboard::IsPressedOnce(keyboard_buttons KeyboardButton)
{
    return State[KeyboardButton].IsDown && !(State[KeyboardButton].WasDown);
}

bool keyboard::IsPressed(keyboard_buttons KeyboardButton)
{
    return State[KeyboardButton].IsDown || State[KeyboardButton].WasDown; 
}
