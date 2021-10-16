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

#pragma once

class InputSystem
{
public:
    InputSystem();

    void KeyDown(unsigned int);
    void KeyUp(unsigned int);
    void SetMousePos(unsigned short x, unsigned short y) const;
    bool IsKeyDown(unsigned int) const;
    void GetMousePos(unsigned short *x, unsigned short *y) const;
    void MousePressed(unsigned int button);
    void MouseReleased(unsigned int button);
    bool IsMousePressed(unsigned int button) const;

private:
    bool m_Keys[256];
    unsigned int m_Mouse;

};

extern InputSystem* input;
