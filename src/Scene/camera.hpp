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

#include "mathlib/mathlib.hpp"
#include <memory>

class Render;
class Framebuffer;

class Camera
{
public:
    Camera(Framebuffer& framebuffer, Render& render);
    void Update();

    float3 GetOrigin()       const { return m_Origin; }
    float3 GetFrontVector()  const { return m_Front; }
    float3 GetUpVector()     const { return m_Up; }

    void SetAperture(float aperture) { aperture_ = aperture; }
    void SetFocusDistance(float focus_distance) { focus_distance_ = focus_distance; }
    float GetAperture() const { return aperture_; }
    float GetFocusDistance() const { return focus_distance_; }

    bool IsChanged()        const { return m_Changed; }
    unsigned int GetFrameCount() const { return m_FrameCount; }

private:
    Render& m_Render;
    Framebuffer& framebuffer_;

    float3 m_Origin;
    float3 m_Velocity;
    float3 m_Front;
    float3 m_Up;

    float m_Pitch;
    float m_Yaw;
    float m_Speed;
    float aperture_ = 0.0f;
    float focus_distance_ = 10.0f;

    unsigned int m_FrameCount;

    bool m_Changed;

};
