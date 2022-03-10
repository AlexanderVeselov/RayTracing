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


#include <GL/glew.h>

#include "window.hpp"
#include <imgui.h>

#ifdef WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#else
#error Other platforms are not currently supported
#endif

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <stdexcept>
#include <unordered_map>
#include <cassert>

namespace
{
const std::unordered_map<KeyCode, int> kKeyCodeToGlfwKey =
{
    { KeyCode::kUnknown       , GLFW_KEY_UNKNOWN       },
    { KeyCode::kSpace         , GLFW_KEY_SPACE         },
    { KeyCode::kApostrophe    , GLFW_KEY_APOSTROPHE    },
    { KeyCode::kComma         , GLFW_KEY_COMMA         },
    { KeyCode::kMinus         , GLFW_KEY_MINUS         },
    { KeyCode::kPeriod        , GLFW_KEY_PERIOD        },
    { KeyCode::kSlash         , GLFW_KEY_SLASH         },
    { KeyCode::k0             , GLFW_KEY_0             },
    { KeyCode::k1             , GLFW_KEY_1             },
    { KeyCode::k2             , GLFW_KEY_2             },
    { KeyCode::k3             , GLFW_KEY_3             },
    { KeyCode::k4             , GLFW_KEY_4             },
    { KeyCode::k5             , GLFW_KEY_5             },
    { KeyCode::k6             , GLFW_KEY_6             },
    { KeyCode::k7             , GLFW_KEY_7             },
    { KeyCode::k8             , GLFW_KEY_8             },
    { KeyCode::k9             , GLFW_KEY_9             },
    { KeyCode::kSemicolon     , GLFW_KEY_SEMICOLON     },
    { KeyCode::kEqual         , GLFW_KEY_EQUAL         },
    { KeyCode::kA             , GLFW_KEY_A             },
    { KeyCode::kB             , GLFW_KEY_B             },
    { KeyCode::kC             , GLFW_KEY_C             },
    { KeyCode::kD             , GLFW_KEY_D             },
    { KeyCode::kE             , GLFW_KEY_E             },
    { KeyCode::kF             , GLFW_KEY_F             },
    { KeyCode::kG             , GLFW_KEY_G             },
    { KeyCode::kH             , GLFW_KEY_H             },
    { KeyCode::kI             , GLFW_KEY_I             },
    { KeyCode::kJ             , GLFW_KEY_J             },
    { KeyCode::kK             , GLFW_KEY_K             },
    { KeyCode::kL             , GLFW_KEY_L             },
    { KeyCode::kM             , GLFW_KEY_M             },
    { KeyCode::kN             , GLFW_KEY_N             },
    { KeyCode::kO             , GLFW_KEY_O             },
    { KeyCode::kP             , GLFW_KEY_P             },
    { KeyCode::kQ             , GLFW_KEY_Q             },
    { KeyCode::kR             , GLFW_KEY_R             },
    { KeyCode::kS             , GLFW_KEY_S             },
    { KeyCode::kT             , GLFW_KEY_T             },
    { KeyCode::kU             , GLFW_KEY_U             },
    { KeyCode::kV             , GLFW_KEY_V             },
    { KeyCode::kW             , GLFW_KEY_W             },
    { KeyCode::kX             , GLFW_KEY_X             },
    { KeyCode::kY             , GLFW_KEY_Y             },
    { KeyCode::kZ             , GLFW_KEY_Z             },
    { KeyCode::kLeftBracket   , GLFW_KEY_LEFT_BRACKET  },
    { KeyCode::kBackslash     , GLFW_KEY_BACKSLASH     },
    { KeyCode::kRightBracket  , GLFW_KEY_RIGHT_BRACKET },
    { KeyCode::kGraceAccent   , GLFW_KEY_GRAVE_ACCENT  },
    { KeyCode::kEscape        , GLFW_KEY_ESCAPE        },
    { KeyCode::kEnter         , GLFW_KEY_ENTER         },
    { KeyCode::kTab           , GLFW_KEY_TAB           },
    { KeyCode::kBackspace     , GLFW_KEY_BACKSPACE     },
    { KeyCode::kInsert        , GLFW_KEY_INSERT        },
    { KeyCode::kDelete        , GLFW_KEY_DELETE        },
    { KeyCode::kRight         , GLFW_KEY_RIGHT         },
    { KeyCode::kLeft          , GLFW_KEY_LEFT          },
    { KeyCode::kDown          , GLFW_KEY_DOWN          },
    { KeyCode::kUp            , GLFW_KEY_UP            },
    { KeyCode::kPageUp        , GLFW_KEY_PAGE_UP       },
    { KeyCode::kPageDown      , GLFW_KEY_PAGE_DOWN     },
    { KeyCode::kHome          , GLFW_KEY_HOME          },
    { KeyCode::kEnd           , GLFW_KEY_END           },
    { KeyCode::kCapsLock      , GLFW_KEY_CAPS_LOCK     },
    { KeyCode::kScrollLock    , GLFW_KEY_SCROLL_LOCK   },
    { KeyCode::kNumLock       , GLFW_KEY_NUM_LOCK      },
    { KeyCode::kPrintScreen   , GLFW_KEY_PRINT_SCREEN  },
    { KeyCode::kPause         , GLFW_KEY_PAUSE         },
    { KeyCode::kF1            , GLFW_KEY_F1            },
    { KeyCode::kF2            , GLFW_KEY_F2            },
    { KeyCode::kF3            , GLFW_KEY_F3            },
    { KeyCode::kF4            , GLFW_KEY_F4            },
    { KeyCode::kF5            , GLFW_KEY_F5            },
    { KeyCode::kF6            , GLFW_KEY_F6            },
    { KeyCode::kF7            , GLFW_KEY_F7            },
    { KeyCode::kF8            , GLFW_KEY_F8            },
    { KeyCode::kF9            , GLFW_KEY_F9            },
    { KeyCode::kF10           , GLFW_KEY_F10           },
    { KeyCode::kF11           , GLFW_KEY_F11           },
    { KeyCode::kF12           , GLFW_KEY_F12           },
    { KeyCode::kKeypad0       , GLFW_KEY_KP_0          },
    { KeyCode::kKeypad1       , GLFW_KEY_KP_1          },
    { KeyCode::kKeypad2       , GLFW_KEY_KP_2          },
    { KeyCode::kKeypad3       , GLFW_KEY_KP_3          },
    { KeyCode::kKeypad4       , GLFW_KEY_KP_4          },
    { KeyCode::kKeypad5       , GLFW_KEY_KP_5          },
    { KeyCode::kKeypad6       , GLFW_KEY_KP_6          },
    { KeyCode::kKeypad7       , GLFW_KEY_KP_7          },
    { KeyCode::kKeypad8       , GLFW_KEY_KP_8          },
    { KeyCode::kKeypad9       , GLFW_KEY_KP_9          },
    { KeyCode::kKeypadDecimal , GLFW_KEY_KP_DECIMAL    },
    { KeyCode::kKeypadDivide  , GLFW_KEY_KP_DIVIDE     },
    { KeyCode::kKeypadMultiply, GLFW_KEY_KP_MULTIPLY   },
    { KeyCode::kKeypadSubtract, GLFW_KEY_KP_SUBTRACT   },
    { KeyCode::kKeypadAdd     , GLFW_KEY_KP_ADD        },
    { KeyCode::kKeypadEnter   , GLFW_KEY_KP_ENTER      },
    { KeyCode::kKeypadEqual   , GLFW_KEY_KP_EQUAL      },
    { KeyCode::kLeftShift     , GLFW_KEY_LEFT_SHIFT    },
    { KeyCode::kLeftControl   , GLFW_KEY_LEFT_CONTROL  },
    { KeyCode::kLeftAlt       , GLFW_KEY_LEFT_ALT      },
    { KeyCode::kRightShift    , GLFW_KEY_RIGHT_SHIFT   },
    { KeyCode::kRightControl  , GLFW_KEY_RIGHT_CONTROL },
    { KeyCode::kRightAlt      , GLFW_KEY_RIGHT_ALT     },
};

const std::unordered_map<MouseButton, int> kMouseButtonToGlfwButton =
{
    { MouseButton::kLeft,   GLFW_MOUSE_BUTTON_LEFT   },
    { MouseButton::kRight,  GLFW_MOUSE_BUTTON_RIGHT  },
    { MouseButton::kMiddle, GLFW_MOUSE_BUTTON_MIDDLE },
};

}

