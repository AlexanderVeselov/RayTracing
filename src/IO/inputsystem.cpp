/*****************************************************************************
 MIT License

 Copyright(c) 2021 Alexander Veselov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this softwareand associated documentation files(the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions :

 The above copyright noticeand this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *****************************************************************************/

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
