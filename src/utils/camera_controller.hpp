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

#pragma once

#include "mathlib/mathlib.hpp"
#include "kernels/common/shared_structures.h"
#include <memory>

class Window;

class CameraController
{
public:
    CameraController(Window& window);
    void Update(float dt);
    bool IsChanged() const { return is_changed_; }
    void OnEndFrame() { is_changed_ = false; }
    Camera const& GetData() const { return camera_data_; }

    void SetAperture(float aperture) { camera_data_.aperture = aperture; is_changed_ = true; }
    void SetFocusDistance(float focus_distance) { camera_data_.focus_distance = focus_distance; is_changed_ = true; }

private:
    Window& window_;

    bool is_changed_ = true;
    Camera camera_data_ = {};
    float3 up_;
    float pitch_;
    float yaw_;
    float speed_;
};
