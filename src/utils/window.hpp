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

#include <GLFW/glfw3.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

enum class KeyCode
{
    kUnknown = -1,
    kSpace,
    kApostrophe,
    kComma,
    kMinus,
    kPeriod,
    kSlash,
    k0,
    k1,
    k2,
    k3,
    k4,
    k5,
    k6,
    k7,
    k8,
    k9,
    kSemicolon,
    kEqual,
    kA,
    kB,
    kC,
    kD,
    kE,
    kF,
    kG,
    kH,
    kI,
    kJ,
    kK,
    kL,
    kM,
    kN,
    kO,
    kP,
    kQ,
    kR,
    kS,
    kT,
    kU,
    kV,
    kW,
    kX,
    kY,
    kZ,
    kLeftBracket,
    kBackslash,
    kRightBracket,
    kGraceAccent,
    kEscape,
    kEnter,
    kTab,
    kBackspace,
    kInsert,
    kDelete,
    kRight,
    kLeft,
    kDown,
    kUp,
    kPageUp,
    kPageDown,
    kHome,
    kEnd,
    kCapsLock,
    kScrollLock,
    kNumLock,
    kPrintScreen,
    kPause,
    kF1,
    kF2,
    kF3,
    kF4,
    kF5,
    kF6,
    kF7,
    kF8,
    kF9,
    kF10,
    kF11,
    kF12,
    kKeypad0,
    kKeypad1,
    kKeypad2,
    kKeypad3,
    kKeypad4,
    kKeypad5,
    kKeypad6,
    kKeypad7,
    kKeypad8,
    kKeypad9,
    kKeypadDecimal,
    kKeypadDivide,
    kKeypadMultiply,
    kKeypadSubtract,
    kKeypadAdd,
    kKeypadEnter,
    kKeypadEqual,
    kLeftShift,
    kLeftControl,
    kLeftAlt,
    kRightShift,
    kRightControl,
    kRightAlt,
};

enum class MouseButton
{
    kLeft,
    kRight,
    kMiddle,
};

class Window
{
public:
    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    Window(std::uint32_t width, std::uint32_t height, char const* title, bool no_api = false);
    ~Window();

    // Returns HWND in the case of WIN32 platform
    void* GetNativeHandle() const;
    std::uint32_t GetWidth() const { return width_; }
    std::uint32_t GetHeight() const { return height_; }

    void GetMousePos(int& x, int& y) const;
    void SetMousePos(int x, int y) const;

    void PollEvents();
    bool ShouldClose() const;
    bool GetKey(KeyCode code) const;
    bool GetMouseButton(MouseButton button) const;
    void SwapBuffers();
    void AddScrollCallback(std::function<void(float)> callback) { scroll_callbacks_.push_back(callback); }

private:
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)> window_;
    std::vector<std::function<void(float)>> scroll_callbacks_;
    std::uint32_t width_ = ~0u;
    std::uint32_t height_ = ~0u;
};