Window::Window(std::uint32_t width, std::uint32_t height, char const* title, bool no_api)
    : window_(nullptr, glfwDestroyWindow)
    , width_(width)
    , height_(height)
{
    if (!glfwInit())
    {
        throw std::runtime_error("glfwInit() failed");
    }

    if (no_api)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    // Create the main window
    window_.reset(glfwCreateWindow(width, height, title, nullptr, nullptr));

    if (!window_.get())
    {
        throw std::runtime_error("Failed to create GLFW window!");
    }

    glfwMakeContextCurrent(window_.get());
    glfwSetWindowUserPointer(window_.get(), this);
    glfwSetScrollCallback(window_.get(), ScrollCallback);

    GLenum glew_status = glewInit();

    if (glew_status != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));

        throw std::runtime_error("Failed to init GLEW");
    }

    ImGui::CreateContext();

    ImGui_ImplOpenGL3_Init();
    ImGui_ImplGlfw_InitForOpenGL(window_.get(), true);
}

Window::~Window()
{
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

void Window::PollEvents()
{
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window_.get());
}

bool Window::GetKey(KeyCode code) const
{
    if (ImGui::GetIO().WantCaptureKeyboard)
    {
        return false;
    }

    auto it = kKeyCodeToGlfwKey.find(code);
    assert(it != kKeyCodeToGlfwKey.end());
    int state = glfwGetKey(window_.get(), it->second);
    return (state == GLFW_PRESS) || (state == GLFW_REPEAT);
}

bool Window::GetMouseButton(MouseButton button) const
{
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return false;
    }

    auto it = kMouseButtonToGlfwButton.find(button);
    assert(it != kMouseButtonToGlfwButton.end());
    int state = glfwGetMouseButton(window_.get(), it->second);
    return state == GLFW_PRESS;
}

void Window::GetMousePos(int& x, int& y) const
{
    double xpos, ypos;
    glfwGetCursorPos(window_.get(), &xpos, &ypos);

    // Convert double to int
    x = (int)xpos;
    y = (int)ypos;
}

void Window::SetMousePos(int x, int y) const
{
    glfwSetCursorPos(window_.get(), (double)x, (double)y);
}

void* Window::GetNativeHandle() const
{
    return glfwGetWin32Window(window_.get());
}

void Window::SwapBuffers()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_.get());
}

void Window::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Window* this_window = (Window*)glfwGetWindowUserPointer(window);
    for (auto callback : this_window->scroll_callbacks_)
    {
        callback(yoffset);
    }
}
