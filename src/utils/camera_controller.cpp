/*****************************************************************************
 MIT License

 Copyright(c) 2022 Alexander Veselov

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

#include "camera_controller.hpp"
#include "render.hpp"
#include "Utils/window.hpp"
#include <iostream>

CameraController::CameraController(Window& window)
    : window_(window)
    , pitch_(MATH_PIDIV2)
    , yaw_(MATH_PIDIV2)
    , speed_(1.0f)
    , up_(0.0f, 0.0f, 1.0f)
{
    camera_data_.focus_distance = 10.0f;
    camera_data_.position = float3(0.0f, -1.0f, 1.0f);
    camera_data_.fov = 75.0f * 3.1415f / 180.0f;
    camera_data_.aspect_ratio = (float)window_.GetWidth() / (float)window_.GetHeight();
}

void CameraController::Update(float dt)
{
    static int prev_x = 0, prev_y = 0;
    int x, y;
    window_.GetMousePos(&x, &y);

    if (window_.IsRightMouseButtonPressed())
    {
        float sensivity = 0.00075f;
        yaw_ -= (x - prev_x) * sensivity;
        pitch_ += (y - prev_y) * sensivity;
        float epsilon = 0.0001f;
        pitch_ = clamp(pitch_, 0.0f + epsilon, MATH_PI - epsilon);
        window_.SetMousePos(prev_x, prev_y);
        is_changed_ = true;
    }
    else
    {
        prev_x = x;
        prev_y = y;
    }

    int frontback = window_.IsKeyPressed('W') - window_.IsKeyPressed('S');
    int strafe = window_.IsKeyPressed('A') - window_.IsKeyPressed('D');

    if (frontback != 0 || strafe != 0)
    {
        is_changed_ = true;
    }

    camera_data_.position += float3(std::cosf(yaw_) * std::sinf(pitch_) * frontback - std::cosf(yaw_ - MATH_PIDIV2) * strafe,
        std::sinf(yaw_) * std::sinf(pitch_) * frontback - std::sinf(yaw_ - MATH_PIDIV2) * strafe, std::cosf(pitch_) * frontback) * speed_ * dt;
    camera_data_.front = float3(std::cosf(yaw_) * std::sinf(pitch_), std::sinf(yaw_) * std::sinf(pitch_), std::cosf(pitch_));

    // Compute the actual up vector
    float3 right = Cross(camera_data_.front, up_).Normalize();
    camera_data_.up = Cross(right, camera_data_.front);
}
