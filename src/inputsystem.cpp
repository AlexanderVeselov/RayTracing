#include "inputsystem.hpp"
#include <Windows.h>

static InputSystem g_InputSystem;
InputSystem* input = &g_InputSystem;

InputSystem::InputSystem() : m_Mouse(0)
{
    for (unsigned int i = 0; i < 255; ++i)
    {
        m_Keys[i] = false;
    }

}

void InputSystem::KeyDown(unsigned int key)
{
    m_Keys[key] = true;
}

void InputSystem::KeyUp(unsigned int key)
{
    m_Keys[key] = false;
}

bool InputSystem::IsKeyDown(unsigned int key) const
{
    return m_Keys[key];
}

void InputSystem::SetMousePos(unsigned short x, unsigned short y) const
{
    SetCursorPos(x, y);
}

void InputSystem::GetMousePos(unsigned short* x, unsigned short* y) const
{
    POINT point;
    GetCursorPos(&point);
    *x = point.x;
    *y = point.y;

}

void InputSystem::MousePressed(unsigned int button)
{
    m_Mouse |= button;
}

void InputSystem::MouseReleased(unsigned int button)
{
    m_Mouse &= ~button;
}

bool InputSystem::IsMousePressed(unsigned int button) const
{
    return m_Mouse & button;
}
