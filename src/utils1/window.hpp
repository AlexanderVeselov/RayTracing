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

#include <cstdint>

class Window
{
public:
    Window(char const* title, std::uint32_t width, std::uint32_t height);

    // Native handles
    void* GetNativeHandle() const { return handle_; }
    void* GetDisplayContext() const { return display_context_; }
    void* GetGLContext() const { return gl_context_; }

    std::uint32_t GetWidth() const { return width_; }
    std::uint32_t GetHeight() const { return height_; }
    bool ShouldClose() const { return should_close_; }
    void PollEvents();
    void SwapBuffers();
    void OnKeyPressed(int key);
    void OnKeyReleased(int key);
    void OnMousePressed(unsigned int button);
    void OnMouseReleased(unsigned int button);
    bool IsKeyPressed(int key) const;
    bool IsLeftMouseButtonPressed() const;
    bool IsRightMouseButtonPressed() const;
    void GetMousePos(int* x, int* y) const;
    void SetMousePos(int x, int y) const;

    ~Window();

private:
    void InitGL();

    void* handle_;
    void* display_context_;
    void* gl_context_;

    std::uint32_t width_;
    std::uint32_t height_;
    bool should_close_ = false;
    bool keys_[256];
    unsigned int mouse_ = 0;
};
