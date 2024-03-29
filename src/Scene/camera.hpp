#ifndef CAMERA_HPP
#define CAMERA_HPP

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

#endif // CAMERA_HPP
