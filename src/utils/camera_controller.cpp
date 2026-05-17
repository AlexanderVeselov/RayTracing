/*****************************************************************************
 MIT License

 Copyright(c) 2026 Alexander Veselov

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
#include "utils/window.hpp"

#include <algorithm>
#include <iostream>

#include <glm/gtc/constants.hpp>

CameraController::CameraController(Window& window)
    : window_(window), pitch_(glm::half_pi<float>()), yaw_(glm::half_pi<float>()), speed_(1.0f), up_(0.0f, 0.0f, 1.0f)
{
    camera_data_.focus_distance = 10.0f;
    camera_data_.position_fov = glm::vec4(0.0f, -1.0f, 1.0f, 75.0f * 3.1415f / 180.0f);
    camera_data_.front_aspect.w = (float)window_.GetWidth() / (float)window_.GetHeight();
}

void CameraController::Update(float dt)
{
    static int prev_x = 0, prev_y = 0;
    int x, y;
    window_.GetMousePos(x, y);

    if (window_.GetMouseButton(MouseButton::kRight))
    {
        float sensivity = 0.00075f;
        yaw_ -= (x - prev_x) * sensivity;
        pitch_ += (y - prev_y) * sensivity;
        float epsilon = 0.0001f;
        pitch_ = std::clamp(pitch_, 0.0f + epsilon, glm::pi<float>() - epsilon);
        window_.SetMousePos(prev_x, prev_y);
        is_changed_ = true;
    }
    else
    {
        prev_x = x;
        prev_y = y;
    }

    int frontback = window_.GetKey(KeyCode::kW) - window_.GetKey(KeyCode::kS);
    int strafe = window_.GetKey(KeyCode::kD) - window_.GetKey(KeyCode::kA);
    int updown = window_.GetKey(KeyCode::kE) - window_.GetKey(KeyCode::kQ);

    if (frontback != 0 || strafe != 0 || updown != 0)
    {
        is_changed_ = true;
    }

    float speed = speed_ * (window_.GetKey(KeyCode::kLeftShift) ? 5.0f : 1.0f);

    // Compute new camera vectors
    glm::vec3 front = glm::vec3(std::cosf(yaw_) * std::sinf(pitch_),
        std::sinf(yaw_) * std::sinf(pitch_),
        std::cosf(pitch_));
    camera_data_.front_aspect.x = front.x;
    camera_data_.front_aspect.y = front.y;
    camera_data_.front_aspect.z = front.z;
    glm::vec3 right = glm::normalize(glm::cross(front, up_));
    // Compute the actual up vector
    glm::vec3 camera_up = glm::cross(right, front);
    camera_data_.up_aperture.x = camera_up.x;
    camera_data_.up_aperture.y = camera_up.y;
    camera_data_.up_aperture.z = camera_up.z;
    // Move the camera
    glm::vec3 position = glm::vec3(camera_data_.position_fov);
    position += (front * (float)frontback + right * (float)strafe + camera_up * (float)updown) * dt * speed;
    camera_data_.position_fov.x = position.x;
    camera_data_.position_fov.y = position.y;
    camera_data_.position_fov.z = position.z;
}
